/*!
 * Copyright (c) 2017-2020 by Contributors
 * \file builder.h
 * \brief AST Builder class
 * \author Hyunsu Cho
 */
#ifndef TL2CGEN_DETAIL_COMPILER_AST_BUILDER_H_
#define TL2CGEN_DETAIL_COMPILER_AST_BUILDER_H_

#include <tl2cgen/detail/compiler/ast/ast.h>

#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace treelite {

class Model;

template <typename ThresholdType, typename LeafOutputType>
class Tree;

}  // namespace treelite

namespace tl2cgen::compiler::detail::ast {

class ASTBuilder {
 public:
  ASTBuilder() : main_node_(nullptr) {}

  /* \brief Initially build AST from model */
  void BuildAST(treelite::Model const& model);
  /* \brief Generate is_categorical[] array, which tells whether each feature
            is categorical or numerical */
  void GenerateIsCategoricalArray();
  /*
   * \brief Split prediction function into multiple translation units
   * \param num_tu Number of translation units
   */
  void SplitIntoTUs(int num_tu);
  /* \brief Replace split thresholds with integers */
  void QuantizeThresholds();
  /* \brief Load data counts from annotation file */
  void LoadDataCounts(std::vector<std::vector<std::uint64_t>> const& counts);
  /*
   * \brief Get a text representation of AST
   */
  std::string GetDump() const;

  inline ASTNode const* GetRootNode() {
    return main_node_;
  }

 private:
  template <typename NodeType, typename... Args>
  NodeType* AddNode(ASTNode* parent, Args&&... args) {
    std::unique_ptr<NodeType> node(new NodeType(std::forward<Args>(args)...));
    NodeType* ref = node.get();
    ref->parent_ = parent;
    ref->meta_ = &meta_;
    nodes_.push_back(std::move(node));
    return ref;
  }

  template <typename ThresholdType, typename LeafOutputType>
  ASTNode* BuildASTFromTree(ASTNode* parent,
      treelite::Tree<ThresholdType, LeafOutputType> const& tree, int tree_id,
      std::int32_t target_id, std::int32_t class_id, int nid);

  // Keep tract of all nodes built so far, to prevent memory leak
  std::vector<std::unique_ptr<ASTNode>> nodes_;
  ASTNode* main_node_;
  ast::ModelMeta meta_;
};

}  // namespace tl2cgen::compiler::detail::ast

#endif  // TL2CGEN_DETAIL_COMPILER_AST_BUILDER_H_
