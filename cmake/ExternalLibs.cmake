include(FetchContent)

# Treelite
find_package(Treelite 3.4.0)
if (Treelite_FOUND)
  set(TREELITE_FROM_SYSTEM_ROOT TRUE)
  set(TREELITE_LIB treelite::treelite)
else ()
  message(STATUS "Did not find Treelite in the system root. Fetching Treelite now...")
  FetchContent_Declare(
    treelite
    GIT_REPOSITORY https://github.com/dmlc/treelite.git
    GIT_TAG 3.4.0
  )
  set(Treelite_BUILD_STATIC_LIBS ON)
  FetchContent_MakeAvailable(treelite)
  set_target_properties(treelite treelite_runtime PROPERTIES EXCLUDE_FROM_ALL TRUE)
  target_include_directories(treelite_static PUBLIC
      $<BUILD_INTERFACE:${treelite_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${treelite_BINARY_DIR}/include>
      $<INSTALL_INTERFACE:include>)
  set(TREELITE_LIB treelite_static)
  set(TREELITE_FROM_SYSTEM_ROOT FALSE)
endif ()

# fmtlib
find_package(fmt)
if (fmt_FOUND)
  get_target_property(fmt_loc fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
  message(STATUS "Found fmtlib at ${fmt_loc}")
  set(FMTLIB_FROM_SYSTEM_ROOT TRUE)
else ()
  message(STATUS "Did not find fmtlib in the system root. Fetching fmtlib now...")
  FetchContent_Declare(
      fmtlib
      GIT_REPOSITORY https://github.com/fmtlib/fmt.git
      GIT_TAG 9.1.0
  )
  FetchContent_MakeAvailable(fmtlib)
  set_target_properties(fmt PROPERTIES EXCLUDE_FROM_ALL TRUE)
  set(FMTLIB_FROM_SYSTEM_ROOT FALSE)
endif ()

# RapidJSON (header-only library)
if (NOT TARGET rapidjson)
  add_library(rapidjson INTERFACE)
  find_package(RapidJSON)
  if (RapidJSON_FOUND)
    target_include_directories(rapidjson INTERFACE ${RAPIDJSON_INCLUDE_DIRS})
  else ()
    message(STATUS "Did not find RapidJSON in the system root. Fetching RapidJSON now...")
    FetchContent_Declare(
        RapidJSON
        GIT_REPOSITORY https://github.com/Tencent/rapidjson
        GIT_TAG v1.1.0
    )
    FetchContent_Populate(RapidJSON)
    message(STATUS "RapidJSON was downloaded at ${rapidjson_SOURCE_DIR}.")
    target_include_directories(rapidjson INTERFACE $<BUILD_INTERFACE:${rapidjson_SOURCE_DIR}/include>)
  endif ()
  add_library(RapidJSON::rapidjson ALIAS rapidjson)
endif ()

# Google C++ tests
if (BUILD_CPP_TESTS)
  find_package(GTest 1.11.0 CONFIG)
  if (NOT GTEST_FOUND)
    message(STATUS "Did not find Google Test in the system root. Fetching Google Test now...")
    set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.11.0
    )
    FetchContent_MakeAvailable(googletest)
    add_library(GTest::GTest ALIAS gtest)
    add_library(GTest::gmock ALIAS gmock)
  endif ()
endif ()
