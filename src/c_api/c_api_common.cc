/*!
 * Copyright (c) 2023 by Contributors
 * \file c_api_common.cc
 * \author Hyunsu Cho
 * \brief C API of tl2cgen (this file is used by both runtime and main package)
 */

#include <tl2cgen/c_api_common.h>
#include <tl2cgen/c_api_error.h>
#include <tl2cgen/logging.h>
#include <tl2cgen/thread_local.h>

using namespace tl2cgen;  // NOLINT(build/namespaces)

int TL2cgenRegisterLogCallback(void (*callback)(char const*)) {
  API_BEGIN();
  LogCallbackRegistry* registry = LogCallbackRegistryStore::Get();
  registry->RegisterCallBackLogInfo(callback);
  API_END();
}

int TL2cgenRegisterWarningCallback(void (*callback)(char const*)) {
  API_BEGIN();
  LogCallbackRegistry* registry = LogCallbackRegistryStore::Get();
  registry->RegisterCallBackLogWarning(callback);
  API_END();
}
