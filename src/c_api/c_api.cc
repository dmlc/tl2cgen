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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

using namespace tl2cgen;  // NOLINT(build/namespaces)

namespace {

/*! \brief Entry to to easily hold returning information */
struct TL2CgenAPIThreadLocalEntry {
  /*! \brief Result holder for returning string */
  std::string ret_str;
  /*! \brief Result holder for returning dimensions */
  std::vector<std::uint64_t> ret_shape;
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

int TL2cgenGenerateCode(
    TL2cgenModelHandle model, char const* compiler_params_json_str, char const* dirpath) {
  API_BEGIN();
  treelite::Model const* model_ = static_cast<treelite::Model*>(model);
  TL2CGEN_CHECK(model_);

  std::filesystem::path dirpath_
      = std::filesystem::weakly_canonical(std::filesystem::u8path(std::string(dirpath)));

  /* Compile model */
  auto param = compiler::CompilerParam::ParseFromJSON(compiler_params_json_str);
  compiler::CompileModel(*model_, param, dirpath_);
  API_END();
}

TL2CGEN_DLL int TL2cgenDumpAST(
    TL2cgenModelHandle model, char const* compiler_params_json_str, char const** out_dump_str) {
  API_BEGIN();
  treelite::Model const* model_ = static_cast<treelite::Model*>(model);
  TL2CGEN_CHECK(model_);
  auto param = compiler::CompilerParam::ParseFromJSON(compiler_params_json_str);
  std::string& ret_str = TL2cgenAPIThreadLocalStore::Get()->ret_str;
  ret_str = compiler::DumpAST(*model_, param);
  *out_dump_str = ret_str.c_str();
  API_END();
}

int TL2cgenDMatrixCreateFromCSR(void const* data, char const* data_type_str,
    std::uint32_t const* col_ind, std::uint64_t const* row_ptr, std::uint64_t num_row,
    std::uint64_t num_col, TL2cgenDMatrixHandle* out) {
  API_BEGIN();
  std::unique_ptr<DMatrix> matrix = DMatrix::Create(DMatrixTypeEnum::kSparseCSR,
      DMatrixElementTypeFromString(data_type_str), data, col_ind, row_ptr, num_row, num_col);
  *out = static_cast<TL2cgenDMatrixHandle>(matrix.release());
  API_END();
}

int TL2cgenDMatrixCreateFromMat(void const* data, char const* data_type_str, std::uint64_t num_row,
    std::uint64_t num_col, void const* missing_value, TL2cgenDMatrixHandle* out) {
  API_BEGIN();
  std::unique_ptr<DMatrix> matrix = DMatrix::Create(DMatrixTypeEnum::kDenseCLayout,
      DMatrixElementTypeFromString(data_type_str), data, missing_value, num_row, num_col);
  *out = static_cast<TL2cgenDMatrixHandle>(matrix.release());
  API_END();
}

int TL2cgenDMatrixGetDimension(TL2cgenDMatrixHandle handle, std::uint64_t* out_num_row,
    std::uint64_t* out_num_col, std::uint64_t* out_nelem) {
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
    int verbose, int pred_margin, void* out_result) {
  API_BEGIN();
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  auto const* dmat_ = static_cast<DMatrix const*>(dmat);
  std::size_t const num_feature = predictor_->GetNumFeature();
  std::string const err_msg = std::string(
                                  "Too many columns (features) in the data matrix. "
                                  "Number of features must not exceed ")
                              + std::to_string(num_feature);
  TL2CGEN_CHECK_LE(dmat_->GetNumCol(), num_feature) << err_msg;
  predictor_->PredictBatch(dmat_, verbose, (pred_margin != 0), out_result);
  API_END();
}

int TL2cgenPredictorGetOutputShape(TL2cgenPredictorHandle predictor, TL2cgenDMatrixHandle dmat,
    std::uint64_t const** out_shape, std::uint64_t* out_ndim) {
  API_BEGIN();
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  auto const* dmat_ = static_cast<DMatrix const*>(dmat);
  std::vector<std::uint64_t>& ret_shape = TL2cgenAPIThreadLocalStore::Get()->ret_shape;
  ret_shape = predictor_->GetOutputShape(dmat_);
  *out_shape = ret_shape.data();
  *out_ndim = ret_shape.size();
  API_END();
}

int TL2cgenPredictorGetThresholdType(TL2cgenPredictorHandle predictor, char const** out) {
  API_BEGIN()
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  std::string& ret_str = TL2cgenAPIThreadLocalStore::Get()->ret_str;
  ret_str = predictor_->GetThresholdType();
  *out = ret_str.c_str();
  API_END();
}

int TL2cgenPredictorGetLeafOutputType(TL2cgenPredictorHandle predictor, char const** out) {
  API_BEGIN()
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  std::string& ret_str = TL2cgenAPIThreadLocalStore::Get()->ret_str;
  ret_str = predictor_->GetLeafOutputType();
  *out = ret_str.c_str();
  API_END();
}

int TL2cgenPredictorGetNumFeature(TL2cgenPredictorHandle predictor, std::int32_t* out) {
  API_BEGIN();
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  *out = predictor_->GetNumFeature();
  API_END();
}

int TL2cgenPredictorGetNumTarget(TL2cgenPredictorHandle predictor, int32_t* out) {
  API_BEGIN();
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  *out = predictor_->GetNumTarget();
  API_END();
}

int TL2cgenPredictorGetNumClass(TL2cgenPredictorHandle predictor, int32_t* out) {
  API_BEGIN();
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  auto num_class = predictor_->GetNumClass();
  std::copy(num_class.begin(), num_class.end(), out);
  API_END();
}

int TL2cgenPredictorFree(TL2cgenPredictorHandle predictor) {
  API_BEGIN();
  delete static_cast<predictor::Predictor*>(predictor);
  API_END();
}
