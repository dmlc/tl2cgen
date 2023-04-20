/*!
 * Copyright (c) 2023 by Contributors
 * \file c_api_runtime.h
 * \author Hyunsu Cho
 * \brief C API of TL2cgen, used for interfacing with other languages
 *        This header is used exclusively by the runtime
 */

/* Note: Make sure to use slash-asterisk form of comments in this file
   (like this one). Do not use double-slash (//). */

#ifndef TL2CGEN_C_API_RUNTIME_H_
#define TL2CGEN_C_API_RUNTIME_H_

#include "c_api_common.h"

/*!
 * \addtogroup opaque_handles
 * Opaque handles
 * \{
 */
/*! \brief Handle to predictor class */
typedef void* TL2cgenPredictorHandle;
/*! \brief Handle to output from predictor */
typedef void* TL2cgenPredictorOutputHandle;

/*! \} */

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
 *
 *        Note. This function does not allocate the result vector. Use
 *        TL2cgenCreatePredictorOutputVector() convenience function to allocate the vector of
 *        the right length and type.
 *
 *        Note. To access the element values from the output vector, you should cast the opaque
 *        handle (TL2cgenPredictorOutputHandle type) to an appropriate pointer LeafOutputType*,
 *        where the type is either float, double, or uint32_t. So carry out the following steps:
 *        1. Call TL2cgenPredictorQueryLeafOutputType() to obtain the type of the leaf output.
 *           It will return a string ("float32", "float64", or "uint32") representing the type.
 *        2. Depending on the type string, cast the output handle to float*, double*, or uint32_t*.
 *        3. Now access the array with the casted pointer. The array's length is given by
 *           TL2cgenPredictorQueryResultSize().
 * \param predictor Predictor
 * \param dmat The data matrix
 * \param verbose Whether to produce extra messages
 * \param pred_margin Whether to produce raw margin scores instead of
 *                    transformed probabilities
 * \param out_result Resulting output vector. This pointer must point to an array of length
 *                   TL2cgenPredictorQueryResultSize() and of type
 *                   TL2cgenPredictorQueryLeafOutputType().
 * \param out_result_size Used to save length of the output vector,
 *                        which is guaranteed to be less than or equal to
 *                        TL2cgenPredictorQueryResultSize()
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorPredictBatch(TL2cgenPredictorHandle predictor,
    TL2cgenDMatrixHandle dmat, int verbose, int pred_margin,
    TL2cgenPredictorOutputHandle out_result, size_t* out_result_size);

/*!
 * \brief Convenience function to allocate an output vector that is able to hold the prediction
 *        result for a given data matrix. The vector's length will be identical to
 *        TL2cgenPredictorQueryResultSize() and its type will be identical to
 *        TL2cgenPredictorQueryLeafOutputType(). To prevent memory leak, make sure to de-allocate
 *        the vector with TL2cgenDeletePredictorOutputVector().
 *
 *        Note. To access the element values from the output vector, you should cast the opaque
 *        handle (TL2cgenPredictorOutputHandle type) to an appropriate pointer LeafOutputType*,
 *        where the type is either float, double, or uint32_t. So carry out the following steps:
 *        1. Call TL2cgenPredictorQueryLeafOutputType() to obtain the type of the leaf output.
 *           It will return a string ("float32", "float64", or "uint32") representing the type.
 *        2. Depending on the type string, cast the output handle to float*, double*, or uint32_t*.
 *        3. Now access the array with the casted pointer. The array's length is given by
 *           TL2cgenPredictorQueryResultSize().
 * \param predictor Predictor
 * \param dmat The data matrix
 * \param out_output_vector Handle to the newly allocated output vector.
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenCreatePredictorOutputVector(TL2cgenPredictorHandle predictor,
    TL2cgenDMatrixHandle dmat, TL2cgenPredictorOutputHandle* out_output_vector);

/*!
 * \brief De-allocate an output vector
 * \param predictor Predictor
 * \param output_vector Output vector to delete from memory
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenDeletePredictorOutputVector(
    TL2cgenPredictorHandle predictor, TL2cgenPredictorOutputHandle output_vector);

/*!
 * \brief Given a data matrix, query the necessary size of array to hold predictions for all rows.
 * \param predictor Predictor
 * \param dmat The data matrix
 * \param out Used to store the length of prediction array
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorQueryResultSize(
    TL2cgenPredictorHandle predictor, TL2cgenDMatrixHandle dmat, size_t* out);

/*!
 * \brief Get the number classes in the loaded model. The number is 1 for most tasks;
 *        it is greater than 1 for multiclass classification.
 * \param predictor Predictor
 * \param out Length of prediction array
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorQueryNumClass(TL2cgenPredictorHandle predictor, size_t* out);

/*!
 * \brief Get the number of columns (features) in the training data
 * \param predictor Predictor
 * \param out Number of features
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorQueryNumFeature(TL2cgenPredictorHandle predictor, size_t* out);

/*!
 * \brief Get name of post prediction transformation used to train the loaded model
 * \param predictor Predictor
 * \param out Name of post prediction transformation
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorQueryPredTransform(
    TL2cgenPredictorHandle predictor, char const** out);

/*!
 * \brief Get alpha value of sigmoid transformation used to train the loaded model
 * \param predictor Predictor
 * \param out Alpha value of sigmoid transformation
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorQuerySigmoidAlpha(TL2cgenPredictorHandle predictor, float* out);

/*!
 * \brief Get D value of exponential standard ratio transformation used to train
 *        the loaded model
 * \param predictor Predictor
 * \param out C value of transformation
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorQueryRatioC(TL2cgenPredictorHandle predictor, float* out);

/*!
 * \brief Get global bias which adjusting predicted margin scores
 * \param predictor Predictor
 * \param out Global bias value
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorQueryGlobalBias(TL2cgenPredictorHandle predictor, float* out);

/*!
 * \brief Query the threshold type of the model
 * \param predictor Predictor
 * \param out String indicating the threshold type
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorQueryThresholdType(
    TL2cgenPredictorHandle predictor, char const** out);

/*!
 * \brief Query the leaf output type of the model
 * \param predictor Predictor
 * \param out String indicating the leaf output type
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorQueryLeafOutputType(
    TL2cgenPredictorHandle predictor, char const** out);

/*!
 * \brief Delete predictor from memory
 * \param predictor Predictor to remove
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenPredictorFree(TL2cgenPredictorHandle predictor);
/*! \} */

#endif  // TL2CGEN_C_API_RUNTIME_H_
