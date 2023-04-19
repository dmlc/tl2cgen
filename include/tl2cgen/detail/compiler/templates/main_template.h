/*!
 * Copyright (c) 2023 by Contributors
 * \file main_template.h
 * \author Hyunsu Cho
 * \brief Template for main function
 */

#ifndef TL2CGEN_DETAIL_COMPILER_TEMPLATES_MAIN_TEMPLATE_H_
#define TL2CGEN_DETAIL_COMPILER_TEMPLATES_MAIN_TEMPLATE_H_

namespace tl2cgen::compiler::detail::templates {

extern char const* const query_functions_definition_template;
extern char const* const main_start_template;
extern char const* const main_end_multiclass_template;
extern char const* const main_end_template;

}  // namespace tl2cgen::compiler::detail::templates

#endif  // TL2CGEN_DETAIL_COMPILER_TEMPLATES_MAIN_TEMPLATE_H_
