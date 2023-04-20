/*!
 * Copyright (c) 2023 by Contributors
 * \file shared_library.h
 * \author Hyunsu Cho
 * \brief Abstraction for loading shared library
 */
#ifndef TL2CGEN_DETAIL_PREDICTOR_SHARED_LIBRARY_H_
#define TL2CGEN_DETAIL_PREDICTOR_SHARED_LIBRARY_H_

#include <string>

namespace tl2cgen::predictor::detail {

/*! \brief Abstraction for a shared library */
class SharedLibrary {
 public:
  using LibraryHandle = void*;
  using FunctionHandle = void*;

  SharedLibrary() : handle_(nullptr), libpath_() {}

  /*! \brief Load a shared library from a given path */
  explicit SharedLibrary(char const* libpath);
  ~SharedLibrary();
  /*! \brief Load a function with a given name */
  FunctionHandle LoadFunction(char const* name) const;
  /*! \brief Same as LoadFunction(), but with additional check to ensure that the loaded
   *         function can be represented as a given type of function pointer. */
  template <typename FuncPtrT>
  FuncPtrT LoadFunctionWithSignature(char const* name) const {
    auto func_handle = reinterpret_cast<FuncPtrT>(LoadFunction(name));
    TL2CGEN_CHECK(func_handle) << "Dynamic shared library `" << libpath_
                               << "' does not contain a function " << name
                               << "() with the requested signature";
    return func_handle;
  }

 private:
  LibraryHandle handle_;
  std::string libpath_;
};

}  // namespace tl2cgen::predictor::detail

#endif  // TL2CGEN_DETAIL_PREDICTOR_SHARED_LIBRARY_H_
