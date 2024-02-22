/*!
 * Copyright (c) 2024 by Contributors
 * \file translation_unit_node.cc
 * \brief Convert TranslationUnitNode in AST into C code
 * \author Hyunsu Cho
 */

#include <fmt/format.h>
#include <tl2cgen/detail/compiler/ast/ast.h>
#include <tl2cgen/detail/compiler/codegen/codegen.h>
#include <tl2cgen/logging.h>

using namespace fmt::literals;

namespace {

char const* const unit_function_name_template = "predict_unit{unit_id}";
char const* const unit_function_signature_template
    = "void {unit_function_name}(union Entry* data, {leaf_output_type}* result)";
char const* const unit_source_start_template =
    R"TL2CGENTEMPLATE(
#include "header.h"

{unit_function_signature} {{
)TL2CGENTEMPLATE";

}  // anonymous namespace

namespace tl2cgen::compiler::detail::codegen {

void HandleTranslationUnitNode(ast::TranslationUnitNode const* node, CodeCollection& gencode) {
  TL2CGEN_CHECK_EQ(node->children_.size(), 1);
  auto leaf_output_ctype_str = GetLeafOutputCType(node);
  std::string const unit_function_name
      = fmt::format(unit_function_name_template, "unit_id"_a = node->unit_id_);
  std::string const unit_function_signature = fmt::format(unit_function_signature_template,
      "unit_function_name"_a = unit_function_name, "leaf_output_type"_a = leaf_output_ctype_str);

  auto current_file = gencode.GetCurrentSourceFile();
  gencode.PushFragment(fmt::format(
      "{unit_function_name}(data, result);", "unit_function_name"_a = unit_function_name));
  gencode.SwitchToSourceFile("header.h");
  gencode.PushFragment(fmt::format("{};", unit_function_signature));
  gencode.SwitchToSourceFile(fmt::format("tu{unit_id}.c", "unit_id"_a = node->unit_id_));
  gencode.PushFragment(fmt::format(
      unit_source_start_template, "unit_function_signature"_a = unit_function_signature));
  gencode.ChangeIndent(1);
  GenerateCodeFromAST(node->children_[0], gencode);
  gencode.ChangeIndent(-1);
  gencode.PushFragment("}");
  gencode.SwitchToSourceFile(current_file);  // Switch back context
}

}  // namespace tl2cgen::compiler::detail::codegen
