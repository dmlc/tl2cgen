/*!
 * Copyright (c) 2023 by Contributors
 * \file c_api.cc
 * \author Hyunsu Cho
 * \brief C API of TL2cgen, main package
 */

#include <tl2cgen/annotator.h>
#include <tl2cgen/c_api.h>
#include <tl2cgen/c_api_error.h>
#include <tl2cgen/logging.h>
#include <treelite/tree.h>

#include <fstream>

using namespace tl2cgen;  // NOLINT(build/namespaces)

int TL2cgenAnnotateBranch(TL2cgenModelHandle model, TL2cgenDMatrixHandle dmat, int nthread,
    int verbose, TL2cgenAnnotationHandle* out) {
  API_BEGIN();
  std::unique_ptr<BranchAnnotator> annotator{new BranchAnnotator()};
  treelite::Model const* model_ = static_cast<treelite::Model*>(model);
  auto const* dmat_ = static_cast<DMatrix const*>(dmat);
  TL2CGEN_CHECK(dmat_) << "Found a dangling reference to DMatrix";
  annotator->Annotate(*model_, dmat_, nthread, verbose);
  *out = static_cast<TL2cgenAnnotationHandle>(annotator.release());
  API_END();
}

int TL2cgenAnnotationSave(TL2cgenAnnotationHandle handle, char const* path) {
  API_BEGIN();
  BranchAnnotator const* annotator = static_cast<BranchAnnotator*>(handle);
  std::ofstream fo(path);
  annotator->Save(fo);
  API_END();
}

int TL2cgenAnnotationFree(TL2cgenAnnotationHandle handle) {
  API_BEGIN();
  delete static_cast<BranchAnnotator*>(handle);
  API_END();
}
