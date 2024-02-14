/*!
 * Copyright (c) 2024 by Contributors
 * \file quantizer_node.cc
 * \brief Convert QuantizerNode in AST into C code
 * \author Hyunsu Cho
 */

#include <fmt/format.h>
#include <tl2cgen/detail/compiler/ast/ast.h>
#include <tl2cgen/detail/compiler/codegen/codegen.h>
#include <tl2cgen/detail/compiler/codegen/format_util.h>
#include <tl2cgen/logging.h>

using namespace fmt::literals;

namespace {

char const* const quantize_function_signature_template
    = "int quantize({threshold_type} val, unsigned fid)";

char const* const quantize_function_template =
    R"TL2CGENTEMPLATE(
/*
 * \brief Function to convert a feature value into bin index.
 * \param val Feature value, in floating-point
 * \param fid Feature identifier
 * \return bin Index corresponding to given feature value
 */
{quantize_function_signature} {{
  const size_t offset = th_begin[fid];
  const {threshold_type}* array = &threshold[offset];
  int len = th_len[fid];
  int low = 0;
  int high = len;
  int mid;
  {threshold_type} mval;
  // It is possible th_begin[i] == [total_num_threshold]. This means that
  // all features i, (i+1), ... are not used for any of the splits in the model.
  // So in this case, just return something
  if (offset == {total_num_threshold} || val < array[0]) {{
    return -10;
  }}
  while (low + 1 < high) {{
    mid = (low + high) / 2;
    mval = array[mid];
    if (val == mval) {{
      return mid * 2;
    }} else if (val < mval) {{
      high = mid;
    }} else {{
      low = mid;
    }}
  }}
  if (array[low] == val) {{
    return low * 2;
  }} else if (high == len) {{
    return len * 2;
  }} else {{
    return low * 2 + 1;
  }}
}}
)TL2CGENTEMPLATE";

char const* const quantize_loop_template =
    R"TL2CGENTEMPLATE(
// Quantize data
for (int i = 0; i < {num_feature}; ++i) {{
  if (data[i].missing != -1 && !is_categorical[i]) {{
    data[i].qvalue = quantize(data[i].fvalue, i);
  }}
}}

)TL2CGENTEMPLATE";

char const* const quantize_arrays_template =
    R"TL2CGENTEMPLATE(
#include "header.h"

static const {threshold_type} threshold[] = {{
{array_threshold}
}};

static const int th_begin[] = {{
{array_th_begin}
}};

static const int th_len[] = {{
{array_th_len}
}};
)TL2CGENTEMPLATE";

}  // anonymous namespace

namespace tl2cgen::compiler::detail::codegen {

void HandleQuantizerNode(ast::QuantizerNode const* node, CodeCollection& gencode) {
  auto threshold_ctype_str = GetThresholdCType(node);
  /* Render arrays needed to convert feature values into bin indices */
  std::string array_threshold, array_th_begin, array_th_len;
  // threshold[] : List of all thresholds that occur at least once in the
  //   ensemble model. For each feature, an ascending list of unique
  //   thresholds is generated. The range th_begin[i]:(th_begin[i]+th_len[i])
  //   of the threshold[] array stores the threshold list for feature i.
  std::size_t total_num_threshold;
  // to hold total number of (distinct) thresholds
  {
    ArrayFormatter formatter(80, 2);
    std::visit(
        [&](auto&& threshold_list_concrete) {
          for (auto const& e : threshold_list_concrete) {
            // threshold_list had been generated in ASTBuilder::QuantizeThresholds
            // threshold_list[i][k] stores the k-th threshold of feature i.
            for (auto v : e) {
              formatter << v;
            }
          }
        },
        node->threshold_list_);
    array_threshold = formatter.str();
  }
  {
    ArrayFormatter formatter(80, 2);
    std::size_t accum = 0;  // used to compute cumulative sum over threshold counts
    std::visit(
        [&](auto&& threshold_list_concrete) {
          for (auto const& e : threshold_list_concrete) {
            formatter << accum;
            accum += e.size();  // e.size() = number of thresholds for each feature
          }
        },
        node->threshold_list_);
    total_num_threshold = accum;
    array_th_begin = formatter.str();
  }
  {
    ArrayFormatter formatter(80, 2);
    std::visit(
        [&](auto&& threshold_list_concrete) {
          for (auto const& e : threshold_list_concrete) {
            formatter << e.size();
          }
        },
        node->threshold_list_);
    array_th_len = formatter.str();
  }
  auto current_file = gencode.GetCurrentSourceFile();
  if (!array_threshold.empty() && !array_th_begin.empty() && !array_th_len.empty()) {
    gencode.PushFragment(
        fmt::format(quantize_loop_template, "num_feature"_a = node->meta_->num_feature_));

    gencode.SwitchToSourceFile("header.h");
    std::string const quantize_function_signature = fmt::format(
        quantize_function_signature_template, "threshold_type"_a = threshold_ctype_str);
    gencode.PushFragment(fmt::format("{};", quantize_function_signature));

    gencode.SwitchToSourceFile("quantize.c");
    gencode.PushFragment(fmt::format(quantize_arrays_template,
        "array_threshold"_a = array_threshold, "threshold_type"_a = threshold_ctype_str,
        "array_th_begin"_a = array_th_begin, "array_th_len"_a = array_th_len));
    gencode.PushFragment(fmt::format(quantize_function_template,
        "quantize_function_signature"_a = quantize_function_signature,
        "total_num_threshold"_a = total_num_threshold, "threshold_type"_a = threshold_ctype_str));
    gencode.SwitchToSourceFile(current_file);  // Switch back context
  }
  TL2CGEN_CHECK_EQ(node->children_.size(), 1);
  GenerateCodeFromAST(node->children_[0], gencode);
}

}  // namespace tl2cgen::compiler::detail::codegen
