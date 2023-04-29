/*!
 * Copyright (c) 2023 by Contributors
 * \file predictor.cc
 * \author Hyunsu Cho
 * \brief Load prediction function exported as a shared library
 */

#include <tl2cgen/data_matrix.h>
#include <tl2cgen/detail/threading_utils/omp_config.h>
#include <tl2cgen/detail/threading_utils/parallel_for.h>
#include <tl2cgen/logging.h>
#include <tl2cgen/predictor.h>

#include <chrono>
#include <cstddef>
#include <memory>

namespace {

inline double GetTime() {
  return std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

inline std::vector<std::size_t> SplitBatch(tl2cgen::DMatrix const* dmat, std::size_t split_factor) {
  const std::size_t num_row = dmat->GetNumRow();
  TL2CGEN_CHECK_LE(split_factor, num_row);
  const std::size_t portion = num_row / split_factor;
  const std::size_t remainder = num_row % split_factor;
  std::vector<std::size_t> workload(split_factor, portion);
  std::vector<std::size_t> row_ptr(split_factor + 1, 0);
  for (std::size_t i = 0; i < remainder; ++i) {
    ++workload[i];
  }
  std::size_t accum = 0;
  for (std::size_t i = 0; i < split_factor; ++i) {
    accum += workload[i];
    row_ptr[i + 1] = accum;
  }
  return row_ptr;
}

}  // anonymous namespace

namespace tl2cgen::predictor {

Predictor::Predictor(char const* libpath, int num_worker_thread) {
  thread_config_ = tl2cgen::detail::threading_utils::ConfigureThreadConfig(num_worker_thread);
  lib_ = std::make_unique<detail::SharedLibrary>(libpath);

  using UnsignedQueryFunc = std::size_t (*)();
  using StringQueryFunc = char const* (*)();
  using FloatQueryFunc = float (*)();

  /* 1. query # of output groups */
  auto* num_class_query_func = lib_->LoadFunctionWithSignature<UnsignedQueryFunc>("get_num_class");
  num_class_ = num_class_query_func();

  /* 2. query # of features */
  auto* num_feature_query_func
      = lib_->LoadFunctionWithSignature<UnsignedQueryFunc>("get_num_feature");
  num_feature_ = num_feature_query_func();
  TL2CGEN_CHECK_GT(num_feature_, 0) << "num_feature cannot be zero";

  /* 3. query # of pred_transform name */
  auto* pred_transform_query_func
      = lib_->LoadFunctionWithSignature<StringQueryFunc>("get_pred_transform");
  pred_transform_ = pred_transform_query_func();

  /* 4. query # of sigmoid_alpha */
  auto* sigmoid_alpha_query_func
      = lib_->LoadFunctionWithSignature<FloatQueryFunc>("get_sigmoid_alpha");
  sigmoid_alpha_ = sigmoid_alpha_query_func();

  /* 5. query # of ratio_c */
  auto* ratio_c_query_func = lib_->LoadFunctionWithSignature<FloatQueryFunc>("get_ratio_c");
  ratio_c_ = ratio_c_query_func();

  /* 6. query # of global_bias */
  auto* global_bias_query_func = lib_->LoadFunctionWithSignature<FloatQueryFunc>("get_global_bias");
  global_bias_ = global_bias_query_func();

  /* 7. Query the data type for thresholds and leaf outputs */
  auto* threshold_type_query_func
      = lib_->LoadFunctionWithSignature<StringQueryFunc>("get_threshold_type");
  threshold_type_ = threshold_type_query_func();
  auto* leaf_output_type_query_func
      = lib_->LoadFunctionWithSignature<StringQueryFunc>("get_leaf_output_type");
  leaf_output_type_ = leaf_output_type_query_func();

  /* 8. load appropriate function for margin prediction */
  TL2CGEN_CHECK_GT(num_class_, 0) << "num_class cannot be zero";
  pred_func_ = std::make_unique<PredictFunction>(DataTypeFromString(threshold_type_),
      DataTypeFromString(leaf_output_type_), *lib_, static_cast<int>(num_feature_),
      static_cast<int>(num_class_));
}

std::size_t Predictor::PredictBatch(
    DMatrix const* dmat, int verbose, bool pred_margin, OutputBuffer* out_result) const {
  const std::size_t num_row = dmat->GetNumRow();
  if (num_row == 0) {
    return 0;
  }
  double const tstart = GetTime();
  // Reduce nthread if n_row is small
  const std::size_t nthread = std::min(static_cast<std::size_t>(thread_config_.nthread), num_row);
  const std::vector<std::size_t> row_ptr = SplitBatch(dmat, nthread);
  std::size_t total_size = 0;
  std::vector<std::size_t> result_size(nthread);
  tl2cgen::detail::threading_utils::ParallelFor(std::size_t(0), nthread, thread_config_,
      tl2cgen::detail::threading_utils::ParallelSchedule::Static(),
      [&](std::size_t thread_id, int) {
        std::size_t rbegin = row_ptr[thread_id];
        std::size_t rend = row_ptr[thread_id + 1];
        result_size[thread_id]
            = pred_func_->PredictBatch(dmat, rbegin, rend, pred_margin, out_result);
      });
  for (auto e : result_size) {
    total_size += e;
  }
  // Re-shape output if total_size < dimension of out_result
  if (total_size < QueryResultSize(dmat, 0, num_row)) {
    TL2CGEN_CHECK_GT(num_class_, 1);
    TL2CGEN_CHECK_EQ(total_size % num_row, 0);
    const std::size_t query_size_per_instance = total_size / num_row;
    TL2CGEN_CHECK_GT(query_size_per_instance, 0);
    TL2CGEN_CHECK_LT(query_size_per_instance, num_class_);
    std::visit(
        [&, num_class = num_class_](auto&& pred_func_concrete, auto&& out_pred_ptr) {
          using LeafOutputType =
              typename std::remove_reference_t<decltype(pred_func_concrete)>::leaf_output_type;
          using ExpectedLeafOutputType
              = std::remove_pointer_t<std::remove_reference_t<decltype(out_pred_ptr)>>;
          if constexpr (std::is_same_v<LeafOutputType, ExpectedLeafOutputType>) {
            auto* out_result_ = static_cast<LeafOutputType*>(out_result->data());
            for (std::size_t rid = 0; rid < num_row; ++rid) {
              for (std::size_t k = 0; k < query_size_per_instance; ++k) {
                out_result_[rid * query_size_per_instance + k] = out_result_[rid * num_class + k];
              }
            }
          } else {
            TL2CGEN_LOG(FATAL)
                << "Type mismatch between LeafOutputType of the model and the output buffer. "
                << "LeafOutputType = " << typeid(LeafOutputType).name()
                << ", ExpectedLeafOutputType = " << typeid(ExpectedLeafOutputType).name();
          }
        },
        pred_func_->variant_, out_result->variant_);
  }
  double const tend = GetTime();
  if (verbose > 0) {
    TL2CGEN_LOG(INFO) << "TL2cgen: Finished prediction in " << (tend - tstart) << " sec";
  }
  return total_size;
}

}  // namespace tl2cgen::predictor
