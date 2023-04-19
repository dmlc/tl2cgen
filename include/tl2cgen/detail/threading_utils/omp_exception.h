/*!
 * Copyright (c) 2023 by Contributors
 * \file omp_exception.h
 * \author Hyunsu Cho
 * \brief Utility to propagate exceptions throws inside an OpenMP block
 */
#ifndef TL2CGEN_DETAIL_THREADING_UTILS_OMP_EXCEPTION_H_
#define TL2CGEN_DETAIL_THREADING_UTILS_OMP_EXCEPTION_H_

#include <tl2cgen/error.h>

#include <exception>
#include <mutex>

namespace tl2cgen::detail::threading_utils {

/*!
 * \brief OMP Exception class catches, saves and rethrows exception from OMP blocks
 */
class OMPException {
 private:
  // exception_ptr member to store the exception
  std::exception_ptr omp_exception_;
  // Mutex to be acquired during catch to set the exception_ptr
  std::mutex mutex_;

 public:
  /*!
   * \brief Parallel OMP blocks should be placed within Run to save exception
   */
  template <typename Function, typename... Parameters>
  void Run(Function f, Parameters... params) {
    try {
      f(params...);
    } catch (tl2cgen::Error&) {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!omp_exception_) {
        omp_exception_ = std::current_exception();
      }
    } catch (std::exception&) {
      std::lock_guard<std::mutex> lock(mutex_);
      if (!omp_exception_) {
        omp_exception_ = std::current_exception();
      }
    }
  }

  /*!
   * \brief should be called from the main thread to rethrow the exception
   */
  void Rethrow() {
    if (this->omp_exception_) {
      std::rethrow_exception(this->omp_exception_);
    }
  }
};

}  // namespace tl2cgen::detail::threading_utils

#endif  // TL2CGEN_DETAIL_THREADING_UTILS_OMP_EXCEPTION_H_
