# Object library

add_library(obj_tl2cgen OBJECT)
target_link_libraries(obj_tl2cgen PRIVATE fmt::fmt-header-only RapidJSON::rapidjson)

# -ldl for UNIX-like systems
if (UNIX)
  target_link_libraries(obj_tl2cgen PUBLIC dl)
endif (UNIX)

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
  foreach (target obj_tl2cgen)
    target_compile_options(${target} PRIVATE -Wall -Wextra)
  endforeach ()
endif ()

target_include_directories(obj_tl2cgen PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)
if (MSVC)
  target_compile_options(obj_tl2cgen PRIVATE /MP)
  target_compile_definitions(obj_tl2cgen PRIVATE -DNOMINMAX)
  target_compile_options(obj_tl2cgen PRIVATE /utf-8 -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE)
else ()
  target_compile_options(obj_tl2cgen PRIVATE -funroll-loops)
endif ()
if (USE_OPENMP)
  target_link_libraries(obj_tl2cgen PUBLIC OpenMP::OpenMP_CXX)
  target_compile_definitions(obj_tl2cgen PUBLIC -DTL2CGEN_OPENMP_SUPPORT)
endif ()
if (TEST_COVERAGE)
  if (MSVC)
    message(FATAL_ERROR "Test coverage not available on Windows")
  endif ()
  target_compile_options(obj_tl2cgen PUBLIC -g -O0 --coverage)
  target_link_options(obj_tl2cgen PUBLIC --coverage)
endif ()

set_target_properties(obj_tl2cgen
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)

if (HIDE_CXX_SYMBOLS)
  set_target_properties(obj_tl2cgen
      PROPERTIES
      C_VISIBILITY_PRESET hidden
      CXX_VISIBILITY_PRESET hidden)
endif (HIDE_CXX_SYMBOLS)

target_sources(obj_tl2cgen
    PRIVATE
    annotator.cc
    filesystem.cc
    logging.cc
    c_api/c_api.cc
    c_api/c_api_error.cc
    c_api/c_api_treelite_bridge.cc
    compiler/compiler.cc
    compiler/compiler_param.cc
    compiler/ast/build.cc
    compiler/ast/dump.cc
    compiler/ast/is_categorical_array.cc
    compiler/ast/load_data_counts.cc
    compiler/ast/quantize.cc
    compiler/ast/split.cc
    compiler/codegen/codegen.cc
    compiler/codegen/condition_node.cc
    compiler/codegen/function_node.cc
    compiler/codegen/main_node.cc
    compiler/codegen/output_node.cc
    compiler/codegen/postprocessor.cc
    compiler/codegen/quantizer_node.cc
    compiler/codegen/translation_unit_node.cc
    predictor/predictor.cc
    predictor/shared_library.cc
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/annotator.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/c_api.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/c_api_error.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/compiler.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/compiler_param.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/data_matrix.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/data_matrix_types.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/error.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/logging.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/predictor.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/predictor_types.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/thread_local.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/detail/data_matrix_impl.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/detail/filesystem.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/detail/math_funcs.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/detail/operator_comp.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/detail/compiler/ast/ast.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/detail/compiler/ast/builder.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/detail/compiler/codegen/codegen.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/detail/compiler/codegen/format_util.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/detail/predictor/shared_library.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/detail/threading_utils/omp_config.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/detail/threading_utils/omp_exception.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/detail/threading_utils/omp_funcs.h
    ${PROJECT_SOURCE_DIR}/include/tl2cgen/detail/threading_utils/parallel_for.h)

target_link_libraries(obj_tl2cgen PRIVATE ${TREELITE_LIB} std::mdspan)
