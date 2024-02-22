/*!
 * Copyright (c) 2024 by Contributors
 * \file output_node.cc
 * \brief Convert OutputNode in AST into C code
 * \author Hyunsu Cho
 */

#include <fmt/format.h>
#include <tl2cgen/detail/compiler/ast/ast.h>
#include <tl2cgen/detail/compiler/codegen/codegen.h>
#include <tl2cgen/logging.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <variant>

using namespace fmt::literals;

namespace tl2cgen::compiler::detail::codegen {

void HandleOutputNode(ast::OutputNode const* node, CodeCollection& gencode) {
  TL2CGEN_CHECK_EQ(node->children_.size(), 0);

  std::int32_t const num_target = node->meta_->num_target_;
  std::vector<std::int32_t> const& num_class = node->meta_->num_class_;
  std::int32_t const max_num_class = *std::max_element(num_class.begin(), num_class.end());

  // In the predict() function, the result[] array represents the slice output(row_id, :, :)
  // that holds the prediction for a single row.
  std::visit(
      [&](auto&& leaf_output) {
        if (node->target_id_ < 0 && node->class_id_ < 0) {
          // The leaf node produces output for all targets and all classes
          std::array<std::int32_t, 2> const expected_shape{num_target, max_num_class};
          TL2CGEN_CHECK(node->meta_->leaf_vector_shape_ == expected_shape);
          TL2CGEN_CHECK_EQ(leaf_output.size(), num_target * max_num_class);
          for (std::int32_t target_id = 0; target_id < num_target; ++target_id) {
            for (std::int32_t class_id = 0; class_id < num_class[target_id]; ++class_id) {
              // output(row_id, target_id, class_id) += leaf(target_id, class_id)
              gencode.PushFragment(fmt::format("result[{offset}] += {leaf};",
                  "offset"_a = target_id * max_num_class + class_id,
                  "leaf"_a = leaf_output[target_id * max_num_class + class_id]));
            }
          }
        } else if (node->target_id_ < 0) {
          // The leaf node produces output for all targets and a single class
          std::array<std::int32_t, 2> const expected_shape{num_target, 1};
          TL2CGEN_CHECK(node->meta_->leaf_vector_shape_ == expected_shape);
          TL2CGEN_CHECK_EQ(leaf_output.size(), num_target);
          TL2CGEN_CHECK_GE(node->class_id_, 0);
          auto const class_id = node->class_id_;
          for (std::int32_t target_id = 0; target_id < num_target; ++target_id) {
            // output(row_id, target_id, class_id) += leaf(target_id)
            gencode.PushFragment(fmt::format("result[{offset}] += {leaf};",
                "offset"_a = target_id * max_num_class + class_id,
                "leaf"_a = leaf_output[target_id]));
          }
        } else if (node->class_id_ < 0) {
          // The leaf node produces output for all classes and a single target
          std::array<std::int32_t, 2> const expected_shape{1, max_num_class};
          TL2CGEN_CHECK(node->meta_->leaf_vector_shape_ == expected_shape);
          TL2CGEN_CHECK_EQ(leaf_output.size(), max_num_class);
          TL2CGEN_CHECK_GE(node->target_id_, 0);
          auto const target_id = node->target_id_;
          for (std::int32_t class_id = 0; class_id < num_class[target_id]; ++class_id) {
            // output(row_id, target_id, class_id) += leaf(class_id)
            gencode.PushFragment(fmt::format("result[{offset}] += {leaf};",
                "offset"_a = target_id * max_num_class + class_id,
                "leaf"_a = leaf_output[class_id]));
          }
        } else {
          // The leaf node produces output for a single target and a single class
          std::array<std::int32_t, 2> const expected_shape{1, 1};
          TL2CGEN_CHECK(node->meta_->leaf_vector_shape_ == expected_shape);
          TL2CGEN_CHECK_EQ(leaf_output.size(), 1);
          TL2CGEN_CHECK_GE(node->target_id_, 0);
          TL2CGEN_CHECK_GE(node->class_id_, 0);
          auto const target_id = node->target_id_;
          auto const class_id = node->class_id_;
          // output(row_id, target_id, class_id) += leaf(0)
          gencode.PushFragment(fmt::format("result[{offset}] += {leaf};",
              "offset"_a = target_id * max_num_class + class_id, "leaf"_a = leaf_output[0]));
        }
      },
      node->leaf_output_);
}

}  // namespace tl2cgen::compiler::detail::codegen
