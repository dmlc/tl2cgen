/*!
 * Copyright (c) 2023 by Contributors
 * \file data_matrix_types.h
 * \author Hyunsu Cho
 * \brief Enum types related to DMatrix
 */

#ifndef TL2CGEN_DATA_MATRIX_TYPES_H_
#define TL2CGEN_DATA_MATRIX_TYPES_H_

#include <tl2cgen/logging.h>

#include <cstdint>
#include <string>

namespace tl2cgen {

enum class DMatrixTypeEnum : std::uint8_t { kDenseCLayout = 0, kSparseCSR = 1 };
constexpr int DMatrixTypeEnumCount = 2;

enum class DMatrixElementTypeEnum : std::uint8_t { kFloat32 = 0, kFloat64 = 1 };

inline DMatrixTypeEnum DMatrixTypeFromString(std::string const& str) {
  if (str == "dense") {
    return DMatrixTypeEnum::kDenseCLayout;
  } else if (str == "sparse_csr") {
    return DMatrixTypeEnum::kSparseCSR;
  } else {
    TL2CGEN_LOG(FATAL) << "Unrecognized DMatrix type: " << str;
    return DMatrixTypeEnum::kDenseCLayout;
  }
}

inline DMatrixElementTypeEnum DMatrixElementTypeFromString(std::string const& str) {
  if (str == "float32") {
    return DMatrixElementTypeEnum::kFloat32;
  } else if (str == "float64") {
    return DMatrixElementTypeEnum::kFloat64;
  } else {
    TL2CGEN_LOG(FATAL) << "Unrecognized DMatrix element type: " << str;
    return DMatrixElementTypeEnum::kFloat32;
  }
}

}  // namespace tl2cgen

#endif  // TL2CGEN_DATA_MATRIX_TYPES_H_
