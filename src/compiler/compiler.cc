/*!
 * Copyright (c) 2023 by Contributors
 * \file compiler.cc
 * \brief Registry of compilers
 */
#include <rapidjson/document.h>
#include <tl2cgen/compiler.h>
#include <tl2cgen/compiler_param.h>
#include <tl2cgen/detail/compiler/ast_native.h>
#include <tl2cgen/detail/compiler/failsafe.h>
#include <tl2cgen/logging.h>

#include <limits>

namespace tl2cgen {

Compiler* Compiler::Create(std::string const& name, char const* param_json_str) {
  compiler::CompilerParam param = compiler::CompilerParam::ParseFromJSON(param_json_str);
  if (name == "ast_native") {
    return new compiler::detail::ASTNativeCompiler(param);
  } else if (name == "failsafe") {
    return new compiler::detail::FailSafeCompiler(param);
  } else {
    TL2CGEN_LOG(FATAL) << "Unrecognized compiler '" << name << "'";
    return nullptr;
  }
}

namespace compiler {

CompilerParam CompilerParam::ParseFromJSON(char const* param_json_str) {
  CompilerParam param;
  // Default values
  param.annotate_in = "NULL";
  param.quantize = 0;
  param.parallel_comp = 0;
  param.verbose = 0;
  param.native_lib_name = "predictor";
  param.code_folding_req = std::numeric_limits<double>::infinity();
  param.dump_array_as_elf = 0;

  rapidjson::Document doc;
  doc.Parse(param_json_str);
  TL2CGEN_CHECK(doc.IsObject()) << "Got an invalid JSON string:\n" << param_json_str;
  for (auto const& e : doc.GetObject()) {
    const std::string key = e.name.GetString();
    if (key == "annotate_in") {
      TL2CGEN_CHECK(e.value.IsString()) << "Expected a string for 'annotate_in'";
      param.annotate_in = e.value.GetString();
    } else if (key == "quantize") {
      TL2CGEN_CHECK(e.value.IsInt()) << "Expected an integer for 'quantize'";
      param.quantize = e.value.GetInt();
      TL2CGEN_CHECK_GE(param.quantize, 0) << "'quantize' must be 0 or greater";
    } else if (key == "parallel_comp") {
      TL2CGEN_CHECK(e.value.IsInt()) << "Expected an integer for 'parallel_comp'";
      param.parallel_comp = e.value.GetInt();
      TL2CGEN_CHECK_GE(param.parallel_comp, 0) << "'parallel_comp' must be 0 or greater";
    } else if (key == "verbose") {
      TL2CGEN_CHECK(e.value.IsInt()) << "Expected an integer for 'verbose'";
      param.verbose = e.value.GetInt();
    } else if (key == "native_lib_name") {
      TL2CGEN_CHECK(e.value.IsString()) << "Expected a string for 'native_lib_name'";
      param.native_lib_name = e.value.GetString();
    } else if (key == "code_folding_req") {
      TL2CGEN_CHECK(e.value.IsDouble())
          << "Expected a floating-point decimal for 'code_folding_req'";
      param.code_folding_req = e.value.GetDouble();
      TL2CGEN_CHECK_GE(param.code_folding_req, 0) << "'code_folding_req' must be 0 or greater";
    } else if (key == "dump_array_as_elf") {
      TL2CGEN_CHECK(e.value.IsInt()) << "Expected an integer for 'dump_array_as_elf'";
      param.dump_array_as_elf = e.value.GetInt();
      TL2CGEN_CHECK_GE(param.dump_array_as_elf, 0) << "'dump_array_as_elf' must be 0 or greater";
    } else {
      TL2CGEN_LOG(FATAL) << "Unrecognized key '" << key << "' in JSON";
    }
  }

  return param;
}

}  // namespace compiler

}  // namespace tl2cgen
