/*!
 * Copyright (c) 2023 by Contributors
 * \file omp_config.h
 * \brief Configuration for OpenMP
 * \author Hyunsu Cho
 */
#ifndef TL2CGEN_DETAIL_THREADING_UTILS_OMP_CONFIG_H_
#define TL2CGEN_DETAIL_THREADING_UTILS_OMP_CONFIG_H_

#include <tl2cgen/detail/threading_utils/omp_funcs.h>
#include <tl2cgen/logging.h>

#include <algorithm>
#include <cstdint>

namespace tl2cgen::detail::threading_utils {

inline int OmpGetThreadLimit() {
  int limit = omp_get_thread_limit();
  TL2CGEN_CHECK_GE(limit, 1) << "Invalid thread limit for OpenMP";
  return limit;
}

inline int MaxNumThread() {
  return std::min(std::min(omp_get_num_procs(), omp_get_max_threads()), OmpGetThreadLimit());
}

/*!
 * \brief Represent thread configuration, to be used with parallel loops.
 */
struct ThreadConfig {
  std::uint32_t nthread;
};

/*!
 * \brief Create therad configuration.
 * @param nthread Number of threads to use. If \<= 0, use all available threads. This value is
 *                validated to ensure that it's in a valid range.
 * @return Thread configuration
 */
inline ThreadConfig ConfigureThreadConfig(int nthread) {
  if (nthread <= 0) {
    nthread = MaxNumThread();
    TL2CGEN_CHECK_GE(nthread, 1) << "Invalid number of threads configured in OpenMP";
  } else {
    TL2CGEN_CHECK_LE(nthread, MaxNumThread())
        << "nthread cannot exceed " << MaxNumThread() << " (configured by OpenMP)";
  }
  return ThreadConfig{static_cast<std::uint32_t>(nthread)};
}

}  // namespace tl2cgen::detail::threading_utils

#endif  // TL2CGEN_DETAIL_THREADING_UTILS_OMP_CONFIG_H_
