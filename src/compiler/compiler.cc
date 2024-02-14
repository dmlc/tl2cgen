/*!
 * Copyright (c) 2024 by Contributors
 * \file compiler.cc
 * \brief Compiler that generates C code from a tree model
 * \author Hyunsu Cho
 */
#include <tl2cgen/annotator.h>
#include <tl2cgen/compiler.h>
#include <tl2cgen/compiler_param.h>
#include <tl2cgen/detail/compiler/ast/builder.h>
#include <tl2cgen/detail/compiler/codegen/codegen.h>
#include <tl2cgen/detail/compiler/codegen/format_util.h>
#include <tl2cgen/detail/filesystem.h>
#include <treelite/tree.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace {

namespace detail = tl2cgen::compiler::detail;

// Lower the tree model to AST using the AST builder, and then return the builder object.
detail::ast::ASTBuilder LowerToAST(
    treelite::Model const& model, tl2cgen::compiler::CompilerParam const& param) {
  /* 1. Lower the tree ensemble model into Abstract Syntax Tree (AST) */
  detail::ast::ASTBuilder builder;
  builder.BuildAST(model);

  /* 2. Apply optimization passes to AST */
  if (param.annotate_in != "NULL") {
    auto annotation_path
        = std::filesystem::weakly_canonical(std::filesystem::u8path(param.annotate_in));
    tl2cgen::BranchAnnotator annotator;
    std::ifstream fi(annotation_path);
    annotator.Load(fi);
    auto const annotation = annotator.Get();
    builder.LoadDataCounts(annotation);
  }
  builder.SplitIntoTUs(param.parallel_comp);
  if (param.quantize > 0) {
    builder.GenerateIsCategoricalArray();
    builder.QuantizeThresholds();
  }
  return builder;
}

}  // anonymous namespace

namespace tl2cgen::compiler {

void CompileModel(treelite::Model const& model, CompilerParam const& param,
    std::filesystem::path const& dirpath) {
  tl2cgen::detail::filesystem::CreateDirectoryIfNotExist(dirpath);
  auto builder = LowerToAST(model, param);
  /* Generate C code */
  detail::codegen::CodeCollection gencode;
  detail::codegen::GenerateCodeFromAST(builder.GetRootNode(), gencode);
  // Write C code to disk
  detail::codegen::WriteCodeToDisk(dirpath, gencode);
  // Write recipe.json
  detail::codegen::WriteBuildRecipeToDisk(dirpath, param.native_lib_name, gencode);
}

std::string DumpAST(treelite::Model const& model, CompilerParam const& param) {
  auto builder = LowerToAST(model, param);
  return builder.GetDump();
}

}  // namespace tl2cgen::compiler
