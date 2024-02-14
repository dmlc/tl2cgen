/*!
 * Copyright (c) 2023 by Contributors
 * \file c_api.h
 * \author Hyunsu Cho
 * \brief C API of TL2cgen, used for interfacing with other languages
 */

/* Note: Make sure to use slash-asterisk form of comments in this file
   (like this one). Do not use double-slash (//). */

#ifndef TL2CGEN_C_API_H_
#define TL2CGEN_C_API_H_

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
#define TL2CGEN_DLL TL2CGEN_EXTERN_C __attribute__((visibility("default")))
#endif

/*!
 * \defgroup opaque_handles
 * Opaque handles
 * \{
 */
/*! \brief Handle to a decision tree ensemble model */
typedef void* TL2cgenModelHandle;
/*! \brief Handle to branch annotation data */
typedef void* TL2cgenAnnotationHandle;
/*! \brief Handle to a data matrix */
typedef void* TL2cgenDMatrixHandle;
/*! \brief Handle to predictor class */
typedef void* TL2cgenPredictorHandle;
/*! \} */

/*!
 * \defgroup logging
 * Logging and error handling functions
 * \{
 */
/*!
 * \brief Display last error; can be called by multiple threads
 * Note. Each thread will get the last error occurred in its own context.
 * \return Error string
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
 * \return Version string, of form MAJOR.MINOR.PATCH
 */
TL2CGEN_DLL char const* TL2cgenQueryTL2cgenVersion(void);
/*! \} */

#ifdef __cplusplus
extern "C" {
extern char const* TL2CGEN_VERSION;
}
#else
extern char const* TL2CGEN_VERSION;
#endif

/*!
 * \defgroup model_loader Model loader interface: read Treelite model object
 * \{
 */
/*!
 * \brief Load Treelite model object from a byte sequence
 * \param treelite_model_bytes Byte sequence containing serialized Treelite model object.
 * \param treelite_model_bytes_len Length of treelite_model_bytes
 * \param out Loaded model object
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenLoadTreeliteModelFromBytes(
    char const* treelite_model_bytes, size_t treelite_model_bytes_len, TL2cgenModelHandle* out);
/*!
 * \brief Query the Treelite version that produce a given Treelite model object.
 * \param model Model object
 * \param major_ver Major version
 * \param minor_ver Minor version
 * \param patch_ver Patch version
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenQueryTreeliteModelVersion(TL2cgenModelHandle model, std::int32_t* major_ver,
    std::int32_t* minor_ver, std::int32_t* patch_ver);
/*!
 * \brief Unload Treelite model object from memory
 * \param model Model object to free
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenFreeTreeliteModel(TL2cgenModelHandle model);
/*! \} */

/*!
 * \defgroup annotator Branch annotator interface
 * \{
 */
/*!
 * \brief Annotate branches in a given model using frequency patterns in the
 *        training data.
 * \param model Model to annotate
 * \param dmat Training data matrix
 * \param nthread Number of threads to use
 * \param verbose Whether to produce extra messages
 * \param out Used to save handle for the created annotation
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenAnnotateBranch(TL2cgenModelHandle model, TL2cgenDMatrixHandle dmat,
    int nthread, int verbose, TL2cgenAnnotationHandle* out);
/*!
 * \brief Save branch annotation to a JSON file
 * \param handle Annotation to save
 * \param path Path to JSON file
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenAnnotationSave(TL2cgenAnnotationHandle handle, char const* path);
/*!
 * \brief Delete branch annotation from memory
 * \param handle Annotation to remove
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenAnnotationFree(TL2cgenAnnotationHandle handle);
/*! \} */

/*!
 * \defgroup compiler Compiler interface
 * \{
 */
/*!
 * \brief Generate prediction code from a tree ensemble model. The code will
 *        be C99 compliant. One header file (.h) will be generated, along with
 *        one or more source files (.c).
 *
 * Usage example:
 * \code
 *   TL2cgenCompilerGenerateCode(model, compiler_params_json_str, "./my/model");
 *   // files to generate: ./my/model/header.h, ./my/model/main.c
 *   // if parallel compilation is enabled:
 *   // ./my/model/header.h, ./my/model/main.c, ./my/model/tu0.c,
 *   // ./my/model/tu1.c, and so forth
 * \endcode
 * \param model Handle for tree ensemble model
 * \param compiler_params_json_str JSON string representing the parameters for the compiler
 * \param dirpath Directory to store header and source files
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenGenerateCode(
    TL2cgenModelHandle model, char const* compiler_params_json_str, char const* dirpath);
/*!
 * \brief Obtain human-readable representation of Abstract Syntax Tree (AST) generated by
 *        the compiler. Useful for debugging the code generation process.
 * \param model Handle for tree ensemble model
 * \param compiler_params_json_str JSON string representing the parameters for the compiler
 * \param out_dump_str Pointer to store the returned string
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenDumpAST(
    TL2cgenModelHandle model, char const* compiler_params_json_str, char const** out_dump_str);
/*! \} */

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
    uint32_t const* col_ind, uint64_t const* row_ptr, uint64_t num_row, uint64_t num_col,
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
TL2CGEN_DLL int TL2cgenDMatrixCreateFromMat(void const* data, char const* data_type,
    uint64_t num_row, uint64_t num_col, void const* missing_value, TL2cgenDMatrixHandle* out);
/*!
 * \brief Get dimensions of a DMatrix
 * \param handle Handle to DMatrix
 * \param out_num_row Used to set number of rows
 * \param out_num_col Used to set number of columns
 * \param out_nelem Used to set number of nonzero entries
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenDMatrixGetDimension(
    TL2cgenDMatrixHandle handle, uint64_t* out_num_row, uint64_t* out_num_col, uint64_t* out_nelem);

/*!
 * \brief Delete DMatrix from memory
 * \param handle Handle to DMatrix
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenDMatrixFree(TL2cgenDMatrixHandle handle);
/*! \} */

/*!
 * \defgroup predictor Predictor interface
 * \{
 */
/*!
 * \brief Load prediction code into memory.
 * This function assumes that the prediction code has been already compiled into
 * a dynamic shared library object (.so/.dll/.dylib).
 * \param library_path Path to library object file containing prediction code
 * \param num_worker_thread Number of worker threads (<= 0 to use max number)
 * \param out Handle to predictor
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorLoad(
    char const* library_path, int num_worker_thread, TL2cgenPredictorHandle* out);

/*!
 * \brief Make predictions for a data matrix (synchronously). This function internally
 *        divides the workload among all worker threads.
 * \param predictor Predictor
 * \param dmat Data matrix
 * \param verbose Whether to produce extra messages
 * \param pred_margin Whether to produce raw margin scores instead of
 *                    transformed probabilities
 * \param out_result Resulting output vector. This pointer must point to an array of shape
 *                   \ref TL2cgenPredictorGetOutputShape and of type
 *                   \ref TL2cgenPredictorGetLeafOutputType.
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorPredictBatch(TL2cgenPredictorHandle predictor,
    TL2cgenDMatrixHandle dmat, int verbose, int pred_margin, void* out_result);

/*!
 * \brief Given a data matrix, get the output shape of array to hold predictions for all rows.
 * \param predictor Predictor
 * \param dmat Data matrix
 * \param out_shape Array of dimensions
 * \param out_ndim Number of dimensions in out_shape
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorGetOutputShape(TL2cgenPredictorHandle predictor,
    TL2cgenDMatrixHandle dmat, uint64_t const** out_shape, uint64_t* out_ndim);

/*!
 * \brief Get the threshold type of the model
 * \param predictor Predictor
 * \param out String indicating the threshold type
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorGetThresholdType(
    TL2cgenPredictorHandle predictor, char const** out);

/*!
 * \brief Get the leaf output type of the model
 * \param predictor Predictor
 * \param out String indicating the leaf output type
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorGetLeafOutputType(
    TL2cgenPredictorHandle predictor, char const** out);

/*!
 * \brief Get the number of features used in the model
 * \param predictor Predictor
 * \param out Number of features
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorGetNumFeature(TL2cgenPredictorHandle predictor, int32_t* out);

/*!
 * \brief Get the number of output targets
 * \param predictor Predictor
 * \param out Number of output targets
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorGetNumTarget(TL2cgenPredictorHandle predictor, int32_t* out);

/*!
 * \brief Get the number of classes per output target.
 * \param predictor Predictor
 * \param out Array to store number of classes per output target, where out[i] indicates the number
 *            of classes for i-th target. Make sure to pass in the array of length (num_target).
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorGetNumClass(TL2cgenPredictorHandle predictor, int32_t* out);

/*!
 * \brief Delete predictor from memory
 * \param predictor Predictor to remove
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorFree(TL2cgenPredictorHandle predictor);
/*! \} */

#endif /* TL2CGEN_C_API_H_ */
