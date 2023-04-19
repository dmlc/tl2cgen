/*!
 * Copyright (c) 2023 by Contributors
 * \file pred_transform.cc
 * \brief Library of transform functions to convert margins into predictions
 * \author Hyunsu Cho
 */

#include <tl2cgen/detail/compiler/pred_transform.h>
#include <tl2cgen/detail/compiler/templates/pred_transform.h>
#include <tl2cgen/logging.h>
#include <treelite/tree.h>

#include <string>
#include <unordered_map>

#define TL2CGEN_PRED_TRANSFORM_FUNC(name) \
  { #name, &(name) }

namespace {

using Model = treelite::Model;
using PredTransformFuncGenerator = std::string (*)(Model const&);

/* boilerplate */
#define TL2CGEN_PRED_TRANSFORM_REGISTRY_DEFAULT_TEMPLATE(FUNC_NAME)                \
  std::string FUNC_NAME(const treelite::Model& model) {                            \
    return tl2cgen::compiler::detail::templates::pred_transform::FUNC_NAME(model); \
  }

TL2CGEN_PRED_TRANSFORM_REGISTRY_DEFAULT_TEMPLATE(identity)
TL2CGEN_PRED_TRANSFORM_REGISTRY_DEFAULT_TEMPLATE(signed_square)
TL2CGEN_PRED_TRANSFORM_REGISTRY_DEFAULT_TEMPLATE(hinge)
TL2CGEN_PRED_TRANSFORM_REGISTRY_DEFAULT_TEMPLATE(sigmoid)
TL2CGEN_PRED_TRANSFORM_REGISTRY_DEFAULT_TEMPLATE(exponential)
TL2CGEN_PRED_TRANSFORM_REGISTRY_DEFAULT_TEMPLATE(exponential_standard_ratio)
TL2CGEN_PRED_TRANSFORM_REGISTRY_DEFAULT_TEMPLATE(logarithm_one_plus_exp)
TL2CGEN_PRED_TRANSFORM_REGISTRY_DEFAULT_TEMPLATE(identity_multiclass)
TL2CGEN_PRED_TRANSFORM_REGISTRY_DEFAULT_TEMPLATE(max_index)
TL2CGEN_PRED_TRANSFORM_REGISTRY_DEFAULT_TEMPLATE(softmax)
TL2CGEN_PRED_TRANSFORM_REGISTRY_DEFAULT_TEMPLATE(multiclass_ova)

const std::unordered_map<std::string, PredTransformFuncGenerator> pred_transform_db
    = {TL2CGEN_PRED_TRANSFORM_FUNC(identity), TL2CGEN_PRED_TRANSFORM_FUNC(signed_square),
        TL2CGEN_PRED_TRANSFORM_FUNC(hinge), TL2CGEN_PRED_TRANSFORM_FUNC(sigmoid),
        TL2CGEN_PRED_TRANSFORM_FUNC(exponential),
        TL2CGEN_PRED_TRANSFORM_FUNC(exponential_standard_ratio),
        TL2CGEN_PRED_TRANSFORM_FUNC(logarithm_one_plus_exp)};
/*! [pred_transform_db]
  - identity
    Do not transform. The output will be a vector of length
    [number of data points] that contains the margin score for every data point.
  - signed_square
    Apply the function f(x) = sign(x) * (x**2) element-wise to the margin vector. The
    output will be a vector of length [number of data points].
  - hinge
    Apply the function f(x) = (1 if x > 0 else 0) element-wise to the margin vector. The
    output will be a vector of length [number of data points], filled with 0's and 1's.
  - sigmoid
    Apply the sigmoid function element-wise to the margin vector. The output
    will be a vector of length [number of data points] that contains the
    probability of each data point belonging to the positive class.
  - exponential
    Apply the exponential function (exp) element-wise to the margin vector. The
    output will be a vector of length [number of data points].
  - exponential_standard_ratio
    Apply the exponential base 2 function (exp2) element-wise to a standardized
    version of the margin vector. The output will be a vector of length [number of data points].
    Each output element is exp2(-x / c), where x is the margin and c is the standardization
  constant.
  - logarithm_one_plus_exp
    Apply the function f(x) = log(1 + exp(x)) element-wise to the margin vector.
    The output will be a vector of length [number of data points].
    [pred_transform_db] */

// prediction transform function for *multi-class classifiers* only
const std::unordered_map<std::string, PredTransformFuncGenerator> pred_transform_multiclass_db
    = {TL2CGEN_PRED_TRANSFORM_FUNC(identity_multiclass), TL2CGEN_PRED_TRANSFORM_FUNC(max_index),
        TL2CGEN_PRED_TRANSFORM_FUNC(softmax), TL2CGEN_PRED_TRANSFORM_FUNC(multiclass_ova)};
/*! [pred_transform_multiclass_db]
 - identity_multiclass
   do not transform. The output will be a matrix with dimensions
   [number of data points] * [number of classes] that contains the margin score
   for every (data point, class) pair.
 - max_index
   compute the most probable class for each data point and output the class
   index. The output will be a vector of length [number of data points] that
   contains the most likely class of each data point.
 - softmax
   use the softmax function to transform a multi-dimensional vector into a
   proper probability distribution. The output will be a matrix with dimensions
   [number of data points] * [number of classes] that contains the predicted
   probability of each data point belonging to each class.
 - multiclass_ova
   apply the sigmoid function element-wise to the margin matrix. The output will
   be a matrix with dimensions [number of data points] * [number of classes].
    [pred_transform_multiclass_db] */

}  // anonymous namespace

std::string tl2cgen::compiler::detail::PredTransformFunction(Model const& model) {
  treelite::ModelParam param = model.param;
  if (model.task_param.num_class > 1) {  // multi-class classification
    auto it = pred_transform_multiclass_db.find(param.pred_transform);
    if (it == pred_transform_multiclass_db.end()) {
      std::ostringstream oss;
      for (auto const& e : pred_transform_multiclass_db) {
        oss << "'" << e.first << "', ";
      }
      TL2CGEN_LOG(FATAL) << "Invalid argument given for `pred_transform` parameter. "
                         << "For multi-class classification, you should set "
                         << "`pred_transform` to one of the following: "
                         << "{ " << oss.str() << " }";
    }
    return (it->second)(model);
  } else {
    auto it = pred_transform_db.find(param.pred_transform);
    if (it == pred_transform_db.end()) {
      std::ostringstream oss;
      for (auto const& e : pred_transform_db) {
        oss << "'" << e.first << "', ";
      }
      TL2CGEN_LOG(FATAL) << "Invalid argument given for `pred_transform` parameter. "
                         << "For any task that is NOT multi-class classification, you "
                         << "should set `pred_transform` to one of the following: "
                         << "{ " << oss.str() << " }";
    }
    return (it->second)(model);
  }
}
