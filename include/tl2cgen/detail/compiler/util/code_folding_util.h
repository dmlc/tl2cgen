/*!
 * Copyright (c) 2023 by Contributors
 * \file code_folding_util.h
 * \author Hyunsu Cho
 * \brief Utilities for code folding
 */
#ifndef TL2CGEN_DETAIL_COMPILER_UTIL_CODE_FOLDING_UTIL_H_
#define TL2CGEN_DETAIL_COMPILER_UTIL_CODE_FOLDING_UTIL_H_

#include <fmt/format.h>
#include <tl2cgen/detail/compiler/ast/ast.h>
#include <tl2cgen/detail/compiler/util/categorical_bitmap.h>
#include <tl2cgen/detail/compiler/util/format_util.h>
#include <tl2cgen/logging.h>
#include <treelite/tree.h>

#include <cstddef>
#include <cstdint>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

using namespace fmt::literals;

namespace tl2cgen::compiler::detail::util {

template <typename ThresholdType, typename LeafOutputType, typename OutputFormatFunc>
inline void RenderCodeFolderArrays(ast::CodeFolderNode const* node, bool quantize,
    bool use_boolean_literal, char const* node_entry_template,
    OutputFormatFunc RenderOutputStatement, std::string* array_nodes, std::string* array_cat_bitmap,
    std::string* array_cat_begin, std::string* output_switch_statements,
    treelite::Operator* common_comp_op) {
  TL2CGEN_CHECK_EQ(node->children.size(), 1);
  int const tree_id = node->children[0]->tree_id;
  // list of descendants, with newly assigned ID's
  std::unordered_map<ast::ASTNode*, int> descendants;
  // list of all OutputNode's among the descendants
  std::vector<ast::OutputNode<LeafOutputType>*> output_nodes;
  // two arrays used to store categorical split info
  std::vector<std::uint64_t> cat_bitmap;
  std::vector<std::size_t> cat_begin{0};

  // 1. Assign new continuous node ID's (0, 1, 2, ...) by traversing the
  // subtree breadth-first
  {
    std::queue<ast::ASTNode*> Q;
    std::set<treelite::Operator> ops;
    int new_node_id = 0;
    int new_leaf_id = -1;
    Q.push(node->children[0]);
    while (!Q.empty()) {
      ast::ASTNode* e = Q.front();
      Q.pop();
      // sanity check: all descendants must have same tree_id
      TL2CGEN_CHECK_EQ(e->tree_id, tree_id);
      // sanity check: all descendants must be ConditionNode or OutputNode
      auto* t1 = dynamic_cast<ast::ConditionNode*>(e);
      auto* t2 = dynamic_cast<ast::OutputNode<LeafOutputType>*>(e);
      ast::NumericalConditionNode<ThresholdType>* t3;
      TL2CGEN_CHECK(t1 || t2);
      if (t2) {  // e is OutputNode
        descendants[e] = new_leaf_id--;
      } else {
        if ((t3 = dynamic_cast<ast::NumericalConditionNode<ThresholdType>*>(t1))) {
          ops.insert(t3->op);
        }
        descendants[e] = new_node_id++;
      }
      for (ast::ASTNode* child : e->children) {
        Q.push(child);
      }
    }
    // sanity check: all numerical splits must have identical comparison operators
    TL2CGEN_CHECK_LE(ops.size(), 1);
    *common_comp_op = ops.empty() ? treelite::Operator::kLT : *ops.begin();
  }

  // 2. Render node_treeXX_nodeXX[] by traversing the subtree once again.
  // Now we can use the re-assigned node ID's.
  {
    ArrayFormatter formatter(80, 2);

    bool default_left;
    std::string threshold;
    int left_child_id, right_child_id;
    unsigned int split_index;
    ast::OutputNode<LeafOutputType>* t1;
    ast::NumericalConditionNode<ThresholdType>* t2;
    ast::CategoricalConditionNode* t3;

    std::queue<ast::ASTNode*> Q;
    Q.push(node->children[0]);
    while (!Q.empty()) {
      ast::ASTNode* e = Q.front();
      Q.pop();
      if ((t1 = dynamic_cast<ast::OutputNode<LeafOutputType>*>(e))) {
        output_nodes.push_back(t1);
        // don't render OutputNode but save it for later
      } else {
        TL2CGEN_CHECK_EQ(e->children.size(), 2U);
        left_child_id = descendants[e->children[0]];
        right_child_id = descendants[e->children[1]];
        if ((t2 = dynamic_cast<ast::NumericalConditionNode<ThresholdType>*>(e))) {
          default_left = t2->default_left;
          split_index = t2->split_index;
          threshold = quantize ? std::to_string(t2->threshold.int_val)
                               : ToStringHighPrecision(t2->threshold.float_val);
        } else {
          TL2CGEN_CHECK((t3 = dynamic_cast<ast::CategoricalConditionNode*>(e)));
          default_left = t3->default_left;
          split_index = t3->split_index;
          threshold = "-1";  // dummy value
          std::vector<std::uint64_t> bitmap = GetCategoricalBitmap(t3->matching_categories);
          cat_bitmap.insert(cat_bitmap.end(), bitmap.begin(), bitmap.end());
          cat_begin.push_back(cat_bitmap.size());
        }
        char const* (*BoolWrapper)(bool);
        if (use_boolean_literal) {
          BoolWrapper = [](bool x) { return x ? "true" : "false"; };
        } else {
          BoolWrapper = [](bool x) { return x ? "1" : "0"; };
        }
        formatter << fmt::format(node_entry_template, "default_left"_a = BoolWrapper(default_left),
            "split_index"_a = split_index, "threshold"_a = threshold,
            "left_child"_a = left_child_id, "right_child"_a = right_child_id);
      }
      for (ast::ASTNode* child : e->children) {
        Q.push(child);
      }
    }
    *array_nodes = formatter.str();
  }
  // 3. render cat_bitmap_treeXX_nodeXX[] and cat_begin_treeXX_nodeXX[]
  if (cat_bitmap.empty()) {  // do not render empty arrays
    *array_cat_bitmap = "";
    *array_cat_begin = "";
  } else {
    {
      ArrayFormatter formatter(80, 2);
      for (std::uint64_t e : cat_bitmap) {
        formatter << fmt::format("{:#X}", e);
      }
      *array_cat_bitmap = formatter.str();
    }
    {
      ArrayFormatter formatter(80, 2);
      for (std::size_t e : cat_begin) {
        formatter << e;
      }
      *array_cat_begin = formatter.str();
    }
  }
  // 4. Render switch statement to associate each node ID with an output
  *output_switch_statements = "switch (nid) {\n";
  for (ast::OutputNode<LeafOutputType>* e : output_nodes) {
    int const node_id = descendants[static_cast<ast::ASTNode*>(e)];
    *output_switch_statements += fmt::format(
        " case {node_id}:\n"
        "{output_statement}"
        "  break;\n",
        "node_id"_a = node_id,
        "output_statement"_a = IndentMultiLineString(RenderOutputStatement(e), 2));
  }
  *output_switch_statements += "}\n";
}

}  // namespace tl2cgen::compiler::detail::util

#endif  // TL2CGEN_DETAIL_COMPILER_UTIL_CODE_FOLDING_UTIL_H_
