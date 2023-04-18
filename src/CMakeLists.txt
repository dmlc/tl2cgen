# Object library

add_library(obj_tl2cgen OBJECT)
target_link_libraries(obj_tl2cgen PRIVATE fmt::fmt-header-only RapidJSON::rapidjson)

add_library(obj_tl2cgen_runtime OBJECT)
# -ldl for UNIX-like systems
if (UNIX)
  target_link_libraries(obj_tl2cgen_runtime PUBLIC dl)
endif (UNIX)

add_library(obj_tl2cgen_common OBJECT)  # Component shared by both main package and runtime

if (USE_OPENMP)
  if (APPLE)
    find_package(OpenMP)
    if (NOT OpenMP_FOUND)
      # Try again with extra path info; required for libomp 15+ from Homebrew
      message(STATUS "OpenMP not found; attempting to locate libomp from Homebrew...")
      execute_process(COMMAND brew --prefix libomp
          OUTPUT_VARIABLE HOMEBREW_LIBOMP_PREFIX
          OUTPUT_STRIP_TRAILING_WHITESPACE)
      set(OpenMP_C_FLAGS
          "-Xpreprocessor -fopenmp -I${HOMEBREW_LIBOMP_PREFIX}/include")
      set(OpenMP_CXX_FLAGS
          "-Xpreprocessor -fopenmp -I${HOMEBREW_LIBOMP_PREFIX}/include")
      set(OpenMP_C_LIB_NAMES omp)
      set(OpenMP_CXX_LIB_NAMES omp)
      set(OpenMP_omp_LIBRARY ${HOMEBREW_LIBOMP_PREFIX}/lib/libomp.dylib)
      find_package(OpenMP REQUIRED)
    endif ()
  else ()
    find_package(OpenMP REQUIRED)
  endif ()
else ()
  message(STATUS "Disabling OpenMP")
endif ()

if (ENABLE_ALL_WARNINGS)
  foreach (target obj_tl2cgen obj_tl2cgen_runtime obj_tl2cgen_common)
    target_compile_options(${target} PRIVATE -Wall -Wextra)
  endforeach ()
endif ()

foreach (lib obj_tl2cgen obj_tl2cgen_runtime obj_tl2cgen_common)
  target_include_directories(${lib} PUBLIC
      $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
      $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)
  if (MSVC)
    target_compile_options(${lib} PRIVATE /MP)
    target_compile_definitions(${lib} PRIVATE -DNOMINMAX)
    target_compile_options(${lib} PRIVATE /utf-8 -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE)
  else ()
    target_compile_options(${lib} PRIVATE -funroll-loops)
  endif ()
  if (USE_OPENMP)
    target_link_libraries(${lib} PUBLIC OpenMP::OpenMP_CXX)
    target_compile_definitions(${lib} PUBLIC -DTL2CGEN_OPENMP_SUPPORT)
  endif ()
  if (TEST_COVERAGE)
    if (MSVC)
      message(FATAL_ERROR "Test coverage not available on Windows")
    endif ()
    target_compile_options(${lib} PUBLIC -g -O0 --coverage)
    target_link_options(${lib} PUBLIC --coverage)
  endif ()
endforeach ()

set_target_properties(obj_tl2cgen obj_tl2cgen_runtime obj_tl2cgen_common
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

target_sources(obj_tl2cgen
    PRIVATE
    c_api/c_api.cc)

target_sources(obj_tl2cgen_runtime
    PRIVATE
    c_api/c_api_runtime.cc)

target_sources(obj_tl2cgen_common
    PRIVATE
    c_api/c_api_common.cc
    c_api/c_api_error.cc
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/c_api_common.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/c_api_error.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/error.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/logging.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/thread_local.h)
