/*!
 * Copyright (c) 2023 by Contributors
 * \file load_data_counts.cc
 * \brief AST manipulation logic to load data counts
 * \author Hyunsu Cho
 */
#include <tl2cgen/detail/compiler/ast/builder.h>

#include <cstdint>

namespace tl2cgen::compiler::detail::ast {

static void load_data_counts(ASTNode* node, std::vector<std::vector<std::uint64_t>> const& counts) {
  if (node->tree_id >= 0 && node->node_id >= 0) {
    node->data_count = counts[node->tree_id][node->node_id];
  }
  for (ASTNode* child : node->children) {
    load_data_counts(child, counts);
  }
}

template <typename ThresholdType, typename LeafOutputType>
void ASTBuilder<ThresholdType, LeafOutputType>::LoadDataCounts(
    std::vector<std::vector<std::uint64_t>> const& counts) {
  load_data_counts(this->main_node, counts);
}

template void ASTBuilder<float, std::uint32_t>::LoadDataCounts(
    std::vector<std::vector<std::uint64_t>> const&);
template void ASTBuilder<float, float>::LoadDataCounts(
    std::vector<std::vector<std::uint64_t>> const&);
template void ASTBuilder<double, std::uint32_t>::LoadDataCounts(
    std::vector<std::vector<std::uint64_t>> const&);
template void ASTBuilder<double, double>::LoadDataCounts(
    std::vector<std::vector<std::uint64_t>> const&);

}  // namespace tl2cgen::compiler::detail::ast
