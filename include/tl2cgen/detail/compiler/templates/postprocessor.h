/*!
 * Copyright (c) 2024 by Contributors
 * \file postprocessor.h
 * \author Hyunsu Cho
 * \brief Template for postprocessor function in generated C code
 */

#ifndef TL2CGEN_DETAIL_COMPILER_TEMPLATES_POSTPROCESSOR_H_
#define TL2CGEN_DETAIL_COMPILER_TEMPLATES_POSTPROCESSOR_H_

#include <string>

namespace treelite {

class Model;  // forward declaration

}  // namespace treelite

namespace tl2cgen::compiler::detail::templates::postprocessor {

std::string identity(treelite::Model const& model);
std::string signed_square(treelite::Model const& model);
std::string hinge(treelite::Model const& model);
std::string sigmoid(treelite::Model const& model);
std::string exponential(treelite::Model const& model);
std::string exponential_standard_ratio(treelite::Model const& model);
std::string logarithm_one_plus_exp(treelite::Model const& model);
std::string identity_multiclass(treelite::Model const& model);
std::string softmax(treelite::Model const& model);
std::string multiclass_ova(treelite::Model const& model);

}  // namespace tl2cgen::compiler::detail::templates::postprocessor

#endif  // TL2CGEN_DETAIL_COMPILER_TEMPLATES_POSTPROCESSOR_H_
