/*!
 * Copyright (c) 2023 by Contributors
 * \file shared_library.cc
 * \author Hyunsu Cho
 * \brief Abstraction for loading shared library
 */

#include <tl2cgen/detail/predictor/shared_library.h>
#include <tl2cgen/logging.h>

#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace tl2cgen::predictor::detail {

SharedLibrary::SharedLibrary(char const* libpath) {
#ifdef _WIN32
  HMODULE handle = LoadLibraryA(libpath);
#else
  void* handle = dlopen(libpath, RTLD_LAZY | RTLD_LOCAL);
#endif
  TL2CGEN_CHECK(handle) << "Failed to load dynamic shared library `" << libpath << "'";
  handle_ = static_cast<LibraryHandle>(handle);
  libpath_ = std::string(libpath);
}

SharedLibrary::~SharedLibrary() {
  if (handle_) {
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle_));
#else
    dlclose(static_cast<void*>(handle_));
#endif
  }
}

SharedLibrary::FunctionHandle SharedLibrary::LoadFunction(char const* name) const {
  TL2CGEN_CHECK(handle_) << "Shared library was not yet loaded.";
#ifdef _WIN32
  FARPROC func_handle = GetProcAddress(static_cast<HMODULE>(handle_), name);
#else
  void* func_handle = dlsym(static_cast<void*>(handle_), name);
#endif
  TL2CGEN_CHECK(func_handle) << "Dynamic shared library `" << libpath_
                             << "' does not contain a function " << name << "().";
  return reinterpret_cast<SharedLibrary::FunctionHandle>(func_handle);
}

}  // namespace tl2cgen::predictor::detail
