/*!
 * Copyright (c) 2023 by Contributors
 * \file parallel_for.h
 * \brief Implementation of parallel for loop
 * \author Hyunsu Cho
 */
#ifndef TL2CGEN_DETAIL_THREADING_UTILS_PARALLEL_FOR_H_
#define TL2CGEN_DETAIL_THREADING_UTILS_PARALLEL_FOR_H_

#include <tl2cgen/detail/threading_utils/omp_config.h>
#include <tl2cgen/detail/threading_utils/omp_exception.h>
#include <tl2cgen/detail/threading_utils/omp_funcs.h>
#include <tl2cgen/logging.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <mutex>
#include <type_traits>

namespace tl2cgen::detail::threading_utils {

// OpenMP schedule
struct ParallelSchedule {
  enum {
    kAuto,
    kDynamic,
    kStatic,
    kGuided,
  } sched{kAuto};
  std::size_t chunk{0};

  ParallelSchedule static Auto() {
    return ParallelSchedule{kAuto};
  }
  ParallelSchedule static Dynamic(std::size_t n = 0) {
    return ParallelSchedule{kDynamic, n};
  }
  ParallelSchedule static Static(std::size_t n = 0) {
    return ParallelSchedule{kStatic, n};
  }
  ParallelSchedule static Guided() {
    return ParallelSchedule{kGuided};
  }
};

template <typename IndexType, typename FuncType>
inline void ParallelFor(IndexType begin, IndexType end, ThreadConfig const& thread_config,
    ParallelSchedule sched, FuncType func) {
  if (begin == end) {
    return;
  }

#if defined(_MSC_VER)
  // msvc doesn't support unsigned integer as openmp index.
  using OmpInd = std::conditional_t<std::is_signed<IndexType>::value, IndexType, std::int64_t>;
#else
  using OmpInd = IndexType;
#endif

  OMPException exc;
  switch (sched.sched) {
  case ParallelSchedule::kAuto: {
#pragma omp parallel for num_threads(thread_config.nthread)
    for (OmpInd i = begin; i < end; ++i) {
      exc.Run(func, static_cast<IndexType>(i), omp_get_thread_num());
    }
    break;
  }
  case ParallelSchedule::kDynamic: {
    if (sched.chunk == 0) {
#pragma omp parallel for num_threads(thread_config.nthread) schedule(dynamic)
      for (OmpInd i = begin; i < end; ++i) {
        exc.Run(func, static_cast<IndexType>(i), omp_get_thread_num());
      }
    } else {
#pragma omp parallel for num_threads(thread_config.nthread) schedule(dynamic, sched.chunk)
      for (OmpInd i = begin; i < end; ++i) {
        exc.Run(func, static_cast<IndexType>(i), omp_get_thread_num());
      }
    }
    break;
  }
  case ParallelSchedule::kStatic: {
    if (sched.chunk == 0) {
#pragma omp parallel for num_threads(thread_config.nthread) schedule(static)
      for (OmpInd i = begin; i < end; ++i) {
        exc.Run(func, static_cast<IndexType>(i), omp_get_thread_num());
      }
    } else {
#pragma omp parallel for num_threads(thread_config.nthread) schedule(static, sched.chunk)
      for (OmpInd i = begin; i < end; ++i) {
        exc.Run(func, static_cast<IndexType>(i), omp_get_thread_num());
      }
    }
    break;
  }
  case ParallelSchedule::kGuided: {
#pragma omp parallel for num_threads(thread_config.nthread) schedule(guided)
    for (OmpInd i = begin; i < end; ++i) {
      exc.Run(func, static_cast<IndexType>(i), omp_get_thread_num());
    }
    break;
  }
  }
  exc.Rethrow();
}

}  // namespace tl2cgen::detail::threading_utils

#endif  // TL2CGEN_DETAIL_THREADING_UTILS_PARALLEL_FOR_H_
