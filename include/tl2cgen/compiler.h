/*!
 * Copyright (c) 2023-2024 by Contributors
 * \file compiler.h
 * \brief Compiler that generates C code from a tree ensemble model
 * \author Hyunsu Cho
 */
#ifndef TL2CGEN_COMPILER_H_
#define TL2CGEN_COMPILER_H_

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace treelite {
class Model;  // forward declaration
}  // namespace treelite

namespace tl2cgen::compiler {

struct CompilerParam;  // forward declaration

/*!
 * \brief Compile tree model into C code
 * \param model Model to compile
 * \param param Parameters to control code generation
 * \param path Path to directory to store the generated C code (represented as human-readable text).
 *             Depending on the parameters, this directory may contain more than one source file.
 */
void CompileModel(
    treelite::Model const& model, CompilerParam const& param, std::filesystem::path const& dirpath);

/*!
 * \brief Obtain human-readable representation of Abstract Syntax Tree (AST) generated by
 *        the compiler. Useful for debugging the code generation process.
 * \param model Handle for tree ensemble model
 * \param compiler_params_json_str JSON string representing the parameters for the compiler
 * \return Text dump of AST
 */
std::string DumpAST(treelite::Model const& model, CompilerParam const& param);

}  // namespace tl2cgen::compiler

#endif  // TL2CGEN_COMPILER_H_
