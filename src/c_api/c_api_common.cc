/*!
 * Copyright (c) 2023 by Contributors
 * \file c_api_common.cc
 * \author Hyunsu Cho
 * \brief C API of TL2cgen (this file is used by both runtime and main package)
 */

#include <tl2cgen/c_api_common.h>
#include <tl2cgen/c_api_error.h>
#include <tl2cgen/data_matrix.h>
#include <tl2cgen/logging.h>
#include <tl2cgen/thread_local.h>

#include <cstddef>
#include <cstdint>

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

int TL2cgenDMatrixCreateFromCSR(void const* data, char const* data_type_str,
    std::uint32_t const* col_ind, std::size_t const* row_ptr, std::size_t num_row,
    std::size_t num_col, TL2cgenDMatrixHandle* out) {
  API_BEGIN();

  std::unique_ptr<DMatrix> matrix = DMatrix::Create(DMatrixTypeEnum::kSparseCSR,
      DMatrixElementTypeFromString(data_type_str), data, col_ind, row_ptr, num_row, num_col);
  *out = static_cast<TL2cgenDMatrixHandle>(matrix.release());
  API_END();
}

int TL2cgenDMatrixCreateFromMat(void const* data, char const* data_type_str, std::size_t num_row,
    std::size_t num_col, void const* missing_value, TL2cgenDMatrixHandle* out) {
  API_BEGIN();
  std::unique_ptr<DMatrix> matrix = DMatrix::Create(DMatrixTypeEnum::kDenseCLayout,
      DMatrixElementTypeFromString(data_type_str), data, missing_value, num_row, num_col);
  *out = static_cast<TL2cgenDMatrixHandle>(matrix.release());
  API_END();
}

int TL2cgenDMatrixGetDimension(TL2cgenDMatrixHandle handle, std::size_t* out_num_row,
    std::size_t* out_num_col, std::size_t* out_nelem) {
  API_BEGIN();
  DMatrix const* dmat = static_cast<DMatrix*>(handle);
  *out_num_row = dmat->GetNumRow();
  *out_num_col = dmat->GetNumCol();
  *out_nelem = dmat->GetNumElem();
  API_END();
}

int TL2cgenDMatrixFree(TL2cgenDMatrixHandle handle) {
  API_BEGIN();
  delete static_cast<DMatrix*>(handle);
  API_END();
}
