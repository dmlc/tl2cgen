/*!
 * Copyright (c) 2023-2024 by Contributors
 * \file filesystem.cc
 * \author Hyunsu Cho
 * \brief Cross-platform wrapper for common filesystem functions
 */

#include <tl2cgen/detail/filesystem.h>
#include <tl2cgen/logging.h>

#include <filesystem>
#include <fstream>

namespace tl2cgen::detail::filesystem {

void CreateDirectoryIfNotExist(std::filesystem::path const& dirpath) {
  if (std::filesystem::exists(dirpath)) {
    TL2CGEN_CHECK(std::filesystem::is_directory(dirpath))
        << "CreateDirectoryIfNotExist: " << dirpath << " is a file, not a directory";
  } else {
    // Directory doesn't seem to exist; attempt to create one
    TL2CGEN_CHECK(std::filesystem::create_directories(dirpath))
        << "CreateDirectoryIfNotExist: failed to create new directory " << dirpath;
  }
}

void WriteToFile(std::filesystem::path const& path, std::string const& content) {
  std::ofstream of(path);
  of << content;
}

void WriteToFile(std::filesystem::path const& path, std::vector<char> const& content) {
  std::ofstream of(path, std::ios::out | std::ios::binary);
  of.write(content.data(), content.size());
}

}  // namespace tl2cgen::detail::filesystem
