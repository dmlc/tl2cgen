/*!
 * Copyright (c) 2024 by Contributors
 * \file condition_node.cc
 * \brief Convert ConditionNode in AST into C code
 * \author Hyunsu Cho
 */

#include <fmt/format.h>
#include <tl2cgen/detail/compiler/ast/ast.h>
#include <tl2cgen/detail/compiler/codegen/codegen.h>
#include <tl2cgen/detail/compiler/codegen/format_util.h>
#include <tl2cgen/detail/operator_comp.h>
#include <tl2cgen/logging.h>
#include <treelite/enum/operator.h>

#include <cstdint>
#include <string>

using namespace fmt::literals;

namespace {

namespace ast = tl2cgen::compiler::detail::ast;
namespace codegen = tl2cgen::compiler::detail::codegen;

std::string GetFabsCFunc(std::string const& threshold_type) {
  if (threshold_type == "float") {
    return "fabsf";
  } else if (threshold_type == "double") {
    return "fabs";
  } else {
    TL2CGEN_LOG(FATAL) << "Unrecognized type: " << threshold_type;
    return "";
  }
}

inline std::string ExtractNumericalCondition(ast::NumericalConditionNode const* node) {
  std::string const threshold_type = codegen::GetThresholdCType(node);
  std::string result;
  if (node->quantized_threshold_) {  // Quantized threshold
    std::string lhs
        = fmt::format("data[{split_index}].qvalue", "split_index"_a = node->split_index_);
    result = fmt::format("{lhs} {opname} {threshold}", "lhs"_a = lhs,
        "opname"_a = treelite::OperatorToString(node->op_),
        "threshold"_a = *node->quantized_threshold_);
  } else {
    result = std::visit(
        [&](auto&& threshold) -> std::string {
          using ThresholdT = std::remove_const_t<std::remove_reference_t<decltype(threshold)>>;
          if (std::isinf(threshold)) {  // infinite threshold
            // According to IEEE 754, the result of comparison [lhs] < infinity
            // must be identical for all finite [lhs]. Same goes for operator >.
            return (tl2cgen::detail::CompareWithOp(static_cast<ThresholdT>(0), node->op_, threshold)
                        ? "1"
                        : "0");
          } else {  // Finite threshold
            std::string lhs
                = fmt::format("data[{split_index}].fvalue", "split_index"_a = node->split_index_);
            return fmt::format("{lhs} {opname} ({threshold_type}){threshold}", "lhs"_a = lhs,
                "opname"_a = treelite::OperatorToString(node->op_),
                "threshold_type"_a = threshold_type,
                "threshold"_a = codegen::ToStringHighPrecision(ThresholdT{threshold}));
          }
        },
        node->threshold_);
  }
  return result;
}

inline std::vector<std::uint64_t> GetCategoricalBitmap(
    std::vector<std::uint32_t> const& category_list) {
  std::size_t const num_categories = category_list.size();
  if (num_categories == 0) {
    return std::vector<std::uint64_t>{0};
  }
  std::uint32_t const max_category = category_list[num_categories - 1];
  std::vector<std::uint64_t> bitmap((max_category + 1 + 63) / 64, 0);
  for (std::uint32_t cat : category_list) {
    std::size_t const idx = cat / 64;
    std::uint32_t const offset = cat % 64;
    bitmap[idx] |= (static_cast<std::uint64_t>(1) << offset);
  }
  return bitmap;
}

inline std::string ExtractCategoricalCondition(ast::CategoricalConditionNode const* node) {
  std::string const threshold_ctype_str = codegen::GetThresholdCType(node);
  std::string const fabs = GetFabsCFunc(threshold_ctype_str);

  std::string result;
  std::vector<std::uint64_t> bitmap = GetCategoricalBitmap(node->category_list_);
  TL2CGEN_CHECK_GE(bitmap.size(), 1);
  bool all_zeros = true;
  for (std::uint64_t e : bitmap) {
    all_zeros &= (e == 0);
  }
  if (all_zeros) {
    result = "0";
  } else {
    std::ostringstream oss;
    std::string const right_categories_flag = (node->category_list_right_child_ ? "!" : "");
    if (node->default_left_) {
      oss << fmt::format(
          "data[{split_index}].missing == -1 || {right_categories_flag}("
          "(tmp = (unsigned int)(data[{split_index}].fvalue) ), ",
          "split_index"_a = node->split_index_, "right_categories_flag"_a = right_categories_flag);
    } else {
      oss << fmt::format(
          "data[{split_index}].missing != -1 && {right_categories_flag}("
          "(tmp = (unsigned int)(data[{split_index}].fvalue) ), ",
          "split_index"_a = node->split_index_, "right_categories_flag"_a = right_categories_flag);
    }

    oss << fmt::format(
        "((data[{split_index}].fvalue >= 0) && "
        "({fabs}(data[{split_index}].fvalue) <= ({threshold_ctype})(1U << FLT_MANT_DIG)) && (",
        "split_index"_a = node->split_index_, "threshold_ctype"_a = threshold_ctype_str,
        "fabs"_a = fabs);
    oss << "(tmp >= 0 && tmp < 64 && (( (uint64_t)" << bitmap[0] << "U >> tmp) & 1) )";
    for (std::size_t i = 1; i < bitmap.size(); ++i) {
      oss << " || (tmp >= " << (i * 64) << " && tmp < " << ((i + 1) * 64) << " && (( (uint64_t)"
          << bitmap[i] << "U >> (tmp - " << (i * 64) << ") ) & 1) )";
    }
    oss << ")))";
    result = oss.str();
  }
  return result;
}

}  // anonymous namespace

namespace tl2cgen::compiler::detail::codegen {

void HandleConditionNode(ast::ConditionNode const* node, CodeCollection& gencode) {
  ast::NumericalConditionNode const* t;
  std::string condition_with_na_check;
  if ((t = dynamic_cast<ast::NumericalConditionNode const*>(node))) {
    /* Numerical split */
    std::string condition = ExtractNumericalCondition(t);
    char const* condition_with_na_check_template
        = (node->default_left_) ? "!(data[{split_index}].missing != -1) || ({condition})"
                                : " (data[{split_index}].missing != -1) && ({condition})";
    condition_with_na_check = fmt::format(condition_with_na_check_template,
        "split_index"_a = node->split_index_, "condition"_a = condition);
  } else { /* Categorical split */
    auto const* t2 = dynamic_cast<ast::CategoricalConditionNode const*>(node);
    TL2CGEN_CHECK(t2);
    condition_with_na_check = ExtractCategoricalCondition(t2);
  }
  if (node->children_[0]->data_count_ && node->children_[1]->data_count_) {
    std::uint64_t const left_freq = *node->children_[0]->data_count_;
    std::uint64_t const right_freq = *node->children_[1]->data_count_;
    condition_with_na_check = fmt::format(" {keyword}( {condition} ) ",
        "keyword"_a = ((left_freq > right_freq) ? "LIKELY" : "UNLIKELY"),
        "condition"_a = condition_with_na_check);
  }
  gencode.PushFragment(fmt::format("if ({}) {{\n", condition_with_na_check));
  gencode.ChangeIndent(1);
  TL2CGEN_CHECK_EQ(node->children_.size(), 2);
  GenerateCodeFromAST(node->children_[0], gencode);
  gencode.ChangeIndent(-1);
  gencode.PushFragment("} else {");
  gencode.ChangeIndent(1);
  GenerateCodeFromAST(node->children_[1], gencode);
  gencode.ChangeIndent(-1);
  gencode.PushFragment("}");
}

}  // namespace tl2cgen::compiler::detail::codegen
