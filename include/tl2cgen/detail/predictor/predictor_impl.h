/*!
 * Copyright (c) 2023 by Contributors
 * \file predictor_impl.h
 * \author Hyunsu Cho
 * \brief Implementation of Predictor objects
 */

#ifndef TL2CGEN_DETAIL_PREDICTOR_PREDICTOR_IMPL_H_
#define TL2CGEN_DETAIL_PREDICTOR_PREDICTOR_IMPL_H_

#include <tl2cgen/detail/data_matrix_impl.h>
#include <tl2cgen/detail/math_funcs.h>
#include <tl2cgen/detail/predictor/shared_library.h>
#include <tl2cgen/logging.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <variant>
#include <vector>

namespace tl2cgen::predictor::detail {

template <typename ThresholdType, typename LeafOutputType, typename ElementType, typename PredFunc>
inline std::size_t ApplyBatch(
    DenseDMatrix<ElementType> const*, int, std::size_t, std::size_t, LeafOutputType*, PredFunc);

template <typename ThresholdType, typename LeafOutputType, typename ElementType, typename PredFunc>
inline std::size_t ApplyBatch(
    CSRDMatrix<ElementType> const*, int, std::size_t, std::size_t, LeafOutputType*, PredFunc);

/*!
 * \brief Data layout. The value -1 signifies the missing value.
 *        When the "missing" field is set to -1, the "fvalue" field is set to
 *        NaN (Not a Number), so there is no danger for mistaking between
 *        missing values and non-missing values.
 */
template <typename ElementType>
union Entry {
  int missing;
  ElementType fvalue;
};

/** Specialized classes **/
/*!
 * \brief Container for the prediction function, with handle to the native library.
 *        This class has template parameters to indicate the type of thresholds and leaf outputs
 *        in the tree model. For most uses, use the type-erased version, PredictFunction, so that
 *        you don't need to specify the template parameters.
 */
template <typename ThresholdType, typename LeafOutputType>
class PredictFunctionPreset {
 public:
  using threshold_type = ThresholdType;
  using leaf_output_type = LeafOutputType;

  PredictFunctionPreset()
      : handle_(nullptr), is_multiclass_(false), num_feature_(0), num_class_(1) {}
  PredictFunctionPreset(SharedLibrary const& shared_lib, int num_feature, int num_class)
      : num_feature_(num_feature), num_class_(num_class) {
    TL2CGEN_CHECK_GT(num_class, 0) << "num_class cannot be zero";
    bool const is_multiclass = (num_class > 1);
    if (is_multiclass) {  // multi-class classification
      handle_ = shared_lib.LoadFunction("predict_multiclass");
    } else {  // everything else
      handle_ = shared_lib.LoadFunction("predict");
    }
    is_multiclass_ = is_multiclass;
  }

  std::size_t PredictBatch(DMatrix const* dmat, std::size_t rbegin, std::size_t rend,
      bool pred_margin, LeafOutputType* out_pred) const {
    TL2CGEN_CHECK(rbegin < rend && rend <= dmat->GetNumRow());
    // Pass the correct prediction function to PredLoop.
    // We also need to specify how the function should be called.
    std::size_t result_size;
    // Dimension of output vector: can be either [num_row] or [num_row] * [num_class].
    // Note that size of prediction may be smaller than out_pred (this occurs
    // when pred_function is set to "max_index").
    using PredFunc = LeafOutputType (*)(Entry<ThresholdType>*, int);
    using MulticlassPredFunc = std::size_t (*)(Entry<ThresholdType>*, int, LeafOutputType*);
    std::function<std::size_t(std::size_t, Entry<ThresholdType>*, LeafOutputType*)> pred_func;
    if (is_multiclass_) {  // multi-class classification
      auto* fptr = reinterpret_cast<MulticlassPredFunc>(handle_);
      TL2CGEN_CHECK(fptr) << "The predict_multiclass() function has incorrect signature.";
      pred_func = [fptr, pred_margin, num_class = num_class_](std::size_t rid,
                      Entry<ThresholdType>* inst, LeafOutputType* out_pred) -> std::size_t {
        return fptr(inst, static_cast<int>(pred_margin), &out_pred[rid * num_class]);
      };
    } else {
      auto* fptr = reinterpret_cast<PredFunc>(handle_);
      TL2CGEN_CHECK(fptr) << "The predict() function has incorrect signature.";
      pred_func = [fptr, pred_margin](std::size_t rid, Entry<ThresholdType>* inst,
                      LeafOutputType* out_pred) -> std::size_t {
        out_pred[rid] = fptr(inst, static_cast<int>(pred_margin));
        return 1;
      };
    }
    return std::visit(
        [this, &pred_func, rbegin, rend, out_pred](auto&& concrete_dmat) {
          return detail::ApplyBatch<ThresholdType, LeafOutputType>(
              &concrete_dmat, num_feature_, rbegin, rend, out_pred, pred_func);
        },
        dmat->variant_);
  }

 private:
  /*! \brief Pointer to the underlying native function */
  SharedLibrary::FunctionHandle handle_;
  bool is_multiclass_;
  int num_feature_;
  int num_class_;
};

/** Variant definitions **/
using OutputPtrVariant = std::variant<float*, double*, std::uint32_t*>;
using PredictFunctionVariant
    = std::variant<PredictFunctionPreset<float, float>, PredictFunctionPreset<float, std::uint32_t>,
        PredictFunctionPreset<double, double>, PredictFunctionPreset<double, std::uint32_t>>;

/* Variant setters */
template <int variant_index>
OutputPtrVariant SetOutputPtrVariant(int target_variant_index, std::size_t size) {
  OutputPtrVariant result;
  if constexpr (variant_index != std::variant_size_v<OutputPtrVariant>) {
    if (variant_index == target_variant_index) {
      using OutputPtr = std::variant_alternative_t<variant_index, OutputPtrVariant>;
      using OutputType = std::remove_pointer_t<OutputPtr>;
      result = static_cast<OutputType*>(new OutputType[size]);
    } else {
      result = SetOutputPtrVariant<variant_index + 1>(target_variant_index, size);
    }
  }
  return result;
}

template <int variant_index>
PredictFunctionVariant SetPredictFunctionVariant(int target_variant_index,
    detail::SharedLibrary const& shared_lib, int num_feature, int num_class) {
  PredictFunctionVariant result;
  if constexpr (variant_index != std::variant_size_v<PredictFunctionVariant>) {
    if (variant_index == target_variant_index) {
      using PredFuncType = std::variant_alternative_t<variant_index, PredictFunctionVariant>;
      result = PredFuncType(shared_lib, num_feature, num_class);
    } else {
      result = SetPredictFunctionVariant<variant_index + 1>(
          target_variant_index, shared_lib, num_feature, num_class);
    }
  }
  return result;
}

/** Implementation of prediction routine **/
template <typename ThresholdType, typename LeafOutputType, typename ElementType, typename PredFunc>
inline std::size_t ApplyBatch(CSRDMatrix<ElementType> const* dmat, int num_feature,
    std::size_t rbegin, std::size_t rend, LeafOutputType* out_pred, PredFunc func) {
  TL2CGEN_CHECK_LE(dmat->num_col_, static_cast<std::size_t>(num_feature));
  std::vector<Entry<ThresholdType>> inst(
      std::max(dmat->num_col_, static_cast<std::size_t>(num_feature)), {-1});
  TL2CGEN_CHECK(rbegin < rend && rend <= dmat->num_row_);
  ElementType const* data = dmat->data_.data();
  std::uint32_t const* col_ind = dmat->col_ind_.data();
  std::size_t const* row_ptr = dmat->row_ptr_.data();
  std::size_t total_output_size = 0;
  for (std::size_t rid = rbegin; rid < rend; ++rid) {
    const std::size_t ibegin = row_ptr[rid];
    const std::size_t iend = row_ptr[rid + 1];
    for (std::size_t i = ibegin; i < iend; ++i) {
      inst[col_ind[i]].fvalue = static_cast<ThresholdType>(data[i]);
    }
    total_output_size += func(rid, &inst[0], out_pred);
    for (std::size_t i = ibegin; i < iend; ++i) {
      inst[col_ind[i]].missing = -1;
    }
  }
  return total_output_size;
}

template <typename ThresholdType, typename LeafOutputType, typename ElementType, typename PredFunc>
inline std::size_t ApplyBatch(DenseDMatrix<ElementType> const* dmat, int num_feature,
    std::size_t rbegin, std::size_t rend, LeafOutputType* out_pred, PredFunc func) {
  bool const nan_missing = tl2cgen::detail::math::CheckNAN(dmat->missing_value_);
  TL2CGEN_CHECK_LE(dmat->num_col_, static_cast<std::size_t>(num_feature));
  std::vector<Entry<ThresholdType>> inst(
      std::max(dmat->num_col_, static_cast<std::size_t>(num_feature)), {-1});
  TL2CGEN_CHECK(rbegin < rend && rend <= dmat->num_row_);
  const std::size_t num_col = dmat->num_col_;
  const ElementType missing_value = dmat->missing_value_;
  ElementType const* data = dmat->data_.data();
  ElementType const* row = nullptr;
  std::size_t total_output_size = 0;
  for (std::size_t rid = rbegin; rid < rend; ++rid) {
    row = &data[rid * num_col];
    for (std::size_t j = 0; j < num_col; ++j) {
      if (tl2cgen::detail::math::CheckNAN(row[j])) {
        TL2CGEN_CHECK(nan_missing)
            << "The missing_value argument must be set to NaN if there is any NaN in the matrix.";
      } else if (nan_missing || row[j] != missing_value) {
        inst[j].fvalue = static_cast<ThresholdType>(row[j]);
      }
    }
    total_output_size += func(rid, &inst[0], out_pred);
    for (std::size_t j = 0; j < num_col; ++j) {
      inst[j].missing = -1;
    }
  }
  return total_output_size;
}

}  // namespace tl2cgen::predictor::detail

#endif  // TL2CGEN_DETAIL_PREDICTOR_PREDICTOR_IMPL_H_
