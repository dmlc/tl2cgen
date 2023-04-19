/*!
 * Copyright (c) 2023 by Contributors
 * \file main_template.cc
 * \author Hyunsu Cho
 * \brief Template for main function
 */

#include <tl2cgen/detail/compiler/templates/main_template.h>

namespace tl2cgen::compiler::detail::templates {

char const* const query_functions_definition_template =
    R"TL2CGENTEMPLATE(
size_t get_num_class(void) {{
  return {num_class};
}}

size_t get_num_feature(void) {{
  return {num_feature};
}}

const char* get_pred_transform(void) {{
  return "{pred_transform}";
}}

float get_sigmoid_alpha(void) {{
  return {sigmoid_alpha};
}}

float get_ratio_c(void) {{
  return {ratio_c};
}}

float get_global_bias(void) {{
  return {global_bias};
}}

const char* get_threshold_type(void) {{
  return "{threshold_type_str}";
}}

const char* get_leaf_output_type(void) {{
  return "{leaf_output_type_str}";
}}
)TL2CGENTEMPLATE";

char const* const main_start_template =
    R"TL2CGENTEMPLATE(
#include "header.h"

{array_is_categorical};

{query_functions_definition}

{pred_transform_function}
{predict_function_signature} {{
)TL2CGENTEMPLATE";

char const* const main_end_multiclass_template =
    R"TL2CGENTEMPLATE(
  for (int i = 0; i < {num_class}; ++i) {{
    result[i] = sum[i]{optional_average_field} + ({leaf_output_type})({global_bias});
  }}
  if (!pred_margin) {{
    return pred_transform(result);
  }} else {{
    return {num_class};
  }}
}}
)TL2CGENTEMPLATE";  // only for multiclass classification

char const* const main_end_template =
    R"TL2CGENTEMPLATE(
  sum = sum{optional_average_field} + ({leaf_output_type})({global_bias});
  if (!pred_margin) {{
    return pred_transform(sum);
  }} else {{
    return sum;
  }}
}}
)TL2CGENTEMPLATE";

}  // namespace tl2cgen::compiler::detail::templates
