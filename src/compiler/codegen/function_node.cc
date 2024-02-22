/*!
 * Copyright (c) 2024 by Contributors
 * \file function_node.cc
 * \brief Convert FunctionNode in AST into C code
 * \author Hyunsu Cho
 */

#include <fmt/format.h>
#include <tl2cgen/detail/compiler/ast/ast.h>
#include <tl2cgen/detail/compiler/codegen/codegen.h>
#include <tl2cgen/logging.h>

using namespace fmt::literals;

namespace tl2cgen::compiler::detail::codegen {

void HandleFunctionNode(ast::FunctionNode const* node, CodeCollection& gencode) {
  gencode.PushFragment("unsigned int tmp;");
  for (ast::ASTNode* child : node->children_) {
    GenerateCodeFromAST(child, gencode);
  }
}

}  // namespace tl2cgen::compiler::detail::codegen
