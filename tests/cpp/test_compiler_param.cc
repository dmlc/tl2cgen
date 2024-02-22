/*!
 * Copyright (c) 2023 by Contributors
 * \file test_compiler_param.cc
 * \author Hyunsu Cho
 * \brief C++ tests for ingesting compiler parameters
 */
#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tl2cgen/compiler_param.h>
#include <tl2cgen/logging.h>

using namespace testing;  // NOLINT(build/namespaces)

namespace tl2cgen::compiler {

TEST(CompilerParam, Basic) {
  std::string json_str = R"JSON(
    {
      "quantize": 1,
      "parallel_comp": 100,
      "native_lib_name": "predictor",
      "annotate_in": "annotation.json",
      "verbose": 3
    })JSON";
  CompilerParam param = CompilerParam::ParseFromJSON(json_str.c_str());
  EXPECT_EQ(param.quantize, 1);
  EXPECT_EQ(param.parallel_comp, 100);
  EXPECT_EQ(param.native_lib_name, "predictor");
  EXPECT_EQ(param.annotate_in, "annotation.json");
  EXPECT_EQ(param.verbose, 3);
}

TEST(CompilerParam, NonExistentKey) {
  std::string json_str = R"JSON(
    {
      "quantize": 1,
      "parallel_comp": 100,
      "nonexistent": 0.3
    })JSON";
  EXPECT_THAT([&]() { CompilerParam::ParseFromJSON(json_str.c_str()); },
      ThrowsMessage<tl2cgen::Error>(HasSubstr("Unrecognized key 'nonexistent'")));
  json_str = R"JSON(
    {
      "quantize": 1,
      "parallel_comp": 100,
      "extra_object": {
        "extra": 30
      }
    })JSON";
  EXPECT_THAT([&]() { CompilerParam::ParseFromJSON(json_str.c_str()); },
      ThrowsMessage<tl2cgen::Error>(HasSubstr("Unrecognized key 'extra_object'")));
}

TEST(CompilerParam, IncorrectType) {
  std::string json_str = R"JSON(
    {
      "quantize": "bad_type"
    })JSON";
  EXPECT_THAT([&]() { CompilerParam::ParseFromJSON(json_str.c_str()); },
      ThrowsMessage<tl2cgen::Error>(HasSubstr("Expected an integer for 'quantize'")));
  json_str = R"JSON(
    {
      "native_lib_name": -10.0
    })JSON";
  EXPECT_THAT([&]() { CompilerParam::ParseFromJSON(json_str.c_str()); },
      ThrowsMessage<tl2cgen::Error>(HasSubstr("Expected a string for 'native_lib_name'")));
  json_str = R"JSON(
    {
      "parallel_comp": 13bad
    })JSON";
  EXPECT_THAT([&]() { CompilerParam::ParseFromJSON(json_str.c_str()); },
      ThrowsMessage<tl2cgen::Error>(HasSubstr("Got an invalid JSON string")));
}

TEST(CompilerParam, InvalidRange) {
  std::string json_str;
  for (auto const& key : std::vector<std::string>{"quantize", "parallel_comp"}) {
    std::string literal = "-1";
    json_str = fmt::format(R"JSON({{ "{0}": {1} }})JSON", key, literal);
    std::string expected_error = fmt::format("'{}' must be 0 or greater", key);
    EXPECT_THAT([&]() { CompilerParam::ParseFromJSON(json_str.c_str()); },
        ThrowsMessage<tl2cgen::Error>(HasSubstr(expected_error)));
  }
}

}  // namespace tl2cgen::compiler
