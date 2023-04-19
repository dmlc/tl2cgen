/*!
 * Copyright (c) 2023 by Contributors
 * \file pred_transform.h
 * \author Hyunsu Cho
 * \brief Template for pred_transform() function in generated C code
 */

#ifndef TL2CGEN_DETAIL_COMPILER_TEMPLATES_PRED_TRANSFORM_H_
#define TL2CGEN_DETAIL_COMPILER_TEMPLATES_PRED_TRANSFORM_H_

#include <string>

namespace treelite {

class Model;  // forward declaration

}  // namespace treelite

namespace tl2cgen::compiler::detail::templates::pred_transform {

std::string identity(treelite::Model const& model);
std::string signed_square(treelite::Model const& model);
std::string hinge(treelite::Model const& model);
std::string sigmoid(treelite::Model const& model);
std::string exponential(treelite::Model const& model);
std::string exponential_standard_ratio(treelite::Model const& model);
std::string logarithm_one_plus_exp(treelite::Model const& model);
std::string identity_multiclass(treelite::Model const& model);
std::string max_index(treelite::Model const& model);
std::string softmax(treelite::Model const& model);
std::string multiclass_ova(treelite::Model const& model);

}  // namespace tl2cgen::compiler::detail::templates::pred_transform

#endif  // TL2CGEN_DETAIL_COMPILER_TEMPLATES_PRED_TRANSFORM_H_
