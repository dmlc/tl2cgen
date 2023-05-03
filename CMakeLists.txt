cmake_policy(SET CMP0091 NEW)
set(CMAKE_FIND_NO_INSTALL_PREFIX TRUE FORCE)
cmake_minimum_required(VERSION 3.16)
project(tl2cgen LANGUAGES CXX C VERSION 0.2.0)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# check MSVC version
if (MSVC)
  if (MSVC_VERSION LESS 1920)
    message(FATAL_ERROR "Need Visual Studio 2019 or newer to compile TL2cgen")
  endif ()
endif ()

option(TEST_COVERAGE "C++ test coverage" OFF)
option(USE_OPENMP "Use OpenMP" ON)
option(BUILD_DOXYGEN "Build documentation for C/C++ functions using Doxygen." OFF)
option(BUILD_CPP_TESTS "Build C++ tests" OFF)
option(HIDE_CXX_SYMBOLS "Hide all C++ symbols. Useful when building Pip package" OFF)
option(DETECT_CONDA_ENV "Enable detection of Conda environment for dependencies" ON)
option(BUILD_JVM_RUNTIME "Build TL2cgen runtime for JVM" OFF)
option(ENABLE_ALL_WARNINGS "Enable all compiler warnings. Only effective for GCC/Clang" OFF)
option(USE_SANITIZER "Use sanitizer flags" OFF)
SET(ENABLED_SANITIZERS "address" "leak" "undefined" CACHE STRING
    "Semicolon separated list of sanitizer names. E.g 'address;leak'.")

if (USE_SANITIZER)
  include(cmake/Sanitizer.cmake)
  enable_sanitizers("${ENABLED_SANITIZERS}")
endif ()

if (ENABLE_ALL_WARNINGS)
  if ((NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang") AND (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    message(SEND_ERROR "ENABLE_ALL_WARNINGS is only available for Clang and GCC.")
  endif ()
endif ()

if (BUILD_DOXYGEN)
  include(cmake/Doxygen.cmake)
  run_doxygen()
endif ()

# When installing dependencies, use Conda environment if available
if (DETECT_CONDA_ENV)
  if (DEFINED ENV{CONDA_PREFIX})
    set(CMAKE_PREFIX_PATH "$ENV{CONDA_PREFIX};${CMAKE_PREFIX_PATH}")
    message(STATUS "Detected Conda environment, CMAKE_PREFIX_PATH set to: ${CMAKE_PREFIX_PATH}")
    if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
      message(STATUS "No CMAKE_INSTALL_PREFIX argument detected, setting to: $ENV{CONDA_PREFIX}")
      set(CMAKE_INSTALL_PREFIX $ENV{CONDA_PREFIX})
    endif ()
  else ()
    message(STATUS "No Conda environment detected")
  endif ()
endif ()

list(APPEND CMAKE_MODULE_PATH "${tl2cgen_SOURCE_DIR}/cmake/modules")
include(cmake/ExternalLibs.cmake)
include(cmake/Utils.cmake)
include(cmake/Version.cmake)

add_subdirectory(src)
if (BUILD_JVM_RUNTIME)
  add_subdirectory(runtime/java)
endif ()
if (BUILD_CPP_TESTS)
  add_subdirectory(tests/cpp)
endif ()

add_library(tl2cgen SHARED)
target_link_libraries(tl2cgen PRIVATE obj_tl2cgen)

set(TL2CGEN_TARGETS tl2cgen)

foreach (lib ${TL2CGEN_TARGETS})
  set_output_directory(${lib} ${PROJECT_BINARY_DIR})
endforeach ()

# Export install targets
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(INSTALL_TARGETS ${TL2CGEN_TARGETS})
if (NOT FMTLIB_FROM_SYSTEM_ROOT)
  list(APPEND INSTALL_TARGETS fmt-header-only)
endif ()
if (NOT TREELITE_FROM_SYSTEM_ROOT)
  list(APPEND INSTALL_TARGETS treelite)
endif ()
install(TARGETS ${INSTALL_TARGETS}
    EXPORT TL2cgenTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION include)
install(DIRECTORY include/tl2cgen ${PROJECT_BINARY_DIR}/include/tl2cgen
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT TL2cgenTargets
    FILE TL2cgenTargets.cmake
    NAMESPACE tl2cgen::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tl2cgen)
configure_package_config_file(
    cmake/TL2cgenConfig.cmake.in
    ${PROJECT_BINARY_DIR}/cmake/TL2cgenConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tl2cgen)
write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/cmake/TL2cgenConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${PROJECT_BINARY_DIR}/cmake/TL2cgenConfig.cmake
    ${PROJECT_BINARY_DIR}/cmake/TL2cgenConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tl2cgen)

write_version()

set_default_configuration_release()
