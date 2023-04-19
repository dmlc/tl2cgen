/*!
 * Copyright (c) 2023 by Contributors
 * \file data_matrix.h
 * \author Hyunsu Cho
 * \brief Data matrix container used in TL2cgen
 */
#ifndef TL2CGEN_DATA_MATRIX_H_
#define TL2CGEN_DATA_MATRIX_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace tl2cgen {

template <typename ElementType>
class DenseDMatrix {
 public:
  DenseDMatrix(std::vector<ElementType> data, ElementType missing_value, std::size_t num_row,
      std::size_t num_col)
      : data_{std::move(data)},
        missing_value_{missing_value},
        num_row_{num_row},
        num_col_{num_col} {}
  std::size_t GetNumRow() const {
    return num_row_;
  }
  std::size_t GetNumCol() const {
    return num_col_;
  }
  std::size_t GetNumElem() const {
    return num_row_ * num_col_;
  }

  /*! \brief Feature values */
  std::vector<ElementType> data_;
  /*! \brief Value representing the missing value (usually NaN) */
  ElementType missing_value_;
  /*! \brief Number of rows */
  std::size_t num_row_;
  /*! \brief Number of columns (i.e. # of features used) */
  std::size_t num_col_;
};

template <typename ElementType>
class CSRDMatrix {
 public:
  CSRDMatrix(std::vector<ElementType> data, std::vector<std::uint32_t> col_ind,
      std::vector<std::size_t> row_ptr, std::size_t num_row, std::size_t num_col)
      : data_{std::move(data)},
        col_ind_{std::move(col_ind)},
        row_ptr_{std::move(row_ptr)},
        num_row_{num_row},
        num_col_{num_col} {}
  std::size_t GetNumRow() const {
    return num_row_;
  }
  std::size_t GetNumCol() const {
    return num_col_;
  }
  std::size_t GetNumElem() const {
    return row_ptr_.at(num_row_);
  }

  /*! \brief Feature values */
  std::vector<ElementType> data_;
  /*! \brief Feature indices. col_ind_[i] indicates the feature index associated with data[i]. */
  std::vector<std::uint32_t> col_ind_;
  /*! \brief Pointer to row headers; length is [num_row] + 1. */
  std::vector<std::size_t> row_ptr_;
  /*! \brief Number of rows */
  std::size_t num_row_;
  /*! \brief Number of columns (i.e. # of features used) */
  std::size_t num_col_;
};

using DMatrixVariant = std::variant<DenseDMatrix<float>, DenseDMatrix<double>, CSRDMatrix<float>,
    CSRDMatrix<double>>;

class DMatrix {
 public:
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
};

}  // namespace tl2cgen

#endif  // TL2CGEN_DATA_MATRIX_H_
