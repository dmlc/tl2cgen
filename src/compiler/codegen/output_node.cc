/*!
 * Copyright (c) 2024 by Contributors
 * \file output_node.cc
 * \brief Convert OutputNode in AST into C code
 * \author Hyunsu Cho
 */

#include <fmt/format.h>
#include <tl2cgen/detail/compiler/ast/ast.h>
#include <tl2cgen/detail/compiler/codegen/codegen.h>
#include <tl2cgen/logging.h>

namespace tl2cgen::compiler::detail::codegen {

void HandleOutputNode(ast::OutputNode const* node, CodeCollection& gencode) {
  TL2CGEN_CHECK_EQ(node->children_.size(), 0);
  /// gencode.PushFragment(RenderOutputStatement(node));
}

}  // namespace tl2cgen::compiler::detail::codegen
