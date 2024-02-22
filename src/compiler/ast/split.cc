/*!
 * Copyright (c) 2024 by Contributors
 * \file split.cc
 * \brief Split prediction subroutine into multiple translation units (files)
 * \author Hyunsu Cho
 */
#include <tl2cgen/detail/compiler/ast/builder.h>
#include <tl2cgen/logging.h>

#include <cstdint>

namespace {

namespace ast = tl2cgen::compiler::detail::ast;

int CountTUNodes(ast::ASTNode* node) {
  int accum = (dynamic_cast<ast::TranslationUnitNode*>(node)) ? 1 : 0;
  for (ast::ASTNode* child : node->children_) {
    accum += CountTUNodes(child);
  }
  return accum;
}

}  // anonymous namespace

namespace tl2cgen::compiler::detail::ast {

void ASTBuilder::SplitIntoTUs(int num_tu) {
  if (num_tu <= 0) {
    TL2CGEN_LOG(INFO) << "Parallel compilation disabled; all member trees will be "
                      << "dumped to a single source file. This may increase "
                      << "compilation time and memory usage.";
    return;
  }
  TL2CGEN_LOG(INFO) << "Parallel compilation enabled; member trees will be "
                    << "divided into " << num_tu << " translation units.";
  TL2CGEN_CHECK_EQ(main_node_->children_.size(), 1);
  ASTNode* top_func_node = main_node_->children_[0];
  TL2CGEN_CHECK(dynamic_cast<FunctionNode*>(top_func_node));

  /* tree_head[i] stores reference to head of tree i */
  std::vector<ASTNode*> tree_head;
  for (ASTNode* node : top_func_node->children_) {
    TL2CGEN_CHECK(dynamic_cast<ConditionNode*>(node) || dynamic_cast<OutputNode*>(node));
    tree_head.push_back(node);
  }
  /* dynamic_cast<> is used here to check node types. This is to ensure
    that we don't accidentally call Split() twice. */

  int const ntree = static_cast<int>(tree_head.size());
  int const unit_size = (ntree + num_tu - 1) / num_tu;
  std::vector<ASTNode*> tu_list;  // list of translation units
  int const current_num_tu = CountTUNodes(main_node_);
  for (int unit_id = 0; unit_id < num_tu; ++unit_id) {
    int const tree_begin = unit_id * unit_size;
    int const tree_end = std::min((unit_id + 1) * unit_size, ntree);
    if (tree_begin < tree_end) {
      TranslationUnitNode* tu
          = AddNode<TranslationUnitNode>(top_func_node, current_num_tu + unit_id);
      tu_list.push_back(tu);
      FunctionNode* func = AddNode<FunctionNode>(tu);
      tu->children_.push_back(func);
      for (int tree_id = tree_begin; tree_id < tree_end; ++tree_id) {
        ASTNode* tree_head_node = tree_head[tree_id];
        tree_head_node->parent_ = func;
        func->children_.push_back(tree_head_node);
      }
    }
  }
  top_func_node->children_ = tu_list;
}

}  // namespace tl2cgen::compiler::detail::ast
