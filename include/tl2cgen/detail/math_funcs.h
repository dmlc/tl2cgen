/*!
 * Copyright (c) 2023 by Contributors
 * \file math_funcs.h
 * \author Hyunsu Cho
 * \brief Misc. math functions
 */
#ifndef TL2CGEN_DETAIL_MATH_FUNCS_H_
#define TL2CGEN_DETAIL_MATH_FUNCS_H_

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace tl2cgen::detail::math {

/*!
 * \brief Perform binary search on the range [begin, end).
 * \param begin Beginning of the search range
 * \param end End of the search range
 * \param val Value being searched
 * \return Iterator pointing to the value if found; end if value not found.
 * \tparam Iter Type of iterator
 * \tparam T Type of elements
 */
template <class Iter, class T>
Iter BinarySearch(Iter begin, Iter end, T const& val) {
  Iter i = std::lower_bound(begin, end, val);
  if (i != end && val == *i) {
    return i;  // found
  } else {
    return end;  // not found
  }
}

/*!
 * \brief Check for NaN (Not a Number)
 * \param value Value to check
 * \return Whether the given value is NaN or not
 * \tparam type Type of value (should be a floating-point value)
 */
template <typename T>
inline bool CheckNAN(T value) {
#ifdef _MSC_VER
  return (_isnan(value) != 0);
#else
  return std::isnan(value);
#endif
}

}  // namespace tl2cgen::detail::math

#endif  // TL2CGEN_DETAIL_MATH_FUNCS_H_
