/*!
 * Copyright (c) 2023 by Contributors
 * \file compiler_param.h
 * \brief Parameters for tree compiler
 * \author Hyunsu Cho
 */
#ifndef TL2CGEN_COMPILER_PARAM_H_
#define TL2CGEN_COMPILER_PARAM_H_

#include <limits>
#include <string>

namespace tl2cgen::compiler {

/*! \brief parameters for tree compiler */
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
  std::string annotate_in;
  /*! \brief Whether to quantize threshold points (0: no, >0: yes) */
  int quantize;
  /*! \brief Option to enable parallel compilation;
             if set to nonzero, the trees will be evely distributed
             into ``[parallel_comp]`` files. Set this option to improve
             compilation time and reduce memory consumption during
             compilation. */
  int parallel_comp;
  /*! \brief If >0, produce extra messages */
  int verbose;
  /*! \brief Native lib name (without extension) */
  std::string native_lib_name;
  /*! \brief Parameter for folding rarely visited subtrees (no if/else blocks);
             all nodes whose data counts are lower than that of the root node
             of the decision tree by ``[code_folding_req]`` will be
             folded. To diable folding, set to ``+inf``. If hessian sums are
             available, they will be used as proxies of data counts. */
  double code_folding_req;
  /*! \brief Only applicable when ``compiler`` is set to ``failsafe``. If set to a positive value,
             the fail-safe compiler will not emit large constant arrays to the C code. Instead,
             the arrays will be emitted as an ELF binary (Linux only). For large arrays, it is
             much faster to directly dump ELF binaries than to pass them to a C compiler. */
  int dump_array_as_elf;
  /*! \} */

  static CompilerParam ParseFromJSON(char const* param_json_str);
};

}  // namespace tl2cgen::compiler

#endif  // TL2CGEN_COMPILER_PARAM_H_
