/*!
 * Copyright (c) 2023 by Contributors
 * \file code_folder_template.cc
 * \author Hyunsu Cho
 * \brief Template for evaluation logic for folded code
 */

#include <tl2cgen/detail/compiler/templates/code_folder_template.h>

namespace tl2cgen::compiler::detail::templates {

char const* const eval_loop_template =
    R"TL2CGENTEMPLATE(
nid = 0;
while (nid >= 0) {{  /* negative nid implies leaf */
  fid = {node_array_name}[nid].split_index;
  if (data[fid].missing == -1) {{
    cond = {node_array_name}[nid].default_left;
  }} else if (is_categorical[fid]) {{
    tmp = (unsigned int)data[fid].fvalue;
    cond = ({cat_bitmap_name}[{cat_begin_name}[nid] + tmp / 64] >> (tmp % 64)) & 1;
  }} else {{
    cond = (data[fid].{data_field} {comp_op} {node_array_name}[nid].threshold);
  }}
  nid = cond ? {node_array_name}[nid].left_child : {node_array_name}[nid].right_child;
}}

{output_switch_statement}
)TL2CGENTEMPLATE";

char const* const eval_loop_template_without_categorical_feature =
    R"TL2CGENTEMPLATE(
nid = 0;
while (nid >= 0) {{  /* negative nid implies leaf */
  fid = {node_array_name}[nid].split_index;
  if (data[fid].missing == -1) {{
    cond = {node_array_name}[nid].default_left;
  }} else {{
    cond = (data[fid].{data_field} {comp_op} {node_array_name}[nid].threshold);
  }}
  nid = cond ? {node_array_name}[nid].left_child : {node_array_name}[nid].right_child;
}}

{output_switch_statement}
)TL2CGENTEMPLATE";

}  // namespace tl2cgen::compiler::detail::templates
