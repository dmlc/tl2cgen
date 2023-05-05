find_package(JNI REQUIRED COMPONENTS JVM OPTIONAL_COMPONENTS AWT)

add_library(tl2cgen4j SHARED tl2cgen4j/src/native/tl2cgen4j.cpp)
target_link_libraries(tl2cgen4j PRIVATE obj_tl2cgen)
target_include_directories(tl2cgen4j PUBLIC ${JNI_INCLUDE_DIRS})
set_target_properties(tl2cgen4j
  PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON)
