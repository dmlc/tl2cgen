/*!
 * Copyright (c) 2024 by Contributors
 * \file main_node.cc
 * \brief Convert MainNode in AST into C code
 * \author Hyunsu Cho
 */

#include <fmt/format.h>
#include <tl2cgen/detail/compiler/ast/ast.h>
#include <tl2cgen/detail/compiler/codegen/codegen.h>
#include <tl2cgen/detail/compiler/codegen/format_util.h>
#include <tl2cgen/logging.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

using namespace fmt::literals;

#if defined(_MSC_VER) || defined(_WIN32)
#define DLLEXPORT_KEYWORD "__declspec(dllexport) "
#else
#define DLLEXPORT_KEYWORD ""
#endif

namespace {

std::string RenderIsCategoricalArray(std::vector<bool> const& is_categorical) {
  if (is_categorical.empty()) {
    return "";
  }
  tl2cgen::compiler::detail::codegen::ArrayFormatter formatter(80, 2);
  for (bool e : is_categorical) {
    formatter << (e ? 1 : 0);
  }
  return fmt::format("const unsigned char is_categorical[] = {{{}}};", formatter.str());
}

std::string RenderNumClassArray(std::vector<std::int32_t> const& num_class) {
  tl2cgen::compiler::detail::codegen::ArrayFormatter formatter(80, 2);
  for (std::int32_t e : num_class) {
    formatter << e;
  }
  return fmt::format("static const int32_t num_class[] = {{{}}};", formatter.str());
}

}  // anonymous namespace

namespace tl2cgen::compiler::detail::codegen {

char const* const header_template =
    R"TL2CGENTEMPLATE(
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <stdint.h>

#if defined(__clang__) || defined(__GNUC__)
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

#define N_TARGET {num_target}
#define MAX_N_CLASS {max_num_class}

union Entry {{
  int missing;
  {threshold_ctype} fvalue;
  int qvalue;
}};

{dllexport}int32_t get_num_target(void);
{dllexport}void get_num_class(int32_t* out);
{dllexport}int32_t get_num_feature(void);
{dllexport}const char* get_threshold_type(void);
{dllexport}const char* get_leaf_output_type(void);
{dllexport}void predict(union Entry* data, int pred_margin, {leaf_output_ctype}* result);
void postprocess({leaf_output_ctype}* result);
)TL2CGENTEMPLATE";

char const* const main_start_template =
    R"TL2CGENTEMPLATE(
#include "header.h"

{array_is_categorical}
{array_num_class}

int32_t get_num_target(void) {{
  return N_TARGET;
}}
void get_num_class(int32_t* out) {{
  for (int i = 0; i < N_TARGET; ++i) {{
    out[i] = num_class[i];
  }}
}}
int32_t get_num_feature(void) {{
  return {num_feature};
}}
const char* get_threshold_type(void) {{
  return "{threshold_type}";
}}
const char* get_leaf_output_type(void) {{
  return "{leaf_output_type}";
}}

void predict(union Entry* data, int pred_margin, {leaf_output_ctype}* result) {{
)TL2CGENTEMPLATE";

void HandleMainNode(ast::MainNode const* node, CodeCollection& gencode) {
  auto const threshold_ctype_str = GetThresholdCType(node);
  auto const leaf_output_ctype_str = GetLeafOutputCType(node);
  std::int32_t const num_target = node->meta_->num_target_;
  std::vector<std::int32_t>& num_class = node->meta_->num_class_;
  std::int32_t const max_num_class = *std::max_element(num_class.begin(), num_class.end());

  gencode.SwitchToSourceFile("header.h");
  gencode.PushFragment(fmt::format(header_template, "threshold_ctype"_a = threshold_ctype_str,
      "leaf_output_ctype"_a = leaf_output_ctype_str, "dllexport"_a = DLLEXPORT_KEYWORD,
      "num_target"_a = num_target, "max_num_class"_a = max_num_class));

  gencode.SwitchToSourceFile("main.c");
  gencode.PushFragment(fmt::format(main_start_template,
      "array_is_categorical"_a = RenderIsCategoricalArray(node->meta_->is_categorical_),
      "array_num_class"_a = RenderNumClassArray(node->meta_->num_class_),
      "num_feature"_a = node->meta_->num_feature_, "threshold_type"_a = GetThresholdTypeStr(node),
      "leaf_output_type"_a = GetLeafOutputTypeStr(node),
      "leaf_output_ctype"_a = leaf_output_ctype_str));
  gencode.ChangeIndent(1);
  TL2CGEN_CHECK_EQ(node->children_.size(), 1);
  GenerateCodeFromAST(node->children_[0], gencode);

  // Tree averaging
  if (node->average_factor_) {
    gencode.PushFragment("\n// Average tree outputs");
    std::vector<std::int32_t> const& average_factor = node->average_factor_.value();
    for (std::int32_t target_id = 0; target_id < num_target; ++target_id) {
      for (std::int32_t class_id = 0; class_id < num_class[target_id]; ++class_id) {
        gencode.PushFragment(fmt::format("result[{offset}] /= {average_factor};",
            "offset"_a = target_id * max_num_class + class_id,
            "average_factor"_a = average_factor[class_id]));
      }
    }
  }

  // Apply base_scores
  gencode.PushFragment("\n// Apply base_scores");
  for (std::int32_t target_id = 0; target_id < num_target; ++target_id) {
    for (std::int32_t class_id = 0; class_id < num_class[target_id]; ++class_id) {
      gencode.PushFragment(fmt::format("result[{offset}] += {base_score};",
          "offset"_a = target_id * max_num_class + class_id,
          "base_score"_a = ToStringHighPrecision(node->base_scores_[class_id])));
    }
  }

  // Apply postprocessor
  gencode.PushFragment(
      "\n// Apply postprocessor"
      "\nif (!pred_margin) { postprocess(result); }");
  gencode.ChangeIndent(-1);
  gencode.PushFragment("}");
  gencode.PushFragment(GetPostprocessorFunc(*node->meta_, node->postprocessor_));
}

}  // namespace tl2cgen::compiler::detail::codegen
