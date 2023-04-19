/*!
 * Copyright (c) 2023 by Contributors
 * \file pred_transform.h
 * \brief Tools to define prediction transform function
 * \author Hyunsu Cho
 */
#ifndef TL2CGEN_DETAIL_COMPILER_PRED_TRANSFORM_H_
#define TL2CGEN_DETAIL_COMPILER_PRED_TRANSFORM_H_

#include <string>
#include <vector>

namespace treelite {

class Model;

}  // namespace treelite

namespace tl2cgen::compiler::detail {

std::string PredTransformFunction(treelite::Model const& model);

}  // namespace tl2cgen::compiler::detail

#endif  // TL2CGEN_DETAIL_COMPILER_PRED_TRANSFORM_H_
