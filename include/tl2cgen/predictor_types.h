/*!
 * Copyright (c) 2023 by Contributors
 * \file predictor_types.h
 * \author Hyunsu Cho
 * \brief Enum types related to Predictor
 */

#ifndef TL2CGEN_PREDICTOR_TYPES_H_
#define TL2CGEN_PREDICTOR_TYPES_H_

#include <tl2cgen/logging.h>

#include <cstdint>
#include <string>

namespace tl2cgen::predictor {

enum class DataTypeEnum : std::uint8_t { kFloat32 = 0, kFloat64 = 1, kUInt32 = 2 };

inline DataTypeEnum DataTypeFromString(std::string const& str) {
  if (str == "float32") {
    return DataTypeEnum::kFloat32;
  } else if (str == "float64") {
    return DataTypeEnum::kFloat64;
  } else if (str == "uint32") {
    return DataTypeEnum::kUInt32;
  } else {
    TL2CGEN_LOG(FATAL) << "Unrecognized data type: " << str;
    return DataTypeEnum::kFloat32;
  }
}

}  // namespace tl2cgen::predictor

#endif  // TL2CGEN_PREDICTOR_TYPES_H_
