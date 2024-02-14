/*!
 * Copyright (c) 2024 by Contributors
 * \file codegen.cc
 * \brief Convert AST into C code
 * \author Hyunsu Cho
 */

#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <tl2cgen/detail/compiler/ast/ast.h>
#include <tl2cgen/detail/compiler/codegen/codegen.h>
#include <tl2cgen/detail/compiler/codegen/format_util.h>
#include <tl2cgen/detail/filesystem.h>
#include <tl2cgen/logging.h>
#include <tl2cgen/predictor_types.h>

#include <fstream>
#include <string>
#include <type_traits>
#include <variant>

namespace {

template <typename T>
std::string GetCType() {
  if constexpr (std::is_same_v<T, float>) {
    return "float";
  } else if constexpr (std::is_same_v<T, double>) {
    return "double";
  } else {
    TL2CGEN_LOG(FATAL) << "Unrecognized type: " << typeid(T).name();
    return "";
  }
}

}  // anonymous namespace

namespace tl2cgen::compiler::detail::codegen {

void GenerateCodeFromAST(ast::ASTNode const* node, CodeCollection& gencode) {
  ast::MainNode const* t1;
  ast::FunctionNode const* t2;
  ast::ConditionNode const* t3;
  ast::OutputNode const* t4;
  ast::TranslationUnitNode const* t5;
  ast::QuantizerNode const* t6;
  if ((t1 = dynamic_cast<ast::MainNode const*>(node))) {
    HandleMainNode(t1, gencode);
  } else if ((t2 = dynamic_cast<ast::FunctionNode const*>(node))) {
    HandleFunctionNode(t2, gencode);
  } else if ((t3 = dynamic_cast<ast::ConditionNode const*>(node))) {
    HandleConditionNode(t3, gencode);
  } else if ((t4 = dynamic_cast<ast::OutputNode const*>(node))) {
    HandleOutputNode(t4, gencode);
  } else if ((t5 = dynamic_cast<ast::TranslationUnitNode const*>(node))) {
    HandleTranslationUnitNode(t5, gencode);
  } else if ((t6 = dynamic_cast<ast::QuantizerNode const*>(node))) {
    HandleQuantizerNode(t6, gencode);
  } else {
    TL2CGEN_LOG(FATAL) << "Unrecognized AST node type";
  }
}

void WriteCodeToDisk(std::filesystem::path const& dirpath, CodeCollection const& collection) {
  namespace fs = tl2cgen::detail::filesystem;
  for (auto const& [file_name, source_file] : collection.sources_) {
    std::ofstream of(dirpath / file_name);
    for (auto const& fragment : source_file.fragments_) {
      of << IndentMultiLineString(fragment.content_, fragment.indent_) << "\n";
    }
    of << "\n";
  }
}

void WriteBuildRecipeToDisk(std::filesystem::path const& dirpath,
    std::string const& native_lib_name, CodeCollection const& collection) {
  std::ofstream ofs(dirpath / "recipe.json");
  rapidjson::OStreamWrapper ofs_wrapped(ofs);
  rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(ofs_wrapped);
  writer.SetFormatOptions(rapidjson::PrettyFormatOptions::kFormatSingleLineArray);

  writer.StartObject();
  writer.Key("target");
  writer.String(native_lib_name);
  writer.Key("sources");
  writer.StartArray();
  for (auto const& [file_name, source_file] : collection.sources_) {
    if (file_name.compare(file_name.length() - 2, 2, ".c") == 0) {
      std::size_t line_count = 0;
      for (auto const& fragment : source_file.fragments_) {
        line_count += std::count(fragment.content_.begin(), fragment.content_.end(), '\n');
      }
      writer.StartObject();
      writer.Key("name");
      std::string name = file_name.substr(0, file_name.length() - 2);
      writer.String(name);
      writer.Key("length");
      writer.Uint64(line_count);
      writer.EndObject();
    }
  }
  writer.EndArray();
  writer.EndObject();
  ofs << "\n";  // Add newline at the end, for convention's sake
}

namespace pred = tl2cgen::predictor;

std::string GetThresholdTypeStr(ast::ASTNode const* node) {
  return std::visit(
      [](auto&& meta) {
        using TypeMetaT = std::remove_const_t<std::remove_reference_t<decltype(meta)>>;
        using ThresholdT = typename TypeMetaT::threshold_type;
        return pred::DataTypeToString(pred::DataTypeFromType<ThresholdT>());
      },
      node->meta_->type_meta_);
}

std::string GetThresholdCType(ast::ASTNode const* node) {
  return std::visit(
      [](auto&& meta) {
        using TypeMetaT = std::remove_const_t<std::remove_reference_t<decltype(meta)>>;
        using ThresholdT = typename TypeMetaT::threshold_type;
        return GetCType<ThresholdT>();
      },
      node->meta_->type_meta_);
}

std::string GetLeafOutputTypeStr(ast::ASTNode const* node) {
  return std::visit(
      [](auto&& meta) {
        using TypeMetaT = std::remove_const_t<std::remove_reference_t<decltype(meta)>>;
        using LeafOutputT = typename TypeMetaT::leaf_output_type;
        return pred::DataTypeToString(pred::DataTypeFromType<LeafOutputT>());
      },
      node->meta_->type_meta_);
}

std::string GetLeafOutputCType(ast::ModelMeta const& model_meta) {
  return std::visit(
      [](auto&& meta) {
        using TypeMetaT = std::remove_const_t<std::remove_reference_t<decltype(meta)>>;
        using LeafOutputT = typename TypeMetaT::leaf_output_type;
        return GetCType<LeafOutputT>();
      },
      model_meta.type_meta_);
}

std::string GetLeafOutputCType(ast::ASTNode const* node) {
  return GetLeafOutputCType(*node->meta_);
}

void SourceFile::ChangeIndent(int n_tabs_delta) {
  current_indent_ += n_tabs_delta * 2;  // 1 tab = 2 spaces for now
  TL2CGEN_CHECK_GE(current_indent_, 0);
}

void SourceFile::PushFragment(std::string content) {
  fragments_.push_back({std::move(content), current_indent_});
}

void CodeCollection::SwitchToSourceFile(std::string const& source_name) {
  current_file_ = source_name;
}

void CodeCollection::ChangeIndent(int n_tabs_delta) {
  sources_[current_file_].ChangeIndent(n_tabs_delta);
}

void CodeCollection::PushFragment(std::string content) {
  sources_[current_file_].PushFragment(content);
}

std::ostream& operator<<(std::ostream& os, CodeCollection const& collection) {
  for (auto const& [name, source_file] : collection.sources_) {
    os << "======== " << name << " ========"
       << "\n";
    for (auto const& fragment : source_file.fragments_) {
      os << IndentMultiLineString(fragment.content_, fragment.indent_) << "\n";
    }
    os << "\n";
  }
  return os;
}

}  // namespace tl2cgen::compiler::detail::codegen
