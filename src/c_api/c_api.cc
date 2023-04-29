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
#include <tl2cgen/data_matrix.h>
#include <tl2cgen/detail/filesystem.h>
#include <tl2cgen/logging.h>
#include <tl2cgen/predictor.h>
#include <tl2cgen/predictor_types.h>
#include <tl2cgen/thread_local.h>
#include <treelite/tree.h>

#include <cstddef>
#include <fstream>
#include <memory>
#include <string>

using namespace tl2cgen;  // NOLINT(build/namespaces)

namespace {

/*! \brief entry to to easily hold returning information */
struct TL2CgenAPIThreadLocalEntry {
  /*! \brief result holder for returning string */
  std::string ret_str;
};

// thread-local store for returning strings
using TL2cgenAPIThreadLocalStore = ThreadLocalStore<TL2CgenAPIThreadLocalEntry>;

}  // anonymous namespace

int TL2cgenRegisterLogCallback(void (*callback)(char const*)) {
  API_BEGIN();
  LogCallbackRegistry* registry = LogCallbackRegistryStore::Get();
  registry->RegisterCallBackLogInfo(callback);
  API_END();
}

int TL2cgenRegisterWarningCallback(void (*callback)(char const*)) {
  API_BEGIN();
  LogCallbackRegistry* registry = LogCallbackRegistryStore::Get();
  registry->RegisterCallBackLogWarning(callback);
  API_END();
}

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

int TL2cgenDMatrixCreateFromCSR(void const* data, char const* data_type_str,
    std::uint32_t const* col_ind, std::size_t const* row_ptr, std::size_t num_row,
    std::size_t num_col, TL2cgenDMatrixHandle* out) {
  API_BEGIN();
  std::unique_ptr<DMatrix> matrix = DMatrix::Create(DMatrixTypeEnum::kSparseCSR,
      DMatrixElementTypeFromString(data_type_str), data, col_ind, row_ptr, num_row, num_col);
  *out = static_cast<TL2cgenDMatrixHandle>(matrix.release());
  API_END();
}

int TL2cgenDMatrixCreateFromMat(void const* data, char const* data_type_str, std::size_t num_row,
    std::size_t num_col, void const* missing_value, TL2cgenDMatrixHandle* out) {
  API_BEGIN();
  std::unique_ptr<DMatrix> matrix = DMatrix::Create(DMatrixTypeEnum::kDenseCLayout,
      DMatrixElementTypeFromString(data_type_str), data, missing_value, num_row, num_col);
  *out = static_cast<TL2cgenDMatrixHandle>(matrix.release());
  API_END();
}

int TL2cgenDMatrixGetDimension(TL2cgenDMatrixHandle handle, std::size_t* out_num_row,
    std::size_t* out_num_col, std::size_t* out_nelem) {
  API_BEGIN();
  DMatrix const* dmat = static_cast<DMatrix*>(handle);
  *out_num_row = dmat->GetNumRow();
  *out_num_col = dmat->GetNumCol();
  *out_nelem = dmat->GetNumElem();
  API_END();
}

int TL2cgenDMatrixFree(TL2cgenDMatrixHandle handle) {
  API_BEGIN();
  delete static_cast<DMatrix*>(handle);
  API_END();
}

int TL2cgenPredictorLoad(
    char const* library_path, int num_worker_thread, TL2cgenPredictorHandle* out) {
  API_BEGIN();
  auto predictor = std::make_unique<predictor::Predictor>(library_path, num_worker_thread);
  *out = static_cast<TL2cgenPredictorHandle>(predictor.release());
  API_END();
}

int TL2cgenPredictorPredictBatch(TL2cgenPredictorHandle predictor, TL2cgenDMatrixHandle dmat,
    int verbose, int pred_margin, TL2cgenPredictorOutputHandle out_result,
    std::size_t* out_result_size) {
  API_BEGIN();
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  auto const* dmat_ = static_cast<DMatrix const*>(dmat);
  auto* out_buffer_ = static_cast<predictor::OutputBuffer*>(out_result);
  const size_t num_feature = predictor_->QueryNumFeature();
  const std::string err_msg = std::string(
                                  "Too many columns (features) in the data matrix. "
                                  "Number of features must not exceed ")
                              + std::to_string(num_feature);
  TL2CGEN_CHECK_LE(dmat_->GetNumCol(), num_feature) << err_msg;
  *out_result_size = predictor_->PredictBatch(dmat_, verbose, (pred_margin != 0), out_buffer_);
  API_END();
}

int TL2cgenPredictorCreateOutputVector(TL2cgenPredictorHandle predictor, TL2cgenDMatrixHandle dmat,
    TL2cgenPredictorOutputHandle* out_output_vector) {
  API_BEGIN();
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  auto const* dmat_ = static_cast<DMatrix const*>(dmat);
  std::unique_ptr<predictor::OutputBuffer> out_buffer = std::make_unique<predictor::OutputBuffer>(
      predictor::DataTypeFromString(predictor_->QueryLeafOutputType()),
      predictor_->QueryResultSize(dmat_));
  *out_output_vector = static_cast<TL2cgenPredictorOutputHandle>(out_buffer.release());
  API_END();
}

int TL2cgenPredictorGetRawPointerFromOutputVector(
    TL2cgenPredictorOutputHandle output_vector, void const** out_ptr) {
  API_BEGIN();
  auto* output_vector_ = static_cast<predictor::OutputBuffer*>(output_vector);
  *out_ptr = output_vector_->data();
  API_END();
}

int TL2cgenPredictorDeleteOutputVector(TL2cgenPredictorOutputHandle output_vector) {
  API_BEGIN();
  auto* output_vector_ = static_cast<predictor::OutputBuffer*>(output_vector);
  delete output_vector_;
  API_END();
}

int TL2cgenPredictorQueryResultSize(
    TL2cgenPredictorHandle predictor, TL2cgenDMatrixHandle dmat, std::size_t* out) {
  API_BEGIN();
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  auto const* dmat_ = static_cast<DMatrix const*>(dmat);
  *out = predictor_->QueryResultSize(dmat_);
  API_END();
}

int TL2cgenPredictorQueryNumClass(TL2cgenPredictorHandle predictor, size_t* out) {
  API_BEGIN();
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  *out = predictor_->QueryNumClass();
  API_END();
}

int TL2cgenPredictorQueryNumFeature(TL2cgenPredictorHandle predictor, size_t* out) {
  API_BEGIN();
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  *out = predictor_->QueryNumFeature();
  API_END();
}

int TL2cgenPredictorQueryPredTransform(TL2cgenPredictorHandle predictor, char const** out) {
  API_BEGIN()
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  auto pred_transform = predictor_->QueryPredTransform();
  std::string& ret_str = TL2cgenAPIThreadLocalStore::Get()->ret_str;
  ret_str = pred_transform;
  *out = ret_str.c_str();
  API_END();
}

int TL2cgenPredictorQuerySigmoidAlpha(TL2cgenPredictorHandle predictor, float* out) {
  API_BEGIN()
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  *out = predictor_->QuerySigmoidAlpha();
  API_END();
}

int TL2cgenPredictorQueryRatioC(TL2cgenPredictorHandle predictor, float* out) {
  API_BEGIN()
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  *out = predictor_->QueryRatioC();
  API_END();
}

int TL2cgenPredictorQueryGlobalBias(TL2cgenPredictorHandle predictor, float* out) {
  API_BEGIN()
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  *out = predictor_->QueryGlobalBias();
  API_END();
}

int TL2cgenPredictorQueryThresholdType(TL2cgenPredictorHandle predictor, char const** out) {
  API_BEGIN()
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  std::string& ret_str = TL2cgenAPIThreadLocalStore::Get()->ret_str;
  ret_str = predictor_->QueryThresholdType();
  *out = ret_str.c_str();
  API_END();
}

int TL2cgenPredictorQueryLeafOutputType(TL2cgenPredictorHandle predictor, char const** out) {
  API_BEGIN()
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  std::string& ret_str = TL2cgenAPIThreadLocalStore::Get()->ret_str;
  ret_str = predictor_->QueryLeafOutputType();
  *out = ret_str.c_str();
  API_END();
}

int TL2cgenPredictorFree(TL2cgenPredictorHandle predictor) {
  API_BEGIN();
  delete static_cast<predictor::Predictor*>(predictor);
  API_END();
}
