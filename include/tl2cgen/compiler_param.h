/*!
 * Copyright (c) 2024 by Contributors
 * \file compiler_param.h
 * \brief Compiler parameters
 * \author Hyunsu Cho
 */
#ifndef TL2CGEN_COMPILER_PARAM_H_
#define TL2CGEN_COMPILER_PARAM_H_

#include <string>

namespace tl2cgen::compiler {

/*! \brief Parameters to control code generation */
struct CompilerParam {
  /*!
   * \defgroup compiler_param Parameters for tree compiler
   * \{
   */
  /*!
   * \brief Name of model annotation file.
   * \verbatim embed:rst:leading-asterisk
   * Use :py:func:`tl2cgen.annotate_branch` to generate this file.
   * \endverbatim
   */
  std::string annotate_in{"NULL"};
  /*! \brief Whether to quantize threshold points (0: no, >0: yes) */
  int quantize{0};
  /*! \brief Option to enable parallel compilation;
             if set to nonzero, the trees will be evely distributed
             into ``[parallel_comp]`` files. Set this option to improve
             compilation time and reduce memory consumption during
             compilation. */
  int parallel_comp{0};
  /*! \brief If >0, produce extra messages */
  int verbose{0};
  /*! \brief Native lib name (without extension) */
  std::string native_lib_name{"predictor"};
  /*! \} */

  static CompilerParam ParseFromJSON(char const* param_json_str);
};

}  // namespace tl2cgen::compiler

#endif  // TL2CGEN_COMPILER_PARAM_H_
