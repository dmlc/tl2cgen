/*!
 * Copyright (c) 2024 by Contributors
 * \file quantize.cc
 * \brief Quantize thresholds in condition nodes
 * \author Hyunsu Cho
 */
#include <tl2cgen/detail/compiler/ast/builder.h>
#include <tl2cgen/detail/math_funcs.h>
#include <tl2cgen/logging.h>

#include <cmath>
#include <cstdint>
#include <set>
#include <type_traits>
#include <variant>

namespace {

namespace ast = tl2cgen::compiler::detail::ast;

template <typename ThresholdType>
void ScanThresholds(ast::ASTNode* node, std::vector<std::set<ThresholdType>>* cut_pts) {
  ast::NumericalConditionNode* num_cond;
  if ((num_cond = dynamic_cast<ast::NumericalConditionNode*>(node))) {
    TL2CGEN_CHECK(!num_cond->quantized_threshold_) << "should not be already quantized";
    auto const threshold = std::get<ThresholdType>(num_cond->threshold_);
    if (std::isfinite(threshold)) {
      (*cut_pts)[num_cond->split_index_].insert(threshold);
    }
  }
  for (ast::ASTNode* child : node->children_) {
    ScanThresholds(child, cut_pts);
  }
}

template <typename ThresholdType>
void RewriteThresholds(ast::ASTNode* node, std::vector<std::vector<ThresholdType>> const& cut_pts) {
  ast::NumericalConditionNode* num_cond;
  if ((num_cond = dynamic_cast<ast::NumericalConditionNode*>(node))) {
    TL2CGEN_CHECK(!num_cond->quantized_threshold_) << "Should not be already quantized";
    auto const threshold = std::get<ThresholdType>(num_cond->threshold_);
    if (std::isfinite(threshold)) {
      auto const& v = cut_pts[num_cond->split_index_];
      {
        auto loc = tl2cgen::detail::math::BinarySearch(v.begin(), v.end(), threshold);
        TL2CGEN_CHECK(loc != v.end());
        num_cond->quantized_threshold_ = static_cast<int>(loc - v.begin()) * 2;
      }
      {
        auto zero = static_cast<ThresholdType>(0);
        auto loc = std::lower_bound(v.begin(), v.end(), zero);
        num_cond->zero_quantized_ = static_cast<int>(loc - v.begin()) * 2;
        if (loc != v.end() && zero != *loc) {
          --num_cond->zero_quantized_;
        }
      }
    }  // Splits with infinite thresholds will not be quantized
  }
  for (ast::ASTNode* child : node->children_) {
    RewriteThresholds(child, cut_pts);
  }
}

}  // anonymous namespace

namespace tl2cgen::compiler::detail::ast {

void ASTBuilder::QuantizeThresholds() {
  std::visit(
      [this](auto&& meta) {
        using TypeMetaT = std::remove_const_t<std::remove_reference_t<decltype(meta)>>;
        using ThresholdT = typename TypeMetaT::threshold_type;
        std::vector<std::set<ThresholdT>> cut_pts;
        std::vector<std::vector<ThresholdT>> cut_pts_vec;
        cut_pts.resize(meta_.num_feature_);
        cut_pts_vec.resize(meta_.num_feature_);
        ScanThresholds(main_node_, &cut_pts);
        // Convert cut_pts into std::vector
        for (int i = 0; i < meta_.num_feature_; ++i) {
          std::copy(cut_pts[i].begin(), cut_pts[i].end(), std::back_inserter(cut_pts_vec[i]));
        }

        /* Revise all numerical splits by quantizing thresholds */
        RewriteThresholds(main_node_, cut_pts_vec);

        TL2CGEN_CHECK_EQ(main_node_->children_.size(), 1);
        ASTNode* top_func_node = main_node_->children_[0];
        TL2CGEN_CHECK(dynamic_cast<FunctionNode*>(top_func_node));
        /* dynamic_cast<> is used here to check node types. This is to ensure
          that we don't accidentally call QuantizeThresholds() twice. */

        ASTNode* quantizer_node = AddNode<QuantizerNode>(main_node_, std::move(cut_pts_vec));
        quantizer_node->children_.push_back(top_func_node);
        top_func_node->parent_ = quantizer_node;
        main_node_->children_[0] = quantizer_node;
      },
      meta_.type_meta_);
}

}  // namespace tl2cgen::compiler::detail::ast
