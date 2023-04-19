/*!
 * Copyright (c) 2023 by Contributors
 * \file data_matrix_type.h
 * \author Hyunsu Cho
 * \brief Enum type representing the kind of DMatrix
 */

#ifndef TL2CGEN_DATA_MATRIX_TYPE_H_
#define TL2CGEN_DATA_MATRIX_TYPE_H_

#include <tl2cgen/logging.h>

#include <cstdint>
#include <string>

namespace tl2cgen {

enum class DMatrixType : std::uint8_t { kDenseCLayout = 0, kSparseCSR = 1 };

enum class DMatrixElementType : std::uint8_t { kFloat32 = 0, kFloat64 = 1 };

inline DMatrixType DMatrixTypeFromString(std::string const& str) {
  if (str == "dense") {
    return DMatrixType::kDenseCLayout;
  } else if (str == "sparse_csr") {
    return DMatrixType::kSparseCSR;
  } else {
    TL2CGEN_LOG(FATAL) << "Unrecognized DMatrix type: " << str;
    return DMatrixType::kDenseCLayout;
  }
}

inline DMatrixElementType DMatrixElementTypeFromString(std::string const& str) {
  if (str == "float32") {
    return DMatrixElementType::kFloat32;
  } else if (str == "float64") {
    return DMatrixElementType::kFloat64;
  } else {
    TL2CGEN_LOG(FATAL) << "Unrecognized DMatrix element type: " << str;
    return DMatrixElementType::kFloat32;
  }
}

}  // namespace tl2cgen

#endif  // TL2CGEN_DATA_MATRIX_TYPE_H_
