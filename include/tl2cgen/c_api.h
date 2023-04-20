/*!
 * Copyright (c) 2023 by Contributors
 * \file c_api.h
 * \author Hyunsu Cho
 * \brief C API of TL2cgen, used for interfacing with other languages
 *        This header is excluded from the runtime
 */

/* Note: Make sure to use slash-asterisk form of comments in this file
   (like this one). Do not use double-slash (//). */

#ifndef TL2CGEN_C_API_H_
#define TL2CGEN_C_API_H_

#include <tl2cgen/c_api_common.h>

/*!
 * \addtogroup opaque_handles
 * Opaque handles
 * \{
 */
/*! \brief Handle to a decision tree ensemble model */
typedef void* TL2cgenModelHandle;
/*! \brief Handle to branch annotation data */
typedef void* TL2cgenAnnotationHandle;
/*! \brief Handle to compiler class */
typedef void* TL2cgenCompilerHandle;
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
 * \brief Create a compiler with a given name
 * \param name Name of compiler
 * \param params_json_str JSON string representing the parameters for the compiler
 * \param out Created compiler
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenCompilerCreate(
    char const* name, char const* params_json_str, TL2cgenCompilerHandle* out);
/*!
 * \brief Generate prediction code from a tree ensemble model. The code will
 *        be C99 compliant. One header file (.h) will be generated, along with
 *        one or more source files (.c).
 *
 * Usage example:
 * \code
 *   TL2cgenCompilerGenerateCode(compiler, model, "./my/model");
 *   // files to generate: ./my/model/header.h, ./my/model/main.c
 *   // if parallel compilation is enabled:
 *   // ./my/model/header.h, ./my/model/main.c, ./my/model/tu0.c,
 *   // ./my/model/tu1.c, and so forth
 * \endcode
 * \param compiler Handle for compiler
 * \param model Handle for tree ensemble model
 * \param dirpath Directory to store header and source files
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenCompilerGenerateCode(
    TL2cgenCompilerHandle compiler, TL2cgenModelHandle model, char const* dirpath);
/*!
 * \brief Delete compiler from memory
 * \param handle Compiler to remove
 * \return 0 for success, -1 for failure
 */
TL2CGEN_DLL int TL2cgenCompilerFree(TL2cgenCompilerHandle handle);
/*! \} */

#endif /* TL2CGEN_C_API_H_ */
