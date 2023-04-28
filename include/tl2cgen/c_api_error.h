/*!
 * Copyright (c) 2023 by Contributors
 * \file c_api_error.h
 * \author Hyunsu Cho
 * \brief Error handling for C API.
 */
#ifndef TL2CGEN_C_API_ERROR_H_
#define TL2CGEN_C_API_ERROR_H_

#include <stdexcept>

/*! \brief macro to guard beginning and end section of all functions */
#define API_BEGIN() try {
/*! \brief every function starts with API_BEGIN();
     and finishes with API_END() or API_END_HANDLE_ERROR */
#define API_END()                               \
  }                                             \
  catch (std::exception & _except_) {           \
    return TL2cgenAPIHandleException(_except_); \
  }                                             \
  return 0
/*!
 * \brief every function starts with API_BEGIN();
 *   and finishes with API_END() or API_END_HANDLE_ERROR()
 *   "Finalize" contains procedure to cleanup states when an error happens
 */
#define API_END_HANDLE_ERROR(Finalize)           \
  }                                              \
  catch (std::exception & _except_) {            \
    Finalize;                                    \
    return TreeliteAPIHandleException(_except_); \
  }                                              \
  return 0

/*!
 * \brief Set the last error message needed by C API
 * \param msg The error message to set.
 */
void TL2cgenAPISetLastError(char const* msg);
/*!
 * \brief handle exception thrown out
 * \param e the exception
 * \return the return value of API after exception is handled
 */
inline int TL2cgenAPIHandleException(std::exception const& e) {
  TL2cgenAPISetLastError(e.what());
  return -1;
}
#endif  // TL2CGEN_C_API_ERROR_H_
