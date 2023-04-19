/*!
 * Copyright (c) 2023 by Contributors
 * \file qnode_template.cc
 * \author Hyunsu Cho
 * \brief Code template for QuantizerNode
 */

#include <tl2cgen/detail/compiler/templates/qnode_template.h>

namespace tl2cgen::compiler::detail::templates {

char const* const qnode_template =
    R"TL2CGENTEMPLATE(
#include <stdlib.h>

/*
 * \brief function to convert a feature value into bin index.
 * \param val feature value, in floating-point
 * \param fid feature identifier
 * \return bin index corresponding to given feature value
 */
static inline int quantize({threshold_type} val, unsigned fid) {{
  const size_t offset = th_begin[fid];
  const {threshold_type}* array = &threshold[offset];
  int len = th_len[fid];
  int low = 0;
  int high = len;
  int mid;
  {threshold_type} mval;
  // It is possible th_begin[i] == [total_num_threshold]. This means that
  // all features i, (i+1), ... are not used for any of the splits in the model.
  // So in this case, just return something
  if (offset == {total_num_threshold} || val < array[0]) {{
    return -10;
  }}
  while (low + 1 < high) {{
    mid = (low + high) / 2;
    mval = array[mid];
    if (val == mval) {{
      return mid * 2;
    }} else if (val < mval) {{
      high = mid;
    }} else {{
      low = mid;
    }}
  }}
  if (array[low] == val) {{
    return low * 2;
  }} else if (high == len) {{
    return len * 2;
  }} else {{
    return low * 2 + 1;
  }}
}}
)TL2CGENTEMPLATE";

char const* const quantize_loop_template =
    R"TL2CGENTEMPLATE(
for (int i = 0; i < {num_feature}; ++i) {{
  if (data[i].missing != -1 && !is_categorical[i]) {{
    data[i].qvalue = quantize(data[i].fvalue, i);
  }}
}}
)TL2CGENTEMPLATE";

}  // namespace tl2cgen::compiler::detail::templates
