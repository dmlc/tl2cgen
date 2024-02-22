/*!
 * Copyright (c) 2024 by Contributors
 * \file load_data_counts.cc
 * \brief AST manipulation logic to load data counts
 * \author Hyunsu Cho
 */
#include <tl2cgen/detail/compiler/ast/builder.h>

#include <cstdint>

namespace {

namespace ast = tl2cgen::compiler::detail::ast;

void LoadDataCountsFromNode(
    ast::ASTNode* node, std::vector<std::vector<std::uint64_t>> const& counts) {
  if (node->tree_id_ >= 0 && node->node_id_ >= 0) {
    node->data_count_ = counts[node->tree_id_][node->node_id_];
  }
  for (ast::ASTNode* child : node->children_) {
    LoadDataCountsFromNode(child, counts);
  }
}

}  // anonymous namespace

namespace tl2cgen::compiler::detail::ast {

void ASTBuilder::LoadDataCounts(std::vector<std::vector<std::uint64_t>> const& counts) {
  LoadDataCountsFromNode(main_node_, counts);
}

}  // namespace tl2cgen::compiler::detail::ast
