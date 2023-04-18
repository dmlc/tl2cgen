/*!
 * Copyright (c) 2023 by Contributors
 * \file logging.cc
 * \author Hyunsu Cho
 * \brief logging facility for TL2cgen
 */

#include <tl2cgen/logging.h>

namespace tl2cgen {

void LogMessage::Log(std::string const& msg) {
  LogCallbackRegistry const* registry = LogCallbackRegistryStore::Get();
  auto callback = registry->GetCallbackLogInfo();
  callback(msg.c_str());
}

void LogMessageWarning::Log(std::string const& msg) {
  LogCallbackRegistry const* registry = LogCallbackRegistryStore::Get();
  auto callback = registry->GetCallbackLogWarning();
  callback(msg.c_str());
}

}  // namespace tl2cgen
