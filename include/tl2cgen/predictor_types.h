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
#include <type_traits>

namespace tl2cgen::predictor {

enum class DataTypeEnum : std::uint8_t { kFloat32 = 0, kFloat64 = 1 };

inline DataTypeEnum DataTypeFromString(std::string const& str) {
  if (str == "float32") {
    return DataTypeEnum::kFloat32;
  } else if (str == "float64") {
    return DataTypeEnum::kFloat64;
  } else {
    TL2CGEN_LOG(FATAL) << "Unrecognized data type: " << str;
    return DataTypeEnum::kFloat32;
  }
}

template <typename T>
inline DataTypeEnum DataTypeFromType() {
  if constexpr (std::is_same_v<T, float>) {
    return DataTypeEnum::kFloat32;
  } else if constexpr (std::is_same_v<T, double>) {
    return DataTypeEnum::kFloat64;
  } else {
    TL2CGEN_LOG(FATAL) << "Unrecognized data type: " << typeid(T).name();
    return DataTypeEnum::kFloat32;
  }
}

inline std::string DataTypeToString(DataTypeEnum data_type) {
  if (data_type == DataTypeEnum::kFloat32) {
    return "float32";
  } else if (data_type == DataTypeEnum::kFloat64) {
    return "float64";
  } else {
    TL2CGEN_LOG(FATAL) << "Unrecognized data type: " << static_cast<int>(data_type);
    return "";
  }
}

}  // namespace tl2cgen::predictor

#endif  // TL2CGEN_PREDICTOR_TYPES_H_
