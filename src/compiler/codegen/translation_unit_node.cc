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

namespace tl2cgen::compiler::detail::codegen {

void HandleTranslationUnitNode(ast::TranslationUnitNode const* node, CodeCollection& gencode) {}

}  // namespace tl2cgen::compiler::detail::codegen
