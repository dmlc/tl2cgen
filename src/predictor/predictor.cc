/*!
 * Copyright (c) 2023 by Contributors
 * \file predictor.cc
 * \author Hyunsu Cho
 * \brief Load prediction function exported as a shared library
 */

#include <tl2cgen/data_matrix.h>
#include <tl2cgen/detail/math_funcs.h>
#include <tl2cgen/detail/threading_utils/omp_config.h>
#include <tl2cgen/detail/threading_utils/parallel_for.h>
#include <tl2cgen/logging.h>
#include <tl2cgen/predictor.h>
#include <tl2cgen/predictor_types.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <experimental/mdspan>
#include <memory>

namespace {

using tl2cgen::predictor::detail::Entry;
namespace stdex = std::experimental;
template <typename ElemT>
using Array2DView = stdex::mdspan<ElemT, stdex::dextents<std::uint64_t, 2>, stdex::layout_right>;
template <typename ElemT>
using Array3DView = stdex::mdspan<ElemT, stdex::dextents<std::uint64_t, 3>, stdex::layout_right>;

inline double GetTime() {
  return std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

inline std::vector<std::uint64_t> SplitBatch(
    tl2cgen::DMatrix const* dmat, std::uint64_t split_factor) {
  std::uint64_t const num_row = dmat->GetNumRow();
  TL2CGEN_CHECK_LE(split_factor, num_row);
  std::uint64_t const portion = num_row / split_factor;
  std::uint64_t const remainder = num_row % split_factor;
  std::vector<std::uint64_t> workload(split_factor, portion);
  std::vector<std::uint64_t> row_ptr(split_factor + 1, 0);
  for (std::uint64_t i = 0; i < remainder; ++i) {
    ++workload[i];
  }
  std::uint64_t accum = 0;
  for (std::uint64_t i = 0; i < split_factor; ++i) {
    accum += workload[i];
    row_ptr[i + 1] = accum;
  }
  return row_ptr;
}

template <typename ThresholdType, typename LeafOutputType, typename ElementType, typename PredFunc>
inline void ApplyBatch(tl2cgen::CSRDMatrix<ElementType> const* dmat, int num_feature,
    std::uint64_t rbegin, std::uint64_t rend, bool pred_margin,
    Array3DView<LeafOutputType> output_view, PredFunc func) {
  TL2CGEN_CHECK_LE(dmat->num_col_, static_cast<std::uint64_t>(num_feature));
  std::vector<Entry<ThresholdType>> inst(
      std::max(dmat->num_col_, static_cast<std::uint64_t>(num_feature)), {-1});
  TL2CGEN_CHECK(rbegin < rend && rend <= dmat->num_row_);
  ElementType const* data = dmat->data_.data();
  std::uint32_t const* col_ind = dmat->col_ind_.data();
  std::uint64_t const* row_ptr = dmat->row_ptr_.data();
  for (std::uint64_t rid = rbegin; rid < rend; ++rid) {
    std::uint64_t const ibegin = row_ptr[rid];
    std::uint64_t const iend = row_ptr[rid + 1];
    for (std::uint64_t i = ibegin; i < iend; ++i) {
      inst[col_ind[i]].fvalue = static_cast<ThresholdType>(data[i]);
    }
    auto output_slice = stdex::submdspan(output_view, rid, stdex::full_extent, stdex::full_extent);
    static_assert(std::is_same_v<decltype(output_slice), Array2DView<LeafOutputType>>);
    func(inst.data(), static_cast<int>(pred_margin), output_slice.data_handle());
    for (std::uint64_t i = ibegin; i < iend; ++i) {
      inst[col_ind[i]].missing = -1;
    }
  }
}

template <typename ThresholdType, typename LeafOutputType, typename ElementType, typename PredFunc>
inline void ApplyBatch(tl2cgen::DenseDMatrix<ElementType> const* dmat, int num_feature,
    std::uint64_t rbegin, std::uint64_t rend, bool pred_margin,
    Array3DView<LeafOutputType> output_view, PredFunc func) {
  bool const nan_missing = tl2cgen::detail::math::CheckNAN(dmat->missing_value_);
  TL2CGEN_CHECK_LE(dmat->num_col_, static_cast<std::uint64_t>(num_feature));
  std::vector<Entry<ThresholdType>> inst(
      std::max(dmat->num_col_, static_cast<std::uint64_t>(num_feature)), {-1});
  TL2CGEN_CHECK(rbegin < rend && rend <= dmat->num_row_);
  std::uint64_t const num_col = dmat->num_col_;
  ElementType const missing_value = dmat->missing_value_;
  ElementType const* data = dmat->data_.data();
  ElementType const* row = nullptr;
  for (std::uint64_t rid = rbegin; rid < rend; ++rid) {
    row = &data[rid * num_col];
    for (std::uint64_t j = 0; j < num_col; ++j) {
      if (tl2cgen::detail::math::CheckNAN(row[j])) {
        TL2CGEN_CHECK(nan_missing)
            << "The missing_value argument must be set to NaN if there is any NaN in the matrix.";
      } else if (nan_missing || row[j] != missing_value) {
        inst[j].fvalue = static_cast<ThresholdType>(row[j]);
      }
    }
    auto output_slice = stdex::submdspan(output_view, rid, stdex::full_extent, stdex::full_extent);
    static_assert(std::is_same_v<decltype(output_slice), Array2DView<LeafOutputType>>);
    func(inst.data(), static_cast<int>(pred_margin), output_slice.data_handle());
    for (std::uint64_t j = 0; j < num_col; ++j) {
      inst[j].missing = -1;
    }
  }
}

}  // anonymous namespace

namespace tl2cgen::predictor {

Predictor::Predictor(char const* libpath, int num_worker_thread) {
  thread_config_ = tl2cgen::detail::threading_utils::ConfigureThreadConfig(num_worker_thread);
  lib_ = std::make_unique<detail::SharedLibrary>(libpath);

  using Int32QueryFunc = std::int32_t (*)();
  using Int32VecQueryFunc = void (*)(std::int32_t*);
  using StringQueryFunc = char const* (*)();

  /* 1. Query number of targets */
  auto* num_target_query_func = lib_->LoadFunctionWithSignature<Int32QueryFunc>("get_num_target");
  num_target_ = num_target_query_func();

  /* 2. Query number of class per target */
  auto* num_class_query_func = lib_->LoadFunctionWithSignature<Int32VecQueryFunc>("get_num_class");
  num_class_ = std::vector<std::int32_t>(num_target_);
  num_class_query_func(num_class_.data());
  max_num_class_ = *std::max_element(num_class_.begin(), num_class_.end());

  /* 3. Query number of features */
  auto* num_feature_query_func = lib_->LoadFunctionWithSignature<Int32QueryFunc>("get_num_feature");
  num_feature_ = num_feature_query_func();

  /* 4. Query the data type for thresholds and leaf outputs */
  auto* threshold_type_query_func
      = lib_->LoadFunctionWithSignature<StringQueryFunc>("get_threshold_type");
  threshold_type_ = threshold_type_query_func();
  auto* leaf_output_type_query_func
      = lib_->LoadFunctionWithSignature<StringQueryFunc>("get_leaf_output_type");
  leaf_output_type_ = leaf_output_type_query_func();

  /* 8. Load appropriate function for margin prediction */
  pred_func_ = std::make_unique<PredictFunction>(DataTypeFromString(threshold_type_),
      DataTypeFromString(leaf_output_type_), *lib_, num_feature_, num_target_, max_num_class_);
}

template <typename ThresholdType, typename LeafOutputType>
void detail::PredictFunctionPreset<ThresholdType, LeafOutputType>::PredictBatch(DMatrix const* dmat,
    std::uint64_t rbegin, std::uint64_t rend, bool pred_margin, LeafOutputType* out_pred) const {
  TL2CGEN_CHECK(rbegin < rend && rend <= dmat->GetNumRow());
  using PredFunc = void (*)(Entry<ThresholdType>*, int, LeafOutputType*);
  auto* pred_func = reinterpret_cast<PredFunc>(handle_);
  TL2CGEN_CHECK(pred_func) << "The predict() function has incorrect signature.";
  auto output_view
      = Array3DView<LeafOutputType>(out_pred, dmat->GetNumRow(), num_target_, max_num_class_);
  std::visit(
      [this, &pred_func, rbegin, rend, pred_margin, output_view](auto&& concrete_dmat) {
        return ApplyBatch<ThresholdType, LeafOutputType>(
            &concrete_dmat, num_feature_, rbegin, rend, pred_margin, output_view, pred_func);
      },
      dmat->variant_);
}

void Predictor::PredictBatch(
    DMatrix const* dmat, int verbose, bool pred_margin, void* out_result) const {
  std::uint64_t const num_row = dmat->GetNumRow();
  if (num_row == 0) {
    return;
  }
  double const tstart = GetTime();
  // Reduce nthread if n_row is small
  std::uint64_t const nthread
      = std::min(static_cast<std::uint64_t>(thread_config_.nthread), num_row);
  std::vector<std::uint64_t> const row_ptr = SplitBatch(dmat, nthread);
  std::vector<std::uint64_t> result_size(nthread);
  tl2cgen::detail::threading_utils::ParallelFor(std::uint64_t(0), nthread, thread_config_,
      tl2cgen::detail::threading_utils::ParallelSchedule::Static(),
      [&](std::uint64_t thread_id, int) {
        std::uint64_t rbegin = row_ptr[thread_id];
        std::uint64_t rend = row_ptr[thread_id + 1];
        pred_func_->PredictBatch(dmat, rbegin, rend, pred_margin, out_result);
      });
  double const tend = GetTime();
  if (verbose > 0) {
    TL2CGEN_LOG(INFO) << "TL2cgen: Finished prediction in " << (tend - tstart) << " sec";
  }
}

template void detail::PredictFunctionPreset<float, float>::PredictBatch(
    DMatrix const*, std::uint64_t, std::uint64_t, bool pred_margin, float* out_pred) const;
template void detail::PredictFunctionPreset<double, double>::PredictBatch(
    DMatrix const*, std::uint64_t, std::uint64_t, bool pred_margin, double* out_pred) const;

}  // namespace tl2cgen::predictor
