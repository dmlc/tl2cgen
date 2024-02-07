/*!
 * Copyright (c) 2024 by Contributors
 * \file postprocessor.cc
 * \brief Library of transform functions to convert margins into predictions
 * \author Hyunsu Cho
 */

#include <tl2cgen/detail/compiler/postprocessor.h>
#include <tl2cgen/detail/compiler/templates/postprocessor.h>
#include <tl2cgen/logging.h>
#include <treelite/tree.h>

#include <string>
#include <unordered_map>

#define TL2CGEN_POSTPROCESSOR_FUNC(name) \
  { #name, &(name) }

namespace {

using Model = treelite::Model;
using PostprocessorGenerator = std::string (*)(Model const&);

/* Boilerplate */
#define TL2CGEN_POSTPROCESSOR_REGISTRY_DEFAULT_TEMPLATE(FUNC_NAME)                \
  std::string FUNC_NAME(const treelite::Model& model) {                           \
    return tl2cgen::compiler::detail::templates::postprocessor::FUNC_NAME(model); \
  }

/*
 * See https://treelite.readthedocs.io/en/latest/knobs/postprocessor.html for the description of
 * each postprocessor function.
 */

TL2CGEN_POSTPROCESSOR_REGISTRY_DEFAULT_TEMPLATE(identity)
TL2CGEN_POSTPROCESSOR_REGISTRY_DEFAULT_TEMPLATE(signed_square)
TL2CGEN_POSTPROCESSOR_REGISTRY_DEFAULT_TEMPLATE(hinge)
TL2CGEN_POSTPROCESSOR_REGISTRY_DEFAULT_TEMPLATE(sigmoid)
TL2CGEN_POSTPROCESSOR_REGISTRY_DEFAULT_TEMPLATE(exponential)
TL2CGEN_POSTPROCESSOR_REGISTRY_DEFAULT_TEMPLATE(exponential_standard_ratio)
TL2CGEN_POSTPROCESSOR_REGISTRY_DEFAULT_TEMPLATE(logarithm_one_plus_exp)
TL2CGEN_POSTPROCESSOR_REGISTRY_DEFAULT_TEMPLATE(identity_multiclass)
TL2CGEN_POSTPROCESSOR_REGISTRY_DEFAULT_TEMPLATE(softmax)
TL2CGEN_POSTPROCESSOR_REGISTRY_DEFAULT_TEMPLATE(multiclass_ova)

std::unordered_map<std::string, PostprocessorGenerator> const const postprocessor_db = {
    TL2CGEN_POSTPROCESSOR_FUNC(identity), TL2CGEN_POSTPROCESSOR_FUNC(signed_square),
    TL2CGEN_POSTPROCESSOR_FUNC(hinge), TL2CGEN_POSTPROCESSOR_FUNC(sigmoid),
    TL2CGEN_POSTPROCESSOR_FUNC(exponential), TL2CGEN_POSTPROCESSOR_FUNC(exponential_standard_ratio),
    TL2CGEN_POSTPROCESSOR_FUNC(logarithm_one_plus_exp)};

// Postprocessor functions for *multi-class classifiers*
std::unordered_map<std::string, PostprocessorGenerator> const const postprocessor_multiclass_db
    = {TL2CGEN_POSTPROCESSOR_FUNC(identity_multiclass), TL2CGEN_POSTPROCESSOR_FUNC(softmax),
        TL2CGEN_POSTPROCESSOR_FUNC(multiclass_ova)};

}  // anonymous namespace

std::string tl2cgen::compiler::detail::PostProcessorFunction(Model const& model) {
  TL2CGEN_CHECK_EQ(model.num_target, 1) << "TL2cgen does not yet support multi-target models";
  auto const num_class = model.num_class[0];
  if (num_class > 1) {  // multi-class classification
    auto it = postprocessor_multiclass_db.find(model.postprocessor);
    if (it == postprocessor_multiclass_db.end()) {
      std::ostringstream oss;
      for (auto const& e : postprocessor_multiclass_db) {
        oss << "'" << e.first << "', ";
      }
      TL2CGEN_LOG(FATAL) << "Invalid argument given for `postprocessor` parameter. "
                         << "For multi-class classification, you should set "
                         << "`postprocessor` to one of the following: "
                         << "{ " << oss.str() << " }";
    }
    return (it->second)(model);
  } else {
    auto it = postprocessor_db.find(model.postprocessor);
    if (it == postprocessor_db.end()) {
      std::ostringstream oss;
      for (auto const& e : postprocessor_db) {
        oss << "'" << e.first << "', ";
      }
      TL2CGEN_LOG(FATAL) << "Invalid argument given for `postprocessor` parameter. "
                         << "For any task that is NOT multi-class classification, you "
                         << "should set `postprocessor` to one of the following: "
                         << "{ " << oss.str() << " }";
    }
    return (it->second)(model);
  }
}
