/*!
 * Copyright (c) 2023 by Contributors
 * \file ast_native.h
 * \brief C code generator
 * \author Hyunsu Cho
 */

#ifndef TL2CGEN_DETAIL_COMPILER_AST_NATIVE_H_
#define TL2CGEN_DETAIL_COMPILER_AST_NATIVE_H_

#include <tl2cgen/compiler.h>
#include <tl2cgen/compiler_param.h>

#include <memory>

namespace tl2cgen::compiler::detail {

class ASTNativeCompilerImpl;

class ASTNativeCompiler : public Compiler {
 public:
  explicit ASTNativeCompiler(CompilerParam const& param);
  ~ASTNativeCompiler() override;
  CompiledModel Compile(treelite::Model const& model) override;
  CompilerParam QueryParam() const override;

 private:
  std::unique_ptr<ASTNativeCompilerImpl> pimpl_;
};

}  // namespace tl2cgen::compiler::detail

#endif  // TL2CGEN_DETAIL_COMPILER_AST_NATIVE_H_
