/*!
 * Copyright (c) 2023 by Contributors
 * \file categorical_bitmap.h
 * \author Hyunsu Cho
 * \brief Function to generate bitmaps for categorical splits
 */
#ifndef TL2CGEN_DETAIL_COMPILER_UTIL_CATEGORICAL_BITMAP_H_
#define TL2CGEN_DETAIL_COMPILER_UTIL_CATEGORICAL_BITMAP_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace tl2cgen::compiler::detail::util {

inline std::vector<std::uint64_t> GetCategoricalBitmap(
    std::vector<std::uint32_t> const& matching_categories) {
  std::size_t const num_matching_categories = matching_categories.size();
  if (num_matching_categories == 0) {
    return std::vector<std::uint64_t>{0};
  }
  std::uint32_t const max_matching_category = matching_categories[num_matching_categories - 1];
  std::vector<std::uint64_t> bitmap((max_matching_category + 1 + 63) / 64, 0);
  for (std::uint32_t cat : matching_categories) {
    std::size_t const idx = cat / 64;
    std::uint32_t const offset = cat % 64;
    bitmap[idx] |= (static_cast<std::uint64_t>(1) << offset);
  }
  return bitmap;
}

}  // namespace tl2cgen::compiler::detail::util

#endif  // TL2CGEN_DETAIL_COMPILER_UTIL_CATEGORICAL_BITMAP_H_
