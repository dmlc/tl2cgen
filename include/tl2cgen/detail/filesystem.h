/*!
 * Copyright (c) 2023 by Contributors
 * \file filesystem.h
 * \author Hyunsu Cho
 * \brief Cross-platform wrapper for common filesystem functions
 */
#ifndef TL2CGEN_DETAIL_FILESYSTEM_H_
#define TL2CGEN_DETAIL_FILESYSTEM_H_

#include <filesystem>
#include <string>
#include <vector>

namespace tl2cgen::detail::filesystem {

/*!
 * \brief Create a directory with a given name if one doesn't exist already.
 * \param dirpath Path to directory to be created.
 */
void CreateDirectoryIfNotExist(std::filesystem::path const& dirpath);

}  // namespace tl2cgen::detail::filesystem

#endif  // TL2CGEN_DETAIL_FILESYSTEM_H_
