/*!
 * Copyright (c) 2023 by Contributors
 * \file c_api_error.cc
 * \author Hyunsu Cho
 * \brief C error handling
 */
#include <tl2cgen/c_api.h>
#include <tl2cgen/c_api_error.h>
#include <tl2cgen/thread_local.h>
#include <tl2cgen/version.h>

#include <sstream>
#include <string>

// Stringify an integer macro constant
#define STR_IMPL_(x) #x
#define STR(x) STR_IMPL_(x)

namespace {

struct TL2cgenAPIErrorEntry {
  std::string last_error;
  std::string version_str;
};

using TL2cgenAPIErrorStore = tl2cgen::ThreadLocalStore<TL2cgenAPIErrorEntry>;

}  // anonymous namespace

char const* TL2cgenGetLastError() {
  return TL2cgenAPIErrorStore::Get()->last_error.c_str();
}

void TL2cgenAPISetLastError(char const* msg) {
  TL2cgenAPIErrorStore::Get()->last_error = msg;
}

char const* TL2cgenQueryTL2cgenVersion() {
  std::ostringstream oss;
  oss << TL2CGEN_VER_MAJOR << "." << TL2CGEN_VER_MINOR << "." << TL2CGEN_VER_PATCH;
  std::string& version_str = TL2cgenAPIErrorStore::Get()->version_str;
  version_str = oss.str();
  return version_str.c_str();
}

char const* TL2CGEN_VERSION = "TL2CGEN_VERSION_" STR(TREELITE_VER_MAJOR) "." STR(
    TREELITE_VER_MINOR) "." STR(TREELITE_VER_PATCH);
