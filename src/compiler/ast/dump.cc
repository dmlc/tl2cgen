/*!
 * Copyright (c) 2024 by Contributors
 * \file dump.cc
 * \brief Get a human-readable text representation of Abstract Syntax Tree (AST) for debugging
 *        and inspection.
 * \author Hyunsu Cho
 */

#include <fmt/format.h>
#include <tl2cgen/detail/compiler/ast/builder.h>
#include <tl2cgen/logging.h>

#include <sstream>
#include <string>
#include <type_traits>

namespace {

using tl2cgen::compiler::detail::ast::ASTNode;

void GetDumpFromNode(std::ostream& os, ASTNode const* node, int indent) {
  os << std::string(indent, ' ') << node->GetDump() << "\n";
  for (ASTNode const* child : node->children_) {
    TL2CGEN_CHECK(child);
    GetDumpFromNode(os, child, indent + 2);
  }
}

template <typename T>
std::string GetDumpFromType() {
  if constexpr (std::is_same_v<T, float>) {
    return "float32";
  } else if constexpr (std::is_same_v<T, double>) {
    return "float64";
  } else {
    return fmt::format("<unknown-type {}>", typeid(T).name());
  }
}

}  // anonymous namespace

namespace tl2cgen::compiler::detail::ast {

std::string ASTBuilder::GetDump() const {
  std::ostringstream oss;
  GetDumpFromNode(oss, main_node_, 0);
  oss << "Metadata: \n";
  oss << "is_categorical_ = [";
  for (bool e : meta_.is_categorical_) {
    oss << (e ? "true" : "false") << ", ";
  }
  oss << "], leaf_vector_shape_ = [" << meta_.leaf_vector_shape_[0] << ", "
      << meta_.leaf_vector_shape_[1] << "], num_feature_ = " << meta_.num_feature_;
  return oss.str();
}

std::string MainNode::GetDump() const {
  std::ostringstream oss;
  oss << "[";
  for (double e : base_scores_) {
    oss << e << ", ";
  }
  oss << "]";
  return fmt::format("MainNode {{ base_scores: float64{} }}", oss.str());
}

std::string TranslationUnitNode::GetDump() const {
  return fmt::format("TranslationUnitNode {{ unit_id: {} }}", unit_id_);
}

std::string QuantizerNode::GetDump() const {
  return std::visit(
      [](auto&& threshold_list_concrete) {
        using ThresholdListT
            = std::remove_const_t<std::remove_reference_t<decltype(threshold_list_concrete)>>;
        using ThresholdT = typename ThresholdListT::value_type::value_type;
        std::ostringstream oss;
        oss << "[ ";
        for (auto const& vec : threshold_list_concrete) {
          oss << "[ ";
          for (auto const& e : vec) {
            oss << e << ", ";
          }
          oss << "], ";
        }
        oss << "]";
        return fmt::format(
            "QuantizerNode {{ threshold_list: {}{} }}", GetDumpFromType<ThresholdT>(), oss.str());
      },
      threshold_list_);
}

std::string FunctionNode::GetDump() const {
  return fmt::format("FunctionNode {{}}");
}

std::string ConditionNode::GetDump() const {
  if (gain_) {
    return fmt::format("ConditionNode {{ split_index: {}, default_left: {}, gain: {} }}",
        split_index_, default_left_, *gain_);
  } else {
    return fmt::format(
        "ConditionNode {{ split_index: {}, default_left: {} }}", split_index_, default_left_);
  }
}

std::string NumericalConditionNode::GetDump() const {
  std::string threshold_str = std::visit(
      [](auto&& e) {
        using ThresholdT = std::remove_const_t<std::remove_reference_t<decltype(e)>>;
        return fmt::format("{}({})", GetDumpFromType<ThresholdT>(), e);
      },
      threshold_);
  return fmt::format(
      "NumericalConditionNode {{ {}, op: {}, threshold: {}, {}"
      "zero_quantized: {} }}",
      ConditionNode::GetDump(), treelite::OperatorToString(op_), threshold_str,
      (quantized_threshold_ ? fmt::format("quantized_threshold_: int({}), ", *quantized_threshold_)
                            : std::string("")),
      zero_quantized_);
}

std::string CategoricalConditionNode::GetDump() const {
  std::ostringstream oss;
  oss << "[";
  for (auto const& e : category_list_) {
    oss << e << ", ";
  }
  oss << "]";
  return fmt::format(
      "CategoricalConditionNode {{ {}, category_list: {}, "
      "category_list_right_child: {} }}",
      ConditionNode::GetDump(), oss.str(), category_list_right_child_);
}

std::string OutputNode::GetDump() const {
  return std::visit(
      [this](auto&& leaf_output_concrete) {
        using OutputT
            = std::remove_const_t<std::remove_reference_t<decltype(leaf_output_concrete)>>;
        using OutputElemT = typename OutputT::value_type;
        std::ostringstream oss;
        oss << "[";
        for (auto const& e : leaf_output_concrete) {
          oss << e << ", ";
        }
        oss << "]";
        return fmt::format("OutputNode {{ target_id: {}, class_id: {}, output: {}{} }}", target_id_,
            class_id_, GetDumpFromType<OutputElemT>(), oss.str());
      },
      leaf_output_);
}

}  // namespace tl2cgen::compiler::detail::ast
