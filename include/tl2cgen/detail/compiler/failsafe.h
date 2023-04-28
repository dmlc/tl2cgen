/*!
 * Copyright (c) 2023 by Contributors
 * \file failsafe.h
 * \brief C code generator (fail-safe). The generated code will mimic prediction logic found in
 *        XGBoost
 * \author Hyunsu Cho
 */

#ifndef TL2CGEN_DETAIL_COMPILER_FAILSAFE_H_
#define TL2CGEN_DETAIL_COMPILER_FAILSAFE_H_

#include <tl2cgen/compiler.h>
#include <tl2cgen/compiler_param.h>

#include <memory>

namespace tl2cgen::compiler::detail {

class FailSafeCompilerImpl;

class FailSafeCompiler : public Compiler {
 public:
  explicit FailSafeCompiler(CompilerParam const& param);
  ~FailSafeCompiler() override;
  CompiledModel Compile(treelite::Model const& model) override;
  CompilerParam QueryParam() const override;

 private:
  std::unique_ptr<FailSafeCompilerImpl> pimpl_;
};

}  // namespace tl2cgen::compiler::detail

#endif  // TL2CGEN_DETAIL_COMPILER_FAILSAFE_H_
