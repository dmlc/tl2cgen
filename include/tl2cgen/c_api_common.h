/*!
 * Copyright (c) 2023 by Contributors
 * \file c_api_common.h
 * \author Hyunsu Cho
 * \brief C API of TL2cgen, used for interfacing with other languages
 *        This header is used by both the runtime and the main package
 */

#ifndef TL2CGEN_C_API_COMMON_H_
#define TL2CGEN_C_API_COMMON_H_

#ifdef __cplusplus
#define TL2CGEN_EXTERN_C extern "C"
#include <cstdint>
#include <cstdio>
#else
#define TL2CGEN_EXTERN_C
#include <stdint.h>
#include <stdio.h>
#endif

/* special symbols for DLL library on Windows */
#if defined(_MSC_VER) || defined(_WIN32)
#define TL2CGEN_DLL TL2CGEN_EXTERN_C __declspec(dllexport)
#else
#define TL2CGEN_DLL TL2CGEN_EXTERN_C
#endif

/*! \brief handle to a data matrix */
typedef void* TL2cgenDMatrixHandle;

/*!
 * \brief display last error; can be called by multiple threads
 * Note. Each thread will get the last error occurred in its own context.
 * \return error string
 */
TL2CGEN_DLL const char* TL2cgenGetLastError(void);

/*!
 * \brief Register callback function for LOG(INFO) messages -- helpful messages
 *        that are not errors.
 * Note: This function can be called by multiple threads. The callback function
 *       will run on the thread that registered it
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenRegisterLogCallback(void (*callback)(const char*));

/*!
 * \brief Register callback function for LOG(WARNING) messages
 * Note: This function can be called by multiple threads. The callback function
 *       will run on the thread that registered it
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenRegisterWarningCallback(void (*callback)(const char*));

/*!
 * \brief Get the version string for the TL2cgen library.
 * \return version string, of form MAJOR.MINOR.PATCH
 */
TL2CGEN_DLL const char* TL2cgenQueryTL2cgenVersion(void);

#ifdef __cplusplus
extern "C" {
extern const char* TREELITE_VERSION;
}
#else
extern const char* TREELITE_VERSION;
#endif

#endif  // TL2CGEN_C_API_COMMON_H_
