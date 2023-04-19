/*!
 * Copyright (c) 2023 by Contributors
 * \file filesystem.h
 * \author Hyunsu Cho
 * \brief Cross-platform wrapper for common filesystem functions
 */
#ifndef TL2CGEN_DETAIL_FILESYSTEM_H_
#define TL2CGEN_DETAIL_FILESYSTEM_H_

#include <string>
#include <vector>

namespace tl2cgen::detail::filesystem {

/*!
 * \brief Create a directory with a given name if one doesn't exist already.
 * \param dirpath Path to directory to be created.
 */
void CreateDirectoryIfNotExist(char const* dirpath);

/*!
 * \brief Write a sequence of strings to a text file, with newline character (\n) inserted between
 *        strings. This function is suitable for creating multi-line text files.
 * \param filename Name of text file
 * \param content A sequence of strings to be written.
 */
void WriteToFile(std::string const& filename, std::string const& content);

/*!
 * \brief Write a sequence of bytes to a text file
 * \param filename Name of text file
 * \param content A sequence of bytes to be written.
 */
void WriteToFile(std::string const& filename, std::vector<char> const& content);

}  // namespace tl2cgen::detail::filesystem

#endif  // TL2CGEN_DETAIL_FILESYSTEM_H_
