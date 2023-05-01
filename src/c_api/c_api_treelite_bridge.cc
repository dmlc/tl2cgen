/*!
 * Copyright (c) 2023 by Contributors
 * \file c_api_treelite_bridge.cc
 * \author Hyunsu Cho
 * \brief Bridge to connect with Treelite
 */

#include <tl2cgen/c_api.h>
#include <tl2cgen/c_api_error.h>
#include <treelite/c_api.h>
#include <treelite/tree.h>

#include <cstddef>
#include <cstdint>

int TL2cgenLoadTreeliteModelFromBytes(char const* treelite_model_bytes,
    std::size_t treelite_model_bytes_len, TL2cgenModelHandle* out) {
  return TreeliteDeserializeModelFromBytes(treelite_model_bytes, treelite_model_bytes_len, out);
}

int TL2cgenQueryTreeliteModelVersion(TL2cgenModelHandle model, std::int32_t* major_ver,
    std::int32_t* minor_ver, std::int32_t* patch_ver) {
  API_BEGIN();
  auto const* model_ = static_cast<treelite::Model const*>(model);
  auto version = model_->GetVersion();
  *major_ver = version.major_ver;
  *minor_ver = version.minor_ver;
  *patch_ver = version.patch_ver;
  API_END();
}

int TL2cgenFreeTreeliteModel(TL2cgenModelHandle model) {
  return TreeliteFreeModel(model);
}
