/*!
 * Copyright (c) 2024 by Contributors
 * \file postprocessor.cc
 * \author Hyunsu Cho
 * \brief Template for postprocessor function in generated C code
 */

#include <fmt/format.h>
#include <tl2cgen/detail/compiler/templates/typeinfo.h>
#include <tl2cgen/logging.h>
#include <treelite/tree.h>

#include <string>

using namespace fmt::literals;

namespace tl2cgen::compiler::detail::templates::postprocessor {

std::string identity(treelite::Model const& model) {
  return fmt::format(
      R"TL2CGENTEMPLATE(static {threshold_type} postprocess({threshold_type} margin) {{
  return margin;
}})TL2CGENTEMPLATE",
      "threshold_type"_a = TypeInfoToCTypeString(model.GetThresholdType()));
}

std::string signed_square(treelite::Model const& model) {
  treelite::TypeInfo const threshold_type = model.GetThresholdType();
  return fmt::format(
      R"TL2CGENTEMPLATE(static inline {threshold_type} postprocess({threshold_type} margin) {{
  return {copysign}(margin * margin, margin);
}})TL2CGENTEMPLATE",
      "threshold_type"_a = TypeInfoToCTypeString(threshold_type),
      "copysign"_a = CCopySignForTypeInfo(threshold_type));
}

std::string hinge(treelite::Model const& model) {
  treelite::TypeInfo const threshold_type = model.GetThresholdType();
  return fmt::format(
      R"TL2CGENTEMPLATE(static inline {threshold_type} postprocess({threshold_type} margin) {{
  if (margin > 0) {{
    return ({threshold_type})(1);
  }} else {{
    return ({threshold_type})(0);
  }}
}})TL2CGENTEMPLATE",
      "threshold_type"_a = TypeInfoToCTypeString(threshold_type));
}

std::string sigmoid(treelite::Model const& model) {
  float const alpha = model.sigmoid_alpha;
  treelite::TypeInfo const threshold_type = model.GetThresholdType();
  TL2CGEN_CHECK_GT(alpha, 0.0f) << "sigmoid: alpha must be strictly positive";
  return fmt::format(
      R"TL2CGENTEMPLATE(static inline {threshold_type} postprocess({threshold_type} margin) {{
  const {threshold_type} alpha = ({threshold_type}){alpha};
  return ({threshold_type})(1) / (({threshold_type})(1) + {exp}(-alpha * margin));
}})TL2CGENTEMPLATE",
      "alpha"_a = alpha, "threshold_type"_a = TypeInfoToCTypeString(threshold_type),
      "exp"_a = CExpForTypeInfo(threshold_type));
}

std::string exponential(treelite::Model const& model) {
  treelite::TypeInfo const threshold_type = model.GetThresholdType();
  return fmt::format(
      R"TL2CGENTEMPLATE(static inline {threshold_type} postprocess({threshold_type} margin) {{
  return {exp}(margin);
}})TL2CGENTEMPLATE",
      "threshold_type"_a = TypeInfoToCTypeString(threshold_type),
      "exp"_a = CExpForTypeInfo(threshold_type));
}

std::string exponential_standard_ratio(treelite::Model const& model) {
  float const ratio_c = model.ratio_c;
  treelite::TypeInfo const threshold_type = model.GetThresholdType();
  return fmt::format(
      R"TL2CGENTEMPLATE(static inline {threshold_type} postprocess({threshold_type} margin) {{
  return {exp2}(-margin / ({threshold_type}){ratio_c});
}})TL2CGENTEMPLATE",
      "threshold_type"_a = TypeInfoToCTypeString(threshold_type), "ratio_c"_a = ratio_c,
      "exp2"_a = CExp2ForTypeInfo(threshold_type));
}

std::string logarithm_one_plus_exp(treelite::Model const& model) {
  treelite::TypeInfo const threshold_type = model.GetThresholdType();
  return fmt::format(
      R"TL2CGENTEMPLATE(static inline {threshold_type} postprocess({threshold_type} margin) {{
  return {log1p}({exp}(margin));
}})TL2CGENTEMPLATE",
      "threshold_type"_a = TypeInfoToCTypeString(threshold_type),
      "exp"_a = CExpForTypeInfo(threshold_type), "log1p"_a = CLog1PForTypeInfo(threshold_type));
}

std::string identity_multiclass(treelite::Model const& model) {
  TL2CGEN_CHECK_EQ(model.num_target, 1) << "TL2cgen does not yet support multi-target models";
  auto const num_class = model.num_class[0];
  TL2CGEN_CHECK_GT(num_class, 1)
      << "identity_multiclass: model is not a proper multi-class classifier";
  return fmt::format(
      R"TL2CGENTEMPLATE(static inline size_t postprocess({threshold_type}* pred) {{
  return {num_class};
}})TL2CGENTEMPLATE",
      "num_class"_a = num_class,
      "threshold_type"_a = TypeInfoToCTypeString(model.GetThresholdType()));
}

std::string softmax(treelite::Model const& model) {
  TL2CGEN_CHECK_EQ(model.num_target, 1) << "TL2cgen does not yet support multi-target models";
  auto const num_class = model.num_class[0];
  TL2CGEN_CHECK_GT(num_class, 1) << "softmax: model is not a proper multi-class classifier";
  treelite::TypeInfo const threshold_type = model.GetThresholdType();
  return fmt::format(
      R"TL2CGENTEMPLATE(static inline size_t postprocess({threshold_type}* pred) {{
  const int num_class = {num_class};
  {threshold_type} max_margin = pred[0];
  double norm_const = 0.0;
  {threshold_type} t;
  for (int k = 1; k < num_class; ++k) {{
    if (pred[k] > max_margin) {{
      max_margin = pred[k];
    }}
  }}
  for (int k = 0; k < num_class; ++k) {{
    t = {exp}(pred[k] - max_margin);
    norm_const += t;
    pred[k] = t;
  }}
  for (int k = 0; k < num_class; ++k) {{
    pred[k] /= ({threshold_type})norm_const;
  }}
  return (size_t)num_class;
}})TL2CGENTEMPLATE",
      "num_class"_a = num_class, "threshold_type"_a = TypeInfoToCTypeString(threshold_type),
      "exp"_a = CExpForTypeInfo(threshold_type));
}

std::string multiclass_ova(treelite::Model const& model) {
  TL2CGEN_CHECK_EQ(model.num_target, 1) << "TL2cgen does not yet support multi-target models";
  auto const num_class = model.num_class[0];
  TL2CGEN_CHECK(num_class > 1) << "multiclass_ova: model is not a proper multi-class classifier";
  float const alpha = model.sigmoid_alpha;
  treelite::TypeInfo const threshold_type = model.GetThresholdType();
  TL2CGEN_CHECK_GT(alpha, 0.0f) << "multiclass_ova: alpha must be strictly positive";
  return fmt::format(
      R"TL2CGENTEMPLATE(static inline size_t postprocess({threshold_type}* pred) {{
  const {threshold_type} alpha = ({threshold_type}){alpha};
  const int num_class = {num_class};
  for (int k = 0; k < num_class; ++k) {{
    pred[k] = ({threshold_type})(1) / (({threshold_type})(1) + {exp}(-alpha * pred[k]));
  }}
  return (size_t)num_class;
}})TL2CGENTEMPLATE",
      "num_class"_a = num_class, "alpha"_a = alpha,
      "threshold_type"_a = TypeInfoToCTypeString(threshold_type),
      "exp"_a = CExpForTypeInfo(threshold_type));
}

}  // namespace tl2cgen::compiler::detail::templates::postprocessor
