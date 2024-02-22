/*!
 * Copyright (c) 2024 by Contributors
 * \file is_categorical_array.cc
 * \brief AST manipulation logic to determine whether each feature is categorical or not
 * \author Hyunsu Cho
 */
#include <tl2cgen/detail/compiler/ast/builder.h>
#include <tl2cgen/logging.h>

#include <cstdint>
#include <vector>

namespace {

namespace ast = tl2cgen::compiler::detail::ast;

void ScanThresholds(ast::ASTNode* node, std::vector<bool>& is_categorical) {
  auto* cat_cond = dynamic_cast<ast::CategoricalConditionNode*>(node);
  if (cat_cond) {
    is_categorical[cat_cond->split_index_] = true;
  }
  for (ast::ASTNode* child : node->children_) {
    ScanThresholds(child, is_categorical);
  }
}

}  // anonymous namespace

namespace tl2cgen::compiler::detail::ast {

void ASTBuilder::GenerateIsCategoricalArray() {
  std::vector<bool> is_categorical(meta_.num_feature_, false);
  ScanThresholds(main_node_, is_categorical);
  meta_.is_categorical_ = std::move(is_categorical);
}

}  // namespace tl2cgen::compiler::detail::ast
