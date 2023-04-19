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
#include <cstddef>
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
TL2CGEN_DLL char const* TL2cgenGetLastError(void);

/*!
 * \brief Register callback function for LOG(INFO) messages -- helpful messages
 *        that are not errors.
 * Note: This function can be called by multiple threads. The callback function
 *       will run on the thread that registered it
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenRegisterLogCallback(void (*callback)(char const*));

/*!
 * \brief Register callback function for LOG(WARNING) messages
 * Note: This function can be called by multiple threads. The callback function
 *       will run on the thread that registered it
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenRegisterWarningCallback(void (*callback)(char const*));

/*!
 * \brief Get the version string for the TL2cgen library.
 * \return version string, of form MAJOR.MINOR.PATCH
 */
TL2CGEN_DLL char const* TL2cgenQueryTL2cgenVersion(void);

#ifdef __cplusplus
extern "C" {
extern char const* TL2CGEN_VERSION;
}
#else
extern char const* TL2CGEN_VERSION;
#endif

/*!
 * \defgroup dmatrix Data matrix interface
 * \{
 */
/*!
 * \brief Create DMatrix from a CSR matrix
 * \param data Feature values
 * \param data_type Type of data elements
 * \param col_ind Feature indices
 * \param row_ptr Pointer to row headers
 * \param num_row Number of rows
 * \param num_col Mumber of columns
 * \param out The created DMatrix
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenDMatrixCreateFromCSR(void const* data, char const* data_type,
    uint32_t const* col_ind, size_t const* row_ptr, size_t num_row, size_t num_col,
    TL2cgenDMatrixHandle* out);
/*!
 * \brief Create DMatrix from a dense matrix
 * \param data Feature values
 * \param data_type Type of data elements
 * \param num_row Number of rows
 * \param num_col Number of columns
 * \param missing_value Value to represent missing value
 * \param out The created DMatrix
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenDMatrixCreateFromMat(void const* data, char const* data_type, size_t num_row,
    size_t num_col, void const* missing_value, TL2cgenDMatrixHandle* out);
/*!
 * \brief Get dimensions of a DMatrix
 * \param handle Handle to DMatrix
 * \param out_num_row Used to set number of rows
 * \param out_num_col Used to set number of columns
 * \param out_nelem Used to set number of nonzero entries
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenDMatrixGetDimension(
    TL2cgenDMatrixHandle handle, size_t* out_num_row, size_t* out_num_col, size_t* out_nelem);

/*!
 * \brief Delete DMatrix from memory
 * \param handle Handle to DMatrix
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenDMatrixFree(TL2cgenDMatrixHandle handle);
/*! \} */

#endif  // TL2CGEN_C_API_COMMON_H_
