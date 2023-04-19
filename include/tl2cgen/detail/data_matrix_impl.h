/*!
 * Copyright (c) 2023 by Contributors
 * \file data_matrix_impl.h
 * \author Hyunsu Cho
 * \brief Data matrix implementation
 */

#ifndef TL2CGEN_DETAIL_DATA_MATRIX_IMPL_H_
#define TL2CGEN_DETAIL_DATA_MATRIX_IMPL_H_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace tl2cgen {

/*! \brief Data matrix with 2D dense row-major layout */
template <typename ElementType>
class DenseDMatrix {
 public:
  DenseDMatrix()
      : data_{},
        missing_value_{std::numeric_limits<ElementType>::quiet_NaN()},
        num_row_{0},
        num_col_{0} {}
  DenseDMatrix(std::vector<ElementType> data, ElementType missing_value, std::size_t num_row,
      std::size_t num_col)
      : data_{std::move(data)},
        missing_value_{missing_value},
        num_row_{num_row},
        num_col_{num_col} {}
  DenseDMatrix(
      void const* data, void const* missing_value, std::size_t num_row, std::size_t num_col)
      : missing_value_{*static_cast<ElementType const*>(missing_value)},
        num_row_{num_row},
        num_col_{num_col} {
    auto const* data_ptr = static_cast<ElementType const*>(data);
    const std::size_t num_elem = num_row * num_col;
    data_ = std::vector<ElementType>{data_ptr, data_ptr + num_elem};
  }
  DenseDMatrix(void const*, std::uint32_t const*, std::size_t const*, std::size_t, std::size_t) {
    TL2CGEN_LOG(FATAL) << "Invalid set of arguments";
  }
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

/*! \brief Data matrix with CSR sparse layout */
template <typename ElementType>
class CSRDMatrix {
 public:
  CSRDMatrix() : data_{}, col_ind_{}, row_ptr_{0}, num_row_{0}, num_col_{0} {}
  CSRDMatrix(std::vector<ElementType> data, std::vector<std::uint32_t> col_ind,
      std::vector<std::size_t> row_ptr, std::size_t num_row, std::size_t num_col)
      : data_{std::move(data)},
        col_ind_{std::move(col_ind)},
        row_ptr_{std::move(row_ptr)},
        num_row_{num_row},
        num_col_{num_col} {}
  CSRDMatrix(void const* data, std::uint32_t const* col_ind, std::size_t const* row_ptr,
      std::size_t num_row, std::size_t num_col)
      : num_row_{num_row}, num_col_{num_col} {
    auto const* data_ptr = static_cast<ElementType const*>(data);
    const std::size_t num_elem = row_ptr[num_row];
    data_ = std::vector<ElementType>{data_ptr, data_ptr + num_elem};
    col_ind_ = std::vector<std::uint32_t>{col_ind, col_ind + num_elem};
    row_ptr_ = std::vector<std::size_t>{row_ptr, row_ptr + num_row + 1};
  }
  CSRDMatrix(void const*, void const*, std::size_t, std::size_t) {
    TL2CGEN_LOG(FATAL) << "Invalid set of arguments";
  }
  std::size_t GetNumRow() const {
    return num_row_;
  }
  std::size_t GetNumCol() const {
    return num_col_;
  }
  std::size_t GetNumElem() const {
    return row_ptr_[num_row_];
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

}  // namespace tl2cgen

#endif  // TL2CGEN_DETAIL_DATA_MATRIX_IMPL_H_
