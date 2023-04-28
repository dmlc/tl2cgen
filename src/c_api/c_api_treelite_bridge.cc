/*!
 * Copyright (c) 2023 by Contributors
 * \file c_api_treelite_bridge.cc
 * \author Hyunsu Cho
 * \brief Bridge to connect with Treelite
 */

#include <tl2cgen/c_api.h>
#include <treelite/c_api.h>

#include <cstddef>

int TL2cgenLoadTreeliteModelFromBytes(char const* treelite_model_bytes,
    std::size_t treelite_model_bytes_len, TL2cgenModelHandle* out) {
  return TreeliteDeserializeModelFromBytes(treelite_model_bytes, treelite_model_bytes_len, out);
}

int TL2cgenFreeTreeliteModel(TL2cgenModelHandle model) {
  return TreeliteFreeModel(model);
}
