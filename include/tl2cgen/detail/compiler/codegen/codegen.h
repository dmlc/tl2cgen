/*!
 * Copyright (c) 2024 by Contributors
 * \file codegen.h
 * \brief Tools to define prediction transform function
 * \author Hyunsu Cho
 */
#ifndef TL2CGEN_DETAIL_COMPILER_CODEGEN_CODEGEN_H_
#define TL2CGEN_DETAIL_COMPILER_CODEGEN_CODEGEN_H_

#include <filesystem>
#include <map>
#include <ostream>
#include <string>
#include <vector>

/* Forward declarations */
namespace treelite {

class Model;

}  // namespace treelite

namespace tl2cgen::compiler {

struct CompilerParam;

}  // namespace tl2cgen::compiler

namespace tl2cgen::compiler::detail::ast {

class ASTNode;
class MainNode;
class FunctionNode;
class ConditionNode;
class OutputNode;
class TranslationUnitNode;
class QuantizerNode;
class ModelMeta;

}  // namespace tl2cgen::compiler::detail::ast

namespace tl2cgen::compiler::detail::codegen {

class CodeCollection;  // forward declaration

void GenerateCodeFromAST(ast::ASTNode const* node, CodeCollection& gencode);
void WriteCodeToDisk(std::filesystem::path const& dirpath, CodeCollection const& collection);
void WriteBuildRecipeToDisk(std::filesystem::path const& dirpath,
    std::string const& native_lib_name, CodeCollection const& collection);

// Codegen implementation for each AST node type
void HandleMainNode(ast::MainNode const* node, CodeCollection& gencode);
void HandleFunctionNode(ast::FunctionNode const* node, CodeCollection& gencode);
void HandleConditionNode(ast::ConditionNode const* node, CodeCollection& gencode);
void HandleOutputNode(ast::OutputNode const* node, CodeCollection& gencode);
void HandleTranslationUnitNode(ast::TranslationUnitNode const* node, CodeCollection& gencode);
void HandleQuantizerNode(ast::QuantizerNode const* node, CodeCollection& gencode);

std::string GetThresholdTypeStr(ast::ASTNode const* node);
std::string GetThresholdCType(ast::ASTNode const* node);
std::string GetLeafOutputTypeStr(ast::ASTNode const* node);
std::string GetLeafOutputCType(ast::ASTNode const* node);
std::string GetLeafOutputCType(ast::ModelMeta const& model_meta);

std::string GetPostprocessorFunc(
    ast::ModelMeta const& model_meta, std::string const& postprocessor);

/*
 * The content of a source file is represented as a sequence of code fragments.
 * Each fragment is optionally given an indentation level.
 */
class CodeFragment {
 public:
  std::string content_{};
  int indent_{0};
};

class CodeCollection;

class SourceFile {
 private:
  std::vector<CodeFragment> fragments_{};
  int current_indent_{0};
  // The indentation level of the code fragment that was most recently updated
  void ChangeIndent(int n_tabs_delta);  // Add or remove indent
  void PushFragment(std::string content);
  friend std::ostream& operator<<(std::ostream&, CodeCollection const&);
  friend void WriteCodeToDisk(std::filesystem::path const& dirpath, CodeCollection const&);
  friend void WriteBuildRecipeToDisk(
      std::filesystem::path const&, std::string const&, CodeCollection const&);
  friend class CodeCollection;
};

class CodeCollection {
 private:
  std::map<std::string, SourceFile> sources_{};
  // sources_["xxx.c"] represents the content of the source file "xxx.c".
  std::string current_file_;
  // The source file that was most recently updated
 public:
  std::string GetCurrentSourceFile() {
    return current_file_;
  }
  void SwitchToSourceFile(std::string const& source_name);
  void ChangeIndent(int n_tabs_delta);
  void PushFragment(std::string content);

  friend std::ostream& operator<<(std::ostream&, CodeCollection const&);
  friend void WriteCodeToDisk(std::filesystem::path const&, CodeCollection const&);
  friend void WriteBuildRecipeToDisk(
      std::filesystem::path const&, std::string const&, CodeCollection const&);
};
std::ostream& operator<<(std::ostream& os, CodeCollection const& collection);

}  // namespace tl2cgen::compiler::detail::codegen

#endif  // TL2CGEN_DETAIL_COMPILER_CODEGEN_CODEGEN_H_
