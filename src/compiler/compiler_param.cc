/*!
 * Copyright (c) 2024 by Contributors
 * \file compiler_param.cc
 * \brief Compiler parameters
 * \author Hyunsu Cho
 */

#include <rapidjson/document.h>
#include <tl2cgen/compiler_param.h>
#include <tl2cgen/logging.h>

namespace tl2cgen::compiler {

CompilerParam CompilerParam::ParseFromJSON(char const* param_json_str) {
  CompilerParam param;

  rapidjson::Document doc;
  doc.Parse(param_json_str);
  TL2CGEN_CHECK(doc.IsObject()) << "Got an invalid JSON string:\n" << param_json_str;
  for (auto const& e : doc.GetObject()) {
    std::string const key = e.name.GetString();
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
    } else {
      TL2CGEN_LOG(FATAL) << "Unrecognized key '" << key << "' in JSON";
    }
  }

  return param;
}

}  // namespace tl2cgen::compiler
