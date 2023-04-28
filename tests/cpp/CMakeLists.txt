add_executable(tl2cgen_cpp_test)
set_target_properties(tl2cgen_cpp_test
    PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)
target_link_libraries(tl2cgen_cpp_test
    PRIVATE obj_tl2cgen obj_tl2cgen_runtime obj_tl2cgen_common rapidjson
    GTest::GTest GTest::gmock fmt::fmt-header-only)
set_output_directory(tl2cgen_cpp_test ${PROJECT_BINARY_DIR})

if (MSVC)
  target_compile_options(tl2cgen_cpp_test PRIVATE
      /utf-8 -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE)
endif ()

if (TEST_COVERAGE)
  if (MSVC)
    message(FATAL_ERROR "Test coverage not available on Windows")
  endif ()
  target_compile_options(tl2cgen_cpp_test PUBLIC -g -O0 --coverage)
  target_link_options(tl2cgen_cpp_test PUBLIC --coverage)
endif ()

target_sources(tl2cgen_cpp_test
    PRIVATE
    test_main.cc
    test_compiler_param.cc
    test_threading_utils.cc)

# In MSVC solution, group sources according to directories
file(GLOB_RECURSE TEST_SOURCES *.cc *.h)
auto_source_group("${TEST_SOURCES}")
