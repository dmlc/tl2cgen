/*!
 * Copyright (c) 2023 by Contributors
 * \file header_template.cc
 * \author Hyunsu Cho
 * \brief Template for header
 */

#include <tl2cgen/detail/compiler/templates/header_template.h>

namespace tl2cgen::compiler::detail::templates {

char const* const query_functions_prototype_template =
    R"TL2CGENTEMPLATE(
{dllexport}size_t get_num_class(void);
{dllexport}size_t get_num_feature(void);
{dllexport}const char* get_pred_transform(void);
{dllexport}float get_sigmoid_alpha(void);
{dllexport}float get_ratio_c(void);
{dllexport}float get_global_bias(void);
{dllexport}const char* get_threshold_type(void);
{dllexport}const char* get_leaf_output_type(void);
)TL2CGENTEMPLATE";

char const* const header_template =
    R"TL2CGENTEMPLATE(
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <stdint.h>

#if defined(__clang__) || defined(__GNUC__)
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

union Entry {{
  int missing;
  {threshold_type} fvalue;
  int qvalue;
}};

struct Node {{
  uint8_t default_left;
  unsigned int split_index;
  {threshold_type_Node} threshold;
  int left_child;
  int right_child;
}};

extern const unsigned char is_categorical[];

{query_functions_prototype}
{dllexport}{predict_function_signature};
)TL2CGENTEMPLATE";

}  // namespace tl2cgen::compiler::detail::templates
