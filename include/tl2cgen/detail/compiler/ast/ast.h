/*!
 * Copyright (c) 2023-2024 by Contributors
 * \file ast.h
 * \brief Definition for AST classes
 * \author Hyunsu Cho
 */
#ifndef TL2CGEN_DETAIL_COMPILER_AST_AST_H_
#define TL2CGEN_DETAIL_COMPILER_AST_AST_H_

#include <treelite/enum/operator.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tl2cgen::compiler::detail::ast {

class ModelMeta;

class ASTNode {
 public:
  ASTNode* parent_;
  std::vector<ASTNode*> children_;
  ModelMeta* meta_;
  int node_id_;
  int tree_id_;
  std::optional<std::uint64_t> data_count_;
  std::optional<double> sum_hess_;
  virtual std::string GetDump() const = 0;
  virtual ~ASTNode() = 0;  // force ASTNode to be abstract class
 protected:
  ASTNode() : meta_(nullptr), parent_(nullptr), node_id_(-1), tree_id_(-1) {}
};
inline ASTNode::~ASTNode() {}

class MainNode : public ASTNode {
 public:
  MainNode(std::vector<double> base_scores, std::optional<std::vector<std::int32_t>> average_factor,
      std::string postprocessor)
      : base_scores_(std::move(base_scores)),
        average_factor_(std::move(average_factor)),
        postprocessor_(std::move(postprocessor)) {}
  std::vector<double> base_scores_;
  // Each output[target_id, class_id] should be incremented by base_scores_[target_id, class_id].
  std::optional<std::vector<std::int32_t>> average_factor_;
  // If model.average_tree_output is True, each output[target_id, class_id] should be divided by
  // average_factor_[target_id, class_id].
  // If model.average_tree_output is False, set this field to std::nullopt.
  std::string postprocessor_;  // Postprocessor to apply after computing raw predictions
  std::string GetDump() const override;
};

class TranslationUnitNode : public ASTNode {
 public:
  explicit TranslationUnitNode(int unit_id) : unit_id_(unit_id) {}
  int unit_id_;
  std::string GetDump() const override;
};

class QuantizerNode : public ASTNode {
 public:
  using ThresholdListVariantT
      = std::variant<std::vector<std::vector<float>>, std::vector<std::vector<double>>>;
  explicit QuantizerNode(ThresholdListVariantT threshold_list)
      : threshold_list_(std::move(threshold_list)) {}
  ThresholdListVariantT threshold_list_;
  std::string GetDump() const override;
};

class FunctionNode : public ASTNode {
 public:
  FunctionNode() {}
  std::string GetDump() const override;
};

class ConditionNode : public ASTNode {
 public:
  ConditionNode(std::uint32_t split_index, bool default_left)
      : split_index_(split_index), default_left_(default_left) {}
  std::uint32_t split_index_;
  bool default_left_;
  std::optional<double> gain_;
  std::string GetDump() const override;
};

class NumericalConditionNode : public ConditionNode {
 public:
  using ThresholdVariantT = std::variant<float, double>;
  NumericalConditionNode(std::uint32_t split_index, bool default_left, treelite::Operator op,
      ThresholdVariantT threshold, std::optional<int> quantized_threshold)
      : ConditionNode(split_index, default_left),
        op_(op),
        threshold_(threshold),
        quantized_threshold_(quantized_threshold),
        zero_quantized_(-1) {}
  treelite::Operator op_;
  ThresholdVariantT threshold_;
  std::optional<int> quantized_threshold_;
  int zero_quantized_;  // quantized value of 0.0f (useful when convert_missing_to_zero is set)
  std::string GetDump() const override;
};

class CategoricalConditionNode : public ConditionNode {
 public:
  CategoricalConditionNode(std::uint32_t split_index, bool default_left,
      std::vector<std::uint32_t> const& category_list, bool category_list_right_child)
      : ConditionNode(split_index, default_left),
        category_list_(category_list),
        category_list_right_child_(category_list_right_child) {}
  std::vector<std::uint32_t> category_list_;
  bool category_list_right_child_;
  std::string GetDump() const override;
};

class OutputNode : public ASTNode {
 public:
  using OutputVariantT = std::variant<std::vector<float>, std::vector<double>>;
  explicit OutputNode(
      std::int32_t target_id, std::int32_t class_id, OutputVariantT const& leaf_output)
      : target_id_(target_id), class_id_(class_id), leaf_output_(leaf_output) {}
  std::int32_t target_id_, class_id_;
  OutputVariantT leaf_output_;
  std::string GetDump() const override;
};

// Metadata about the model
class ModelMeta {
 public:
  std::int32_t num_target_;  // Number of output targets
  std::vector<std::int32_t> num_class_;  // num_class[i]: Number of classes in i-th target
  std::array<std::int32_t, 2> leaf_vector_shape_;  // Shape of each leaf output
  std::int32_t num_feature_;  // Number of features in the training data
  std::vector<bool> is_categorical_;
  // is_categorical_[i]: is feature i categorical?
  float sigmoid_alpha_;  // Parameter to control the "sigmoid" postprocessor
  float ratio_c_;  // Parameter to control the "exponential_standard_ratio" postprocessor
  template <typename ThresholdType, typename LeafOutputType>
  class TypeMeta {
   public:
    using threshold_type = ThresholdType;
    using leaf_output_type = LeafOutputType;
  };
  std::variant<TypeMeta<float, float>, TypeMeta<double, double>> type_meta_;
  // Helper class to remember types for thresholds and leaf outputs
};

}  // namespace tl2cgen::compiler::detail::ast

#endif  // TL2CGEN_DETAIL_COMPILER_AST_AST_H_
