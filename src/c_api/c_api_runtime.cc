/*!
 * Copyright (c) 2023 by Contributors
 * \file c_api_runtime.cc
 * \author Hyunsu Cho
 * \brief C API of TL2cgen, runtime package
 */

#include <tl2cgen/c_api_error.h>
#include <tl2cgen/c_api_runtime.h>
#include <tl2cgen/logging.h>
#include <tl2cgen/predictor.h>
#include <tl2cgen/predictor_types.h>
#include <tl2cgen/thread_local.h>

#include <cstddef>
#include <memory>
#include <string>

using namespace tl2cgen;  // NOLINT(build/namespaces)

namespace {

/*! \brief entry to to easily hold returning information */
struct TL2CgenRuntimeAPIThreadLocalEntry {
  /*! \brief result holder for returning string */
  std::string ret_str;
};

// thread-local store for returning strings
using TL2cgenRuntimeAPIThreadLocalStore = ThreadLocalStore<TL2CgenRuntimeAPIThreadLocalEntry>;

}  // anonymous namespace

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

int TL2cgenCreatePredictorOutputVector(TL2cgenPredictorHandle predictor, TL2cgenDMatrixHandle dmat,
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

int TL2cgenDeletePredictorOutputVector(TL2cgenPredictorOutputHandle output_vector) {
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

int TreelitePredictorQueryNumFeature(TL2cgenPredictorHandle predictor, size_t* out) {
  API_BEGIN();
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  *out = predictor_->QueryNumFeature();
  API_END();
}

int TreelitePredictorQueryPredTransform(TL2cgenPredictorHandle predictor, char const** out) {
  API_BEGIN()
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  auto pred_transform = predictor_->QueryPredTransform();
  std::string& ret_str = TL2cgenRuntimeAPIThreadLocalStore::Get()->ret_str;
  ret_str = pred_transform;
  *out = ret_str.c_str();
  API_END();
}

int TreelitePredictorQuerySigmoidAlpha(TL2cgenPredictorHandle predictor, float* out) {
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
  std::string& ret_str = TL2cgenRuntimeAPIThreadLocalStore::Get()->ret_str;
  ret_str = predictor_->QueryThresholdType();
  *out = ret_str.c_str();
  API_END();
}

int TL2cgenPredictorQueryLeafOutputType(TL2cgenPredictorHandle predictor, char const** out) {
  API_BEGIN()
  auto const* predictor_ = static_cast<predictor::Predictor const*>(predictor);
  std::string& ret_str = TL2cgenRuntimeAPIThreadLocalStore::Get()->ret_str;
  ret_str = predictor_->QueryLeafOutputType();
  *out = ret_str.c_str();
  API_END();
}

int TL2cgenPredictorFree(TL2cgenPredictorHandle predictor) {
  API_BEGIN();
  delete static_cast<predictor::Predictor*>(predictor);
  API_END();
}
