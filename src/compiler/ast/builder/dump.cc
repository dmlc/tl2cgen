/*!
 * Copyright (c) 2023 by Contributors
 * \file dump.cc
 * \brief Generate text representation of AST
 * \author Hyunsu Cho
 */
#include <tl2cgen/detail/compiler/ast/builder.h>
#include <tl2cgen/logging.h>

#include <cstdint>

namespace {

using tl2cgen::compiler::detail::ast::ASTNode;

void get_dump_from_node(std::ostringstream* oss, ASTNode const* node, int indent) {
  (*oss) << std::string(indent, ' ') << node->GetDump() << "\n";
  for (ASTNode const* child : node->children) {
    TREELITE_CHECK(child);
    get_dump_from_node(oss, child, indent + 2);
  }
}

}  // anonymous namespace

namespace tl2cgen::compiler::detail::ast {

template <typename ThresholdType, typename LeafOutputType>
std::string ASTBuilder<ThresholdType, LeafOutputType>::GetDump() const {
  std::ostringstream oss;
  get_dump_from_node(&oss, this->main_node, 0);
  return oss.str();
}

template std::string ASTBuilder<float, std::uint32_t>::GetDump() const;
template std::string ASTBuilder<float, float>::GetDump() const;
template std::string ASTBuilder<double, std::uint32_t>::GetDump() const;
template std::string ASTBuilder<double, double>::GetDump() const;

}  // namespace tl2cgen::compiler::detail::ast
