/*!
 * Copyright (c) 2023 by Contributors
 * \file error.h
 * \brief Exception class used throughout the TL2cgen codebase
 * \author Hyunsu Cho
 */
#ifndef TL2CGEN_ERROR_H_
#define TL2CGEN_ERROR_H_

#include <stdexcept>
#include <string>

namespace tl2cgen {

/*!
 * \brief Exception class that will be thrown by TL2cgen
 */
struct Error : public std::runtime_error {
  explicit Error(std::string const& s) : std::runtime_error(s) {}
};

}  // namespace tl2cgen

#endif  // TL2CGEN_ERROR_H_
