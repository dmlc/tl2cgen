/*!
 * Copyright (c) 2023 by Contributors
 * \file predictor.h
 * \author Hyunsu Cho
 * \brief Predictor class, to predict with a compiled C module
 */
#ifndef TL2CGEN_PREDICTOR_H_
#define TL2CGEN_PREDICTOR_H_

#include <tl2cgen/data_matrix.h>
#include <tl2cgen/detail/predictor/shared_library.h>
#include <tl2cgen/detail/threading_utils/omp_config.h>
#include <tl2cgen/detail/threading_utils/omp_exception.h>
#include <tl2cgen/logging.h>
#include <tl2cgen/predictor_types.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace tl2cgen::predictor {

namespace detail {

class SharedLibrary;

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

  PredictFunctionPreset() : handle_(nullptr), num_feature_(0), num_target_(1), max_num_class_(1) {}
  PredictFunctionPreset(SharedLibrary const& shared_lib, int num_feature, std::int32_t num_target,
      std::int32_t max_num_class)
      : num_feature_(num_feature), num_target_(num_target), max_num_class_(max_num_class) {
    handle_ = shared_lib.LoadFunction("predict");
  }

  void PredictBatch(DMatrix const* dmat, std::uint64_t rbegin, std::uint64_t rend, bool pred_margin,
      LeafOutputType* out_pred) const;

 private:
  /*! \brief Pointer to the underlying native function */
  SharedLibrary::FunctionHandle handle_;
  std::int32_t num_feature_;
  std::int32_t num_target_;
  std::int32_t max_num_class_;
};

/** Variant definitions **/
using PredictFunctionVariant
    = std::variant<PredictFunctionPreset<float, float>, PredictFunctionPreset<double, double>>;

/* Variant setters */
template <int variant_index>
PredictFunctionVariant SetPredictFunctionVariant(int target_variant_index,
    detail::SharedLibrary const& shared_lib, std::int32_t num_feature, std::int32_t num_target,
    std::int32_t max_num_class) {
  PredictFunctionVariant result;
  if constexpr (variant_index != std::variant_size_v<PredictFunctionVariant>) {
    if (variant_index == target_variant_index) {
      using PredFuncType = std::variant_alternative_t<variant_index, PredictFunctionVariant>;
      result = PredFuncType(shared_lib, num_feature, num_target, max_num_class);
    } else {
      result = SetPredictFunctionVariant<variant_index + 1>(
          target_variant_index, shared_lib, num_feature, num_target, max_num_class);
    }
  }
  return result;
}

}  // namespace detail

/*!
 * \brief Prediction function object. It will call the prediction function from the
 *        shared library with appropriate input and output types specified.
 * Expected input shape: (num_row, num_feature)
 * Expected output shape: (num_row, num_target, max(num_class)), where num_class is the array
 *   with num_class[i] representing the number of classes for the i-th output target.
 */
class PredictFunction {
 public:
  PredictFunction(DataTypeEnum threshold_type, DataTypeEnum leaf_output_type,
      detail::SharedLibrary const& shared_lib, std::int32_t num_feature, std::int32_t num_target,
      std::int32_t max_num_class) {
    TL2CGEN_CHECK(threshold_type == leaf_output_type)
        << "The leaf output must have same type as the threshold";
    int const target_variant_index = (threshold_type == DataTypeEnum::kFloat64);
    variant_ = detail::SetPredictFunctionVariant<0>(
        target_variant_index, shared_lib, num_feature, num_target, max_num_class);
  }

  /*!
   * \brief Make prediction for a slice [rbegin:rend] in the data matrix
   * \param dmat Data matrix
   * \param rbegin Beginning of the slice
   * \param rend End of the slice
   * \param pred_margin Whether to produce raw margin scores instead of
   *                    transformed probabilities
   * \param out_pred Output buffer to store prediction result
   */
  void PredictBatch(DMatrix const* dmat, std::uint64_t rbegin, std::uint64_t rend, bool pred_margin,
      void* out_pred) const {
    std::visit(
        [&](auto&& pred_func_concrete) {
          using LeafOutputType =
              typename std::remove_reference_t<decltype(pred_func_concrete)>::leaf_output_type;
          pred_func_concrete.PredictBatch(
              dmat, rbegin, rend, pred_margin, static_cast<LeafOutputType*>(out_pred));
        },
        variant_);
  }

  detail::PredictFunctionVariant variant_;
};

/*!
 * \brief Predictor class: Use CPU worker threads to run prediction in parallel.
 *        This function uses OpenMP.
 */
class Predictor {
 public:
  /*!
   * \brief Load the prediction function from dynamic shared library.
   * \param libpath path of dynamic shared library (.so/.dll/.dylib).
   */
  explicit Predictor(char const* libpath, int num_worker_thread = -1);
  ~Predictor() = default;

  /*!
   * \brief Make predictions on a batch of data rows (synchronously). This
   *        function internally divides the workload among all worker threads.
   * \param dmat A batch of rows
   * \param verbose Whether to produce extra messages
   * \param pred_margin Whether to produce raw margin scores instead of
   *                    transformed probabilities
   * \param out_result Output buffer to store prediction result. The buffer should be allocated with
   *                   prod(output_shape) * sizeof(leaf_output_type), where output_shape and
   *                   leaf_output_type are given by \ref GetOutputShape and \ref GetLeafOutputType,
   *                   respectively.
   */
  void PredictBatch(DMatrix const* dmat, int verbose, bool pred_margin, void* out_result) const;
  /*!
   * \brief Given a batch of data rows, query the necessary shape of array to
   *        hold predictions for all data points.
   * \param dmat Batch of rows
   * \return Shape of prediction array
   */
  std::vector<std::uint64_t> GetOutputShape(DMatrix const* dmat) const {
    TL2CGEN_CHECK(pred_func_) << "A shared library needs to be loaded first using Load()";
    return {dmat->GetNumRow(), static_cast<std::uint64_t>(num_target_),
        static_cast<std::uint64_t>(max_num_class_)};
  }
  /*!
   * \brief Given a batch of data rows, query the necessary shape of array to
   *        hold predictions for all data points.
   * \param dmat Batch of rows
   * \param rbegin Beginning of range of rows
   * \param rend End of range of rows
   * \return Shape of prediction array
   */
  std::vector<std::uint64_t> GetOutputShape(
      DMatrix const* dmat, std::uint64_t rbegin, std::uint64_t rend) const {
    TL2CGEN_CHECK(pred_func_) << "A shared library needs to be loaded first using Load()";
    TL2CGEN_CHECK(rbegin < rend && rend <= dmat->GetNumRow());
    return {rend - rbegin, static_cast<std::uint64_t>(num_target_),
        static_cast<std::uint64_t>(max_num_class_)};
  }

  /*!
   * \brief Get the type of the split thresholds
   * \return Type of the split thresholds
   */
  std::string GetThresholdType() const {
    return threshold_type_;
  }

  /*!
   * \brief Get the type of the leaf outputs
   * \return Type of the leaf outputs
   */
  std::string GetLeafOutputType() const {
    return leaf_output_type_;
  }

  /*!
   * \brief Get the number of features used in the training data
   * \return Number of features
   */
  std::int32_t GetNumFeature() const {
    return num_feature_;
  }

  /*!
   * \brief Get the number of output targets
   * \return Number of output targets
   */
  std::int32_t GetNumTarget() const {
    return num_target_;
  }

  /*!
   * \brief Get the number of classes per output target
   * \return Number of classes per output target
   */
  std::vector<std::int32_t> GetNumClass() const {
    return num_class_;
  }

 private:
  std::unique_ptr<detail::SharedLibrary> lib_;
  std::unique_ptr<PredictFunction> pred_func_;
  std::int32_t num_feature_;
  std::int32_t num_target_;
  std::vector<std::int32_t> num_class_;
  std::int32_t max_num_class_;
  std::string threshold_type_;
  std::string leaf_output_type_;
  tl2cgen::detail::threading_utils::ThreadConfig thread_config_;
};

}  // namespace tl2cgen::predictor

#endif  // TL2CGEN_PREDICTOR_H_
