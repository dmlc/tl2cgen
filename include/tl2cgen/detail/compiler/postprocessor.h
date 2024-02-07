/*!
 * Copyright (c) 2024 by Contributors
 * \file postprocessor.h
 * \brief Tools to define postprocessor function
 * \author Hyunsu Cho
 */
#ifndef TL2CGEN_DETAIL_COMPILER_POSTPROCESSOR_H_
#define TL2CGEN_DETAIL_COMPILER_POSTPROCESSOR_H_

#include <string>
#include <vector>

namespace treelite {

class Model;

}  // namespace treelite

namespace tl2cgen::compiler::detail {

std::string PostProcessorFunction(treelite::Model const& model);

}  // namespace tl2cgen::compiler::detail

#endif  // TL2CGEN_DETAIL_COMPILER_POSTPROCESSOR_H_
