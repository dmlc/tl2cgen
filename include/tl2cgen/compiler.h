/*!
 * Copyright (c) 2023 by Contributors
 * \file compiler.h
 * \brief Interface of compiler that compiles a tree ensemble model
 * \author Hyunsu Cho
 */
#ifndef TL2CGEN_COMPILER_H_
#define TL2CGEN_COMPILER_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace treelite {
class Model;  // forward declaration
}  // namespace treelite

namespace tl2cgen {

namespace compiler {

struct CompilerParam;  // forward declaration

struct CompiledModel {
  struct FileEntry {
    std::string content;
    std::vector<char> content_binary;
    bool is_binary;
    FileEntry() : is_binary(false) {}
    // Passing std::vector<char> indicates binary data
    // Passing std::string indicates text data
    // Use move constructor and assignment exclusively to save memory
    explicit FileEntry(std::string const& content) = delete;
    explicit FileEntry(std::string&& content) : content(std::move(content)), is_binary(false) {}
    explicit FileEntry(std::vector<char> const&) = delete;
    explicit FileEntry(std::vector<char>&& content)
        : content_binary(std::move(content)), is_binary(true) {}
    FileEntry(FileEntry const& other) = delete;
    FileEntry(FileEntry&& other) = default;
    FileEntry& operator=(FileEntry const& other) = delete;
    FileEntry& operator=(FileEntry&& other) = default;
  };
  std::unordered_map<std::string, FileEntry> files;
  std::string file_prefix;
};

}  // namespace compiler

/*! \brief interface of compiler */
class Compiler {
 public:
  /*! \brief virtual destructor */
  virtual ~Compiler() = default;
  /*!
   * \brief convert tree ensemble model
   * \return compiled model
   */
  virtual compiler::CompiledModel Compile(treelite::Model const& model) = 0;
  /*!
   * \brief Query the parameters used to intiailize the compiler
   * \return Parameters used
   */
  virtual compiler::CompilerParam QueryParam() const = 0;
  /*!
   * \brief Create a compiler from given name
   * \param name Name of compiler
   * \param param_json_str JSON string representing compiler configuration
   * \return The created compiler
   */
  static Compiler* Create(std::string const& name, char const* param_json_str);
};

}  // namespace tl2cgen

#endif  // TL2CGEN_COMPILER_H_
