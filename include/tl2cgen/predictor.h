/*!
 * Copyright (c) 2023 by Contributors
 * \file predictor.h
 * \author Hyunsu Cho
 * \brief Predictor class, to predict with a compiled C module
 */
#ifndef TL2CGEN_PREDICTOR_H_
#define TL2CGEN_PREDICTOR_H_

#include <tl2cgen/data_matrix.h>
#include <tl2cgen/detail/predictor/predictor_impl.h>
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

namespace tl2cgen::predictor {

namespace detail {

class SharedLibrary;

}  // namespace detail

class OutputBuffer {
 public:
  OutputBuffer() : variant_(static_cast<float*>(nullptr)) {}
  OutputBuffer(DataTypeEnum output_type, std::size_t size) {
    int target_variant_index = static_cast<int>(output_type);
    variant_ = detail::SetOutputPtrVariant<0>(target_variant_index, size);
  }
  ~OutputBuffer() {
    std::visit([](auto&& ptr) { delete[] ptr; }, variant_);
  }
  void* data() {
    return std::visit([](auto&& ptr) { return static_cast<void*>(ptr); }, variant_);
  }

  detail::OutputPtrVariant variant_;
};

/*!
 * \brief Prediction function object. It will call the prediction function from the
 *        shared library with appropriate input and output types specified.
 */
class PredictFunction {
 public:
  PredictFunction(DataTypeEnum threshold_type, DataTypeEnum leaf_output_type,
      detail::SharedLibrary const& shared_lib, int num_feature, int num_class) {
    TL2CGEN_CHECK(threshold_type == leaf_output_type || leaf_output_type == DataTypeEnum::kUInt32)
        << "If the leaf output is a floating-point type, it must have same width as the threshold";
    int const target_variant_index = (threshold_type == DataTypeEnum::kFloat64) * 2
                                     + (leaf_output_type == DataTypeEnum::kUInt32);
    variant_ = detail::SetPredictFunctionVariant<0>(
        target_variant_index, shared_lib, num_feature, num_class);
  }

  std::size_t PredictBatch(DMatrix const* dmat, std::size_t rbegin, std::size_t rend,
      bool pred_margin, OutputBuffer* out_pred) const {
    return std::visit(
        [&](auto&& pred_func_concrete, auto&& out_pred_ptr) {
          using LeafOutputType =
              typename std::remove_reference_t<decltype(pred_func_concrete)>::leaf_output_type;
          using ExpectedLeafOutputType
              = std::remove_pointer_t<std::remove_reference_t<decltype(out_pred_ptr)>>;
          if constexpr (std::is_same_v<LeafOutputType, ExpectedLeafOutputType>) {
            return pred_func_concrete.PredictBatch(dmat, rbegin, rend, pred_margin, out_pred_ptr);
          } else {
            TL2CGEN_LOG(FATAL)
                << "Type mismatch between LeafOutputType of the model and the output buffer. "
                << "LeafOutputType = " << typeid(LeafOutputType).name()
                << ", ExpectedLeafOutputType = " << typeid(ExpectedLeafOutputType).name();
            return std::size_t(0);
          }
        },
        variant_, out_pred->variant_);
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
   * \param out_result Output buffer to store prediction result. Allocate this buffer by calling
   *                   OutputBuffer(output_type=QueryLeafOutputType(), size=QueryResultSize()).
   * \return Length of the output vector, which is guaranteed to be less than
   *         or equal to QueryResultSize()
   */
  std::size_t PredictBatch(
      DMatrix const* dmat, int verbose, bool pred_margin, OutputBuffer* out_result) const;
  /*!
   * \brief Given a batch of data rows, query the necessary size of array to
   *        hold predictions for all data points.
   * \param dmat a batch of rows
   * \return length of prediction array
   */
  inline std::size_t QueryResultSize(DMatrix const* dmat) const {
    TL2CGEN_CHECK(pred_func_) << "A shared library needs to be loaded first using Load()";
    return dmat->GetNumRow() * num_class_;
  }
  /*!
   * \brief Given a batch of data rows, query the necessary size of array to
   *        hold predictions for all data points.
   * \param dmat a batch of rows
   * \param rbegin beginning of range of rows
   * \param rend end of range of rows
   * \return length of prediction array
   */
  inline std::size_t QueryResultSize(
      DMatrix const* dmat, std::size_t rbegin, std::size_t rend) const {
    TL2CGEN_CHECK(pred_func_) << "A shared library needs to be loaded first using Load()";
    TL2CGEN_CHECK(rbegin < rend && rend <= dmat->GetNumRow());
    return (rend - rbegin) * num_class_;
  }
  /*!
   * \brief Get the number of classes in the loaded model
   *        The number is 1 for most tasks; it is greater than 1 for multiclass classification.
   * \return Length of prediction array
   */
  inline std::size_t QueryNumClass() const {
    return num_class_;
  }
  /*!
   * \brief Get the width (number of features) of each instance used to train
   *        the loaded model
   * \return Number of features
   */
  inline std::size_t QueryNumFeature() const {
    return num_feature_;
  }
  /*!
   * \brief Get name of post prediction transformation used to train the loaded model
   * \return Name of prediction transformation
   */
  inline std::string QueryPredTransform() const {
    return pred_transform_;
  }
  /*!
   * \brief Get alpha value in sigmoid transformation used to train the loaded model
   * \return Alpha value in sigmoid transformation
   */
  inline float QuerySigmoidAlpha() const {
    return sigmoid_alpha_;
  }
  /*!
   * \brief Get C value in exponential standard ratio used to train the loaded model
   * \return C value in exponential standard ratio transformation
   */
  inline float QueryRatioC() const {
    return ratio_c_;
  }
  /*!
   * \brief Get global bias which adjusting predicted margin scores
   * \return Global bias
   */
  inline float QueryGlobalBias() const {
    return global_bias_;
  }
  /*!
   * \brief Get the type of the split thresholds
   * \return Type of the split thresholds
   */
  inline std::string QueryThresholdType() const {
    return threshold_type_;
  }
  /*!
   * \brief Get the type of the leaf outputs
   * \return Type of the leaf outputs
   */
  inline std::string QueryLeafOutputType() const {
    return leaf_output_type_;
  }

 private:
  std::unique_ptr<detail::SharedLibrary> lib_;
  std::unique_ptr<PredictFunction> pred_func_;
  std::size_t num_class_;
  std::size_t num_feature_;
  std::string pred_transform_;
  float sigmoid_alpha_;
  float ratio_c_;
  float global_bias_;
  tl2cgen::detail::threading_utils::ThreadConfig thread_config_;
  std::string threshold_type_;
  std::string leaf_output_type_;

  mutable tl2cgen::detail::threading_utils::OMPException exception_catcher_;
};

}  // namespace tl2cgen::predictor

#endif  // TL2CGEN_PREDICTOR_H_
