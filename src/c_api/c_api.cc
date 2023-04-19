/*!
 * Copyright (c) 2023 by Contributors
 * \file c_api.cc
 * \author Hyunsu Cho
 * \brief C API of TL2cgen, main package
 */

#include <tl2cgen/annotator.h>
#include <tl2cgen/c_api.h>
#include <tl2cgen/c_api_error.h>
#include <tl2cgen/compiler.h>
#include <tl2cgen/compiler_param.h>
#include <tl2cgen/detail/filesystem.h>
#include <tl2cgen/logging.h>
#include <treelite/tree.h>

#include <fstream>

using namespace tl2cgen;  // NOLINT(build/namespaces)

int TL2cgenAnnotateBranch(TL2cgenModelHandle model, TL2cgenDMatrixHandle dmat, int nthread,
    int verbose, TL2cgenAnnotationHandle* out) {
  API_BEGIN();
  std::unique_ptr<BranchAnnotator> annotator{new BranchAnnotator()};
  treelite::Model const* model_ = static_cast<treelite::Model*>(model);
  auto const* dmat_ = static_cast<DMatrix const*>(dmat);
  TL2CGEN_CHECK(dmat_) << "Found a dangling reference to DMatrix";
  annotator->Annotate(*model_, dmat_, nthread, verbose);
  *out = static_cast<TL2cgenAnnotationHandle>(annotator.release());
  API_END();
}

int TL2cgenAnnotationSave(TL2cgenAnnotationHandle handle, char const* path) {
  API_BEGIN();
  BranchAnnotator const* annotator = static_cast<BranchAnnotator*>(handle);
  std::ofstream fo(path);
  annotator->Save(fo);
  API_END();
}

int TL2cgenAnnotationFree(TL2cgenAnnotationHandle handle) {
  API_BEGIN();
  delete static_cast<BranchAnnotator*>(handle);
  API_END();
}

int TL2cgenCompilerCreate(
    char const* name, char const* params_json_str, TL2cgenCompilerHandle* out) {
  API_BEGIN();
  std::unique_ptr<Compiler> compiler{Compiler::Create(name, params_json_str)};
  *out = static_cast<TL2cgenCompilerHandle>(compiler.release());
  API_END();
}

int TL2cgenCompilerGenerateCode(
    TL2cgenCompilerHandle compiler, TL2cgenModelHandle model, char const* dirpath) {
  API_BEGIN();
  treelite::Model const* model_ = static_cast<treelite::Model*>(model);
  auto* compiler_ = static_cast<Compiler*>(compiler);
  TL2CGEN_CHECK(model_);
  TL2CGEN_CHECK(compiler_);
  compiler::CompilerParam param = compiler_->QueryParam();

  // create directory named dirpath
  std::string const& dirpath_(dirpath);
  detail::filesystem::CreateDirectoryIfNotExist(dirpath);

  /* compile model */
  auto compiled_model = compiler_->Compile(*model_);
  if (param.verbose > 0) {
    TL2CGEN_LOG(INFO) << "Code generation finished. Writing code to files...";
  }

  for (auto const& it : compiled_model.files) {
    if (param.verbose > 0) {
      TL2CGEN_LOG(INFO) << "Writing file " << it.first << "...";
    }
    const std::string filename_full = dirpath_ + "/" + it.first;
    if (it.second.is_binary) {
      detail::filesystem::WriteToFile(filename_full, it.second.content_binary);
    } else {
      detail::filesystem::WriteToFile(filename_full, it.second.content);
    }
  }
  API_END();
}

int TL2cgenCompilerFree(TL2cgenCompilerHandle handle) {
  API_BEGIN();
  delete static_cast<Compiler*>(handle);
  API_END();
}
