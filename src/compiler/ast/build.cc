/*!
 * Copyright (c) 2024 by Contributors
 * \file build.cc
 * \brief Initially build an Abstract Syntax Tree (AST) from a tree model
 * \author Hyunsu Cho
 */

#include <tl2cgen/detail/compiler/ast/ast.h>
#include <tl2cgen/detail/compiler/ast/builder.h>
#include <treelite/tree.h>

#include <optional>
#include <type_traits>
#include <variant>

namespace tl2cgen::compiler::detail::ast {

void ASTBuilder::BuildAST(treelite::Model const& model) {
  main_node_ = AddNode<MainNode>(nullptr, model.base_scores.AsVector());
  meta_.num_target_ = model.num_target;
  meta_.num_class_ = model.num_class.AsVector();
  meta_.leaf_vector_shape_ = {model.leaf_vector_shape[0], model.leaf_vector_shape[1]};
  meta_.num_feature_ = model.num_feature;

  ASTNode* func = AddNode<FunctionNode>(main_node_);
  main_node_->children_.push_back(func);
  std::visit(
      [&](auto&& model_preset) {
        for (std::size_t tree_id = 0; tree_id < model_preset.trees.size(); ++tree_id) {
          ASTNode* tree_head = BuildASTFromTree(func, model_preset.trees[tree_id],
              static_cast<int>(tree_id), model.target_id[tree_id], model.class_id[tree_id], 0);
          func->children_.push_back(tree_head);
        }
        using ModelPresetT = std::remove_const_t<std::remove_reference_t<decltype(model_preset)>>;
        meta_.type_meta_ = ModelMeta::TypeMeta<typename ModelPresetT::threshold_type,
            typename ModelPresetT::leaf_output_type>{};
      },
      model.variant_);
}

/*!
 * \brief Generate AST from a single decision tree
 * \param parent The parent node to which the generated AST subtree is to be attached
 * \param tree Single decision tree
 * \param tree_id ID of the tree, starting from 0
 * \param target_id Output target for which the tree produces output. -1 indicates that the
 *                  tree produces output for all targets.
 * \param tree_id Class for which the tree produces output. -1 indicates that the tree produces
 *                output for all classes.
 * \param nid ID of the node to be first rendered. Set it to 0 to start the recursive invocation
 *            of the function, and the function will call itself with different nid.
 */
template <typename ThresholdType, typename LeafOutputType>
ASTNode* ASTBuilder::BuildASTFromTree(ASTNode* parent,
    treelite::Tree<ThresholdType, LeafOutputType> const& tree, int tree_id, std::int32_t target_id,
    std::int32_t class_id, int nid) {
  ASTNode* ast_node = nullptr;
  if (tree.IsLeaf(nid)) {
    if (meta_.leaf_vector_shape_[0] == 1 && meta_.leaf_vector_shape_[1] == 1) {
      ast_node = AddNode<OutputNode>(
          parent, target_id, class_id, std::vector<LeafOutputType>{tree.LeafValue(nid)});
    } else {
      ast_node = AddNode<OutputNode>(parent, target_id, class_id, tree.LeafVector(nid));
    }
  } else {
    if (tree.NodeType(nid) == treelite::TreeNodeType::kNumericalTestNode) {
      ast_node = AddNode<NumericalConditionNode>(parent, tree.SplitIndex(nid),
          tree.DefaultLeft(nid), tree.ComparisonOp(nid), tree.Threshold(nid), std::nullopt);
    } else {
      ast_node = AddNode<CategoricalConditionNode>(parent, tree.SplitIndex(nid),
          tree.DefaultLeft(nid), tree.CategoryList(nid), tree.CategoryListRightChild(nid));
    }
    if (tree.HasGain(nid)) {
      dynamic_cast<ConditionNode*>(ast_node)->gain_ = tree.Gain(nid);
    }
    ast_node->children_.push_back(
        BuildASTFromTree(ast_node, tree, tree_id, target_id, class_id, tree.LeftChild(nid)));
    ast_node->children_.push_back(
        BuildASTFromTree(ast_node, tree, tree_id, target_id, class_id, tree.RightChild(nid)));
  }
  ast_node->node_id_ = nid;
  ast_node->tree_id_ = tree_id;
  if (tree.HasDataCount(nid)) {
    ast_node->data_count_ = tree.DataCount(nid);
  }
  if (tree.HasSumHess(nid)) {
    ast_node->sum_hess_ = tree.SumHess(nid);
  }

  return ast_node;
}

template ASTNode* ASTBuilder::BuildASTFromTree(
    ASTNode*, treelite::Tree<float, float> const&, int, std::int32_t, std::int32_t, int);
template ASTNode* ASTBuilder::BuildASTFromTree(
    ASTNode*, treelite::Tree<double, double> const&, int, std::int32_t, std::int32_t, int);

}  // namespace tl2cgen::compiler::detail::ast
