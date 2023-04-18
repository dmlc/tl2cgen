/*!
 * Copyright (c) 2023 by Contributors
 * \file thread_local.h
 * \brief Helper class for thread-local storage
 * \author Hyunsu Cho
 */
#ifndef TL2CGEN_THREAD_LOCAL_H_
#define TL2CGEN_THREAD_LOCAL_H_

namespace tl2cgen {

/*!
 * \brief A thread-local storage
 * \tparam T the type we like to store
 */
template <typename T>
class ThreadLocalStore {
 public:
  /*! \return get a thread local singleton */
  static T* Get() {
    static thread_local T inst;
    return &inst;
  }
};

}  // namespace tl2cgen

#endif  // TL2CGEN_THREAD_LOCAL_H_
