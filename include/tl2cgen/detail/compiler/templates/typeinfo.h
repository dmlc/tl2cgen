/*!
 * Copyright (c) 2023 by Contributors
 * \file typeinfo.h
 * \author Hyunsu Cho
 * \brief Look up C symbols corresponding to treelite::TypeInfo
 */

#ifndef TL2CGEN_DETAIL_COMPILER_TEMPLATES_TYPEINFO_H_
#define TL2CGEN_DETAIL_COMPILER_TEMPLATES_TYPEINFO_H_

#include <tl2cgen/logging.h>
#include <treelite/base.h>

#include <string>

namespace tl2cgen::compiler::detail::templates {

/*!
 * \brief Get string representation of the C type that's equivalent to the given type info
 * \param info A type info
 * \return String representation
 */
inline std::string TypeInfoToCTypeString(treelite::TypeInfo type) {
  switch (type) {
  case treelite::TypeInfo::kInvalid:
    TL2CGEN_LOG(FATAL) << "Invalid type";
    return "";
  case treelite::TypeInfo::kUInt32:
    return "uint32_t";
  case treelite::TypeInfo::kFloat32:
    return "float";
  case treelite::TypeInfo::kFloat64:
    return "double";
  default:
    TL2CGEN_LOG(FATAL) << "Unrecognized type: " << static_cast<int>(type);
    return "";
  }
}

/*!
 * \brief Look up the correct variant of exp() in C that should be used with a given type
 * \param info a type info
 * \return String representation
 */
inline std::string CExpForTypeInfo(treelite::TypeInfo type) {
  switch (type) {
  case treelite::TypeInfo::kInvalid:
  case treelite::TypeInfo::kUInt32:
    TL2CGEN_LOG(FATAL) << "Invalid type" << TypeInfoToString(type);
    return "";
  case treelite::TypeInfo::kFloat32:
    return "expf";
  case treelite::TypeInfo::kFloat64:
    return "exp";
  default:
    TL2CGEN_LOG(FATAL) << "Unrecognized type: " << static_cast<int>(type);
    return "";
  }
}

/*!
 * \brief Look up the correct variant of exp2() in C that should be used with a given type
 * \param info A type info
 * \return String representation
 */
inline std::string CExp2ForTypeInfo(treelite::TypeInfo type) {
  switch (type) {
  case treelite::TypeInfo::kInvalid:
  case treelite::TypeInfo::kUInt32:
    TL2CGEN_LOG(FATAL) << "Invalid type" << TypeInfoToString(type);
    return "";
  case treelite::TypeInfo::kFloat32:
    return "exp2f";
  case treelite::TypeInfo::kFloat64:
    return "exp2";
  default:
    TL2CGEN_LOG(FATAL) << "Unrecognized type: " << static_cast<int>(type);
    return "";
  }
}

/*!
 * \brief Look up the correct variant of copysign() in C that should be used with a given type
 * \param info A type info
 * \return String representation
 */
inline std::string CCopySignForTypeInfo(treelite::TypeInfo type) {
  switch (type) {
  case treelite::TypeInfo::kInvalid:
  case treelite::TypeInfo::kUInt32:
    TL2CGEN_LOG(FATAL) << "Invalid type" << TypeInfoToString(type);
    return "";
  case treelite::TypeInfo::kFloat32:
    return "copysignf";
  case treelite::TypeInfo::kFloat64:
    return "copysign";
  default:
    TL2CGEN_LOG(FATAL) << "Unrecognized type: " << static_cast<int>(type);
    return "";
  }
}

/*!
 * \brief Look up the correct variant of log1p() in C that should be used with a given type
 * \param info A type info
 * \return String representation
 */
inline std::string CLog1PForTypeInfo(treelite::TypeInfo type) {
  switch (type) {
  case treelite::TypeInfo::kInvalid:
  case treelite::TypeInfo::kUInt32:
    TL2CGEN_LOG(FATAL) << "Invalid type" << TypeInfoToString(type);
    return "";
  case treelite::TypeInfo::kFloat32:
    return "log1pf";
  case treelite::TypeInfo::kFloat64:
    return "log1p";
  default:
    TL2CGEN_LOG(FATAL) << "Unrecognized type: " << static_cast<int>(type);
    return "";
  }
}

}  // namespace tl2cgen::compiler::detail::templates

#endif  // TL2CGEN_DETAIL_COMPILER_TEMPLATES_TYPEINFO_H_
