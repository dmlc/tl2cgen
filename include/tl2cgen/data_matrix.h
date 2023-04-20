/*!
 * Copyright (c) 2023 by Contributors
 * \file data_matrix.h
 * \author Hyunsu Cho
 * \brief Data matrix container used in TL2cgen
 */
#ifndef TL2CGEN_DATA_MATRIX_H_
#define TL2CGEN_DATA_MATRIX_H_

#include <tl2cgen/data_matrix_types.h>
#include <tl2cgen/detail/data_matrix_impl.h>
#include <tl2cgen/logging.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace tl2cgen {

using DMatrixVariant = std::variant<DenseDMatrix<float>, DenseDMatrix<double>, CSRDMatrix<float>,
    CSRDMatrix<double>>;

template <int variant_index, typename... Args>
DMatrixVariant CreateDMatrixWithSpecificVariant(int target_variant_index, Args&&... args) {
  DMatrixVariant result;
  if constexpr (variant_index != std::variant_size_v<DMatrixVariant>) {
    if (variant_index == target_variant_index) {
      using DMatrixType = std::variant_alternative_t<variant_index, DMatrixVariant>;
      result = DMatrixType{std::forward<Args>(args)...};
    } else {
      result = CreateDMatrixWithSpecificVariant<variant_index + 1>(
          target_variant_index, std::forward<Args>(args)...);
    }
  }
  return result;
}

/*! \brief Generic data matrix */
class DMatrix {
 public:
  DMatrix() = default;
  explicit DMatrix(DMatrixVariant&& variant) : variant_{std::move(variant)} {}
  std::size_t GetNumRow() const {
    return std::visit(
        [](auto&& concrete_dmatrix) { return concrete_dmatrix.GetNumRow(); }, variant_);
  }
  std::size_t GetNumCol() const {
    return std::visit(
        [](auto&& concrete_dmatrix) { return concrete_dmatrix.GetNumCol(); }, variant_);
  }
  std::size_t GetNumElem() const {
    return std::visit(
        [](auto&& concrete_dmatrix) { return concrete_dmatrix.GetNumElem(); }, variant_);
  }

  DMatrixVariant variant_;

  template <typename... Args>
  static std::unique_ptr<DMatrix> Create(
      DMatrixTypeEnum dmat_type, DMatrixElementTypeEnum elem_type, Args&&... args) {
    int const target_variant_index
        = (static_cast<int>(dmat_type) * DMatrixTypeEnumCount) + static_cast<int>(elem_type);
    std::unique_ptr<DMatrix> result = std::make_unique<DMatrix>(
        CreateDMatrixWithSpecificVariant<0>(target_variant_index, std::forward<Args>(args)...));
    return result;
  }

  template <typename DMatrixT>
  static std::unique_ptr<DMatrix> Create(DMatrixT&& dmat) {
    auto ptr = std::make_unique<DMatrix>();
    ptr->variant_ = std::forward<DMatrixT>(dmat);
    return ptr;
  }
};

}  // namespace tl2cgen

#endif  // TL2CGEN_DATA_MATRIX_H_
