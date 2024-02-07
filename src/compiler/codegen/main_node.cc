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

#include <cstddef>
#include <variant>

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
  for (std::size_t fid = 0; fid < is_categorical.size(); ++fid) {
    formatter << (is_categorical[fid] ? 1 : 0);
  }
  return fmt::format("const unsigned char is_categorical[] = {{{}}};", formatter.str());
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

union Entry {{
  int missing;
  {threshold_type} fvalue;
  int qvalue;
}};

{extern_array_is_categorical}

{dllexport}void predict(union Entry* data, int pred_margin, {threshold_type}* result);
)TL2CGENTEMPLATE";

char const* const main_start_template =
    R"TL2CGENTEMPLATE(
#include "header.h"

{array_is_categorical}

void predict(union Entry* data, int pred_margin, {threshold_type}* result) {{
)TL2CGENTEMPLATE";

void HandleMainNode(ast::MainNode const* node, CodeCollection& gencode) {
  auto threshold_ctype_str = GetThresholdCType(node);
  gencode.SwitchToSourceFile("header.h");
  gencode.PushFragment(fmt::format(header_template, "threshold_type"_a = threshold_ctype_str,
      "dllexport"_a = DLLEXPORT_KEYWORD,
      "extern_array_is_categorical"_a
      = (!node->meta_->is_categorical_.empty() ? "extern const unsigned char is_categorical[];"
                                               : "")));

  gencode.SwitchToSourceFile("main.c");
  gencode.PushFragment(fmt::format(main_start_template,
      "array_is_categorical"_a = RenderIsCategoricalArray(node->meta_->is_categorical_),
      "threshold_type"_a = threshold_ctype_str));
  gencode.ChangeIndent(1);
  TL2CGEN_CHECK_EQ(node->children_.size(), 1);
  GenerateCodeFromAST(node->children_[0], gencode);
  gencode.ChangeIndent(-1);
  gencode.PushFragment("}");
}

}  // namespace tl2cgen::compiler::detail::codegen
