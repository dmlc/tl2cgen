/*!
 * Copyright (c) 2023 by Contributors
 * \file build.cc
 * \brief Build AST from a given model
 * \author Hyunsu Cho
 */
#include <tl2cgen/detail/compiler/ast/builder.h>
#include <treelite/tree.h>

#include <cstddef>
#include <cstdint>

namespace tl2cgen::compiler::detail::ast {

template <typename ThresholdType, typename LeafOutputType>
void ASTBuilder<ThresholdType, LeafOutputType>::BuildAST(
    treelite::ModelImpl<ThresholdType, LeafOutputType> const& model) {
  this->output_vector_flag = (model.task_param.leaf_vector_size > 1);
  this->num_feature = model.num_feature;
  this->average_output_flag = model.average_tree_output;

  this->main_node = AddNode<MainNode>(nullptr, model.param.global_bias, model.average_tree_output,
      static_cast<int>(model.trees.size()), model.num_feature);
  ASTNode* ac = AddNode<AccumulatorContextNode>(this->main_node);
  this->main_node->children.push_back(ac);
  for (std::size_t tree_id = 0; tree_id < model.trees.size(); ++tree_id) {
    ASTNode* tree_head = BuildASTFromTree(model.trees[tree_id], static_cast<int>(tree_id), 0, ac);
    ac->children.push_back(tree_head);
  }
  this->model_param = model.param.__DICT__();
}

template <typename ThresholdType, typename LeafOutputType>
ASTNode* ASTBuilder<ThresholdType, LeafOutputType>::BuildASTFromTree(
    treelite::Tree<ThresholdType, LeafOutputType> const& tree, int tree_id, int nid,
    ASTNode* parent) {
  ASTNode* ast_node = nullptr;
  if (tree.IsLeaf(nid)) {
    if (this->output_vector_flag) {
      ast_node = AddNode<OutputNode<LeafOutputType>>(parent, tree.LeafVector(nid));
    } else {
      ast_node = AddNode<OutputNode<LeafOutputType>>(parent, tree.LeafValue(nid));
    }
  } else {
    if (tree.SplitType(nid) == treelite::SplitFeatureType::kNumerical) {
      ast_node = AddNode<NumericalConditionNode<ThresholdType>>(parent, tree.SplitIndex(nid),
          tree.DefaultLeft(nid), false, tree.ComparisonOp(nid),
          ThresholdVariant<ThresholdType>(tree.Threshold(nid)));
    } else {
      ast_node = AddNode<CategoricalConditionNode>(parent, tree.SplitIndex(nid),
          tree.DefaultLeft(nid), tree.MatchingCategories(nid), tree.CategoriesListRightChild(nid));
    }
    if (tree.HasGain(nid)) {
      dynamic_cast<ConditionNode*>(ast_node)->gain = tree.Gain(nid);
    }
    ast_node->children.push_back(BuildASTFromTree(tree, tree_id, tree.LeftChild(nid), ast_node));
    ast_node->children.push_back(BuildASTFromTree(tree, tree_id, tree.RightChild(nid), ast_node));
  }
  ast_node->node_id = nid;
  ast_node->tree_id = tree_id;
  if (tree.HasDataCount(nid)) {
    ast_node->data_count = tree.DataCount(nid);
  }
  if (tree.HasSumHess(nid)) {
    ast_node->sum_hess = tree.SumHess(nid);
  }

  return ast_node;
}

template void ASTBuilder<float, std::uint32_t>::BuildAST(
    treelite::ModelImpl<float, std::uint32_t> const&);
template void ASTBuilder<float, float>::BuildAST(treelite::ModelImpl<float, float> const&);
template void ASTBuilder<double, std::uint32_t>::BuildAST(
    treelite::ModelImpl<double, std::uint32_t> const&);
template void ASTBuilder<double, double>::BuildAST(treelite::ModelImpl<double, double> const&);
template ASTNode* ASTBuilder<float, std::uint32_t>::BuildASTFromTree(
    treelite::Tree<float, std::uint32_t> const&, int, int, ASTNode*);
template ASTNode* ASTBuilder<float, float>::BuildASTFromTree(
    treelite::Tree<float, float> const&, int, int, ASTNode*);
template ASTNode* ASTBuilder<double, std::uint32_t>::BuildASTFromTree(
    treelite::Tree<double, std::uint32_t> const&, int, int, ASTNode*);
template ASTNode* ASTBuilder<double, double>::BuildASTFromTree(
    treelite::Tree<double, double> const&, int, int, ASTNode*);

}  // namespace tl2cgen::compiler::detail::ast
