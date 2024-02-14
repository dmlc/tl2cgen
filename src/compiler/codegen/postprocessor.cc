/*!
 * Copyright (c) 2024 by Contributors
 * \file postprocessor.cc
 * \author Hyunsu Cho
 * \brief Generate C code for postprocessing functions
 */

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <tl2cgen/detail/compiler/ast/ast.h>
#include <tl2cgen/detail/compiler/codegen/codegen.h>
#include <tl2cgen/logging.h>

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>

using namespace fmt::literals;

namespace {

namespace ast = tl2cgen::compiler::detail::ast;
namespace codegen = tl2cgen::compiler::detail::codegen;

std::string GetCopySignCFunc(std::string const& threshold_ctype) {
  if (threshold_ctype == "float") {
    return "copysignf";
  } else if (threshold_ctype == "double") {
    return "copysign";
  } else {
    TL2CGEN_LOG(FATAL) << "Unrecognized type: " << threshold_ctype;
    return "";
  }
}

std::string GetExpCFunc(std::string const& threshold_ctype) {
  if (threshold_ctype == "float") {
    return "expf";
  } else if (threshold_ctype == "double") {
    return "exp";
  } else {
    TL2CGEN_LOG(FATAL) << "Unrecognized type: " << threshold_ctype;
    return "";
  }
}

std::string GetExp2CFunc(std::string const& threshold_ctype) {
  if (threshold_ctype == "float") {
    return "exp2f";
  } else if (threshold_ctype == "double") {
    return "exp2";
  } else {
    TL2CGEN_LOG(FATAL) << "Unrecognized type: " << threshold_ctype;
    return "";
  }
}

std::string GetLog1pCFunc(std::string const& threshold_ctype) {
  if (threshold_ctype == "float") {
    return "log1pf";
  } else if (threshold_ctype == "double") {
    return "log1p";
  } else {
    TL2CGEN_LOG(FATAL) << "Unrecognized type: " << threshold_ctype;
    return "";
  }
}

std::string Identity(ast::ModelMeta const& model_meta) {
  return fmt::format(
      R"TL2CGENTEMPLATE(
void postprocess({threshold_type}* result) {{
  // Do nothing
}}
)TL2CGENTEMPLATE",
      "threshold_type"_a = codegen::GetThresholdCType(model_meta));
}

std::string SignedSquare(ast::ModelMeta const& model_meta) {
  std::string const threshold_type = codegen::GetThresholdCType(model_meta);
  return fmt::format(
      R"TL2CGENTEMPLATE(
void postprocess({threshold_type}* result) {{
  // signed_square
  {threshold_type} margin;
  for (size_t i = 0; i < N_TARGET * MAX_N_CLASS; ++i) {{
    margin = result[i];
    result[i] = {copysign}(margin * margin, margin);
  }}
}}
)TL2CGENTEMPLATE",
      "threshold_type"_a = threshold_type, "copysign"_a = GetCopySignCFunc(threshold_type));
}

std::string Hinge(ast::ModelMeta const& model_meta) {
  std::string const threshold_type = codegen::GetThresholdCType(model_meta);
  return fmt::format(
      R"TL2CGENTEMPLATE(
void postprocess({threshold_type}* result) {{
  // hinge
  for (size_t i = 0; i < N_TARGET * MAX_N_CLASS; ++i) {{
    if (result[i] > 0) {{
      result[i] = ({threshold_type})(1);
    }} else {{
      result[i] = ({threshold_type})(0);
    }}
  }}
}}
)TL2CGENTEMPLATE",
      "threshold_type"_a = threshold_type);
}

std::string Sigmoid(ast::ModelMeta const& model_meta) {
  float const alpha = model_meta.sigmoid_alpha_;
  std::string const threshold_type = codegen::GetThresholdCType(model_meta);
  TL2CGEN_CHECK_GT(alpha, 0.0f) << "sigmoid: alpha must be strictly positive";
  return fmt::format(
      R"TL2CGENTEMPLATE(
void postprocess({threshold_type}* result) {{
  // sigmoid
  const {threshold_type} alpha = ({threshold_type}){alpha};
  for (size_t i = 0; i < N_TARGET * MAX_N_CLASS; ++i) {{
    result[i] = ({threshold_type})(1) / (({threshold_type})(1) + {exp}(-alpha * result[i]));
  }}
}}
)TL2CGENTEMPLATE",
      "alpha"_a = alpha, "threshold_type"_a = threshold_type,
      "exp"_a = GetExpCFunc(threshold_type));
}

std::string Exponential(ast::ModelMeta const& model_meta) {
  std::string const threshold_type = codegen::GetThresholdCType(model_meta);
  return fmt::format(
      R"TL2CGENTEMPLATE(
void postprocess({threshold_type}* result) {{
  // exponential
  for (size_t i = 0; i < N_TARGET * MAX_N_CLASS; ++i) {{
    result[i] = {exp}(result[i]);
  }}
}}
)TL2CGENTEMPLATE",
      "threshold_type"_a = threshold_type, "exp"_a = GetExpCFunc(threshold_type));
}

std::string ExponentialStandardRatio(ast::ModelMeta const& model_meta) {
  float const ratio_c = model_meta.ratio_c_;
  std::string const threshold_type = codegen::GetThresholdCType(model_meta);
  return fmt::format(
      R"TL2CGENTEMPLATE(
void postprocess({threshold_type}* result) {{
  // exponential_standard_ratio
  const {threshold_type} ratio_c = ({threshold_type}){ratio_c};
  for (size_t i = 0; i < N_TARGET * MAX_N_CLASS; ++i) {{
    result[i] = {exp2}(-result[i] / ratio_c);
  }}
}}
)TL2CGENTEMPLATE",
      "threshold_type"_a = threshold_type, "exp2"_a = GetExp2CFunc(threshold_type));
}

std::string LogarithmOnePlusExp(ast::ModelMeta const& model_meta) {
  std::string const threshold_type = codegen::GetThresholdCType(model_meta);
  return fmt::format(
      R"TL2CGENTEMPLATE(
void postprocess({threshold_type}* result) {{
  // logarithm_one_plus_exp
  for (size_t i = 0; i < N_TARGET * MAX_N_CLASS; ++i) {{
    result[i] = {log1p}({exp}(result[i]));
  }}
}}
)TL2CGENTEMPLATE",
      "threshold_type"_a = threshold_type, "exp"_a = GetExpCFunc(threshold_type),
      "log1p"_a = GetLog1pCFunc(threshold_type));
}

std::string Softmax(ast::ModelMeta const& model_meta) {
  std::ostringstream oss;
  std::string const threshold_type = codegen::GetThresholdCType(model_meta);
  fmt::print(oss,
      R"TL2CGENTEMPLATE(
// Apply postprocessor for a single target
static void postprocess_impl({threshold_type}* target_result, int num_class) {{
  {threshold_type} max_margin = target_result[0];
  double norm_const = 0.0;
  {threshold_type} t;
  for (int k = 1; k < num_class; ++k) {{
    if (target_result[k] > max_margin) {{
      max_margin = target_result[k];
    }}
  }}
  for (int k = 0; k < num_class; ++k) {{
    t = {exp}(target_result[k] - max_margin);
    norm_const += t;
    pred[k] = t;
  }}
  for (int k = 0; k < num_class; ++k) {{
    pred[k] /= ({threshold_type})norm_const;
  }}
}}

void postprocess({threshold_type}* result) {{
  // softmax
)TL2CGENTEMPLATE",
      "threshold_type"_a = threshold_type, "exp"_a = GetExpCFunc(threshold_type));

  auto const max_num_class
      = *std::max_element(model_meta.num_class_.begin(), model_meta.num_class_.end());
  for (std::int32_t target_id = 0; target_id < model_meta.num_target_; ++target_id) {
    fmt::print(oss, "  postprocess_impl(result[{offset}], {num_class});\n",
        "offset"_a = target_id * max_num_class, "num_class"_a = model_meta.num_class_[target_id]);
  }
  oss << "}\n";
  return oss.str();
}

std::string MulticlassOva(ast::ModelMeta const& model_meta) {
  std::ostringstream oss;
  float const alpha = model_meta.sigmoid_alpha_;
  TL2CGEN_CHECK_GT(alpha, 0.0f) << "multiclass_ova: alpha must be strictly positive";
  std::string const threshold_type = codegen::GetThresholdCType(model_meta);
  fmt::print(oss,
      R"TL2CGENTEMPLATE(
// Apply postprocessor for a single target
static void postprocess_impl({threshold_type}* target_result, int num_class) {{
  const {threshold_type} alpha = ({threshold_type}){alpha};
  for (int k = 0; k < num_class; ++k) {{
    target_result[k] =
      ({threshold_type})(1) / (({threshold_type})(1) + {exp}(-alpha * target_result[k]));
  }}
}}

void postprocess({threshold_type}* result) {{
  // multiclass_ova
)TL2CGENTEMPLATE",
      "threshold_type"_a = threshold_type, "exp"_a = GetExpCFunc(threshold_type),
      "alpha"_a = alpha);

  auto const max_num_class
      = *std::max_element(model_meta.num_class_.begin(), model_meta.num_class_.end());
  for (std::int32_t target_id = 0; target_id < model_meta.num_target_; ++target_id) {
    fmt::print(oss, "  postprocess_impl(result[{offset}], {num_class});\n",
        "offset"_a = target_id * max_num_class, "num_class"_a = model_meta.num_class_[target_id]);
  }
  oss << "}\n";
  return oss.str();
}

}  // anonymous namespace

namespace tl2cgen::compiler::detail::codegen {

std::string GetPostprocessorFunc(
    ast::ModelMeta const& model_meta, std::string const& postprocessor) {
  if (postprocessor == "identity") {
    return Identity(model_meta);
  } else if (postprocessor == "signed_square") {
    return SignedSquare(model_meta);
  } else if (postprocessor == "hinge") {
    return Hinge(model_meta);
  } else if (postprocessor == "sigmoid") {
    return Sigmoid(model_meta);
  } else if (postprocessor == "exponential") {
    return Exponential(model_meta);
  } else if (postprocessor == "exponential_standard_ratio") {
    return ExponentialStandardRatio(model_meta);
  } else if (postprocessor == "logarithm_one_plus_exp") {
    return LogarithmOnePlusExp(model_meta);
  } else if (postprocessor == "identity_multiclass") {
    return Identity(model_meta);
  } else if (postprocessor == "softmax") {
    return Softmax(model_meta);
  } else if (postprocessor == "multiclass_ova") {
    return MulticlassOva(model_meta);
  } else {
    TL2CGEN_LOG(FATAL) << "Unknown postprocessor function: " << postprocessor;
    return "";
  }
}

}  // namespace tl2cgen::compiler::detail::codegen
