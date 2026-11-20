cmake_minimum_required(VERSION 3.20)
project(ores_mobile_bridge VERSION 0.1.0 LANGUAGES C)
include(CTest)

option(ORES_MOBILE_REQUIRE_LLVM_GUEST
  "Require a provenance-pinned Oreslang LLVM compiler for JNI guest execution" OFF)
set(ORES_MOBILE_LLVMC "" CACHE FILEPATH
  "Absolute path to an independently built oreslang-llvmc")
set(ORES_MOBILE_LLVMC_SHA256 "" CACHE STRING
  "Expected SHA-256 digest of the independently built oreslang-llvmc")

if(ORES_MOBILE_REQUIRE_LLVM_GUEST)
  if(NOT BUILD_TESTING OR CMAKE_CROSSCOMPILING OR WIN32 OR APPLE)
    message(FATAL_ERROR
      "Required mobile LLVM/JNI guest proof currently needs a native Linux CTest build")
  endif()
  if(NOT ORES_MOBILE_LLVMC)
    message(FATAL_ERROR "ORES_MOBILE_REQUIRE_LLVM_GUEST=ON needs ORES_MOBILE_LLVMC")
  endif()
  if(NOT ORES_MOBILE_LLVMC_SHA256)
    message(FATAL_ERROR "Real mobile guest execution requires ORES_MOBILE_LLVMC_SHA256")
  endif()
  if(NOT IS_ABSOLUTE "${ORES_MOBILE_LLVMC}" OR
     NOT EXISTS "${ORES_MOBILE_LLVMC}" OR
     IS_DIRECTORY "${ORES_MOBILE_LLVMC}" OR
     IS_SYMLINK "${ORES_MOBILE_LLVMC}")
    message(FATAL_ERROR
      "ORES_MOBILE_LLVMC must name an existing absolute file that is not a symlink")
  endif()
  string(TOLOWER "${ORES_MOBILE_LLVMC_SHA256}" expected_mobile_compiler_sha256)
  string(LENGTH "${expected_mobile_compiler_sha256}" mobile_sha_length)
  if(NOT mobile_sha_length EQUAL 64 OR
     NOT expected_mobile_compiler_sha256 MATCHES "^[0-9a-f]+$")
    message(FATAL_ERROR "ORES_MOBILE_LLVMC_SHA256 must be 64 hexadecimal digits")
  endif()
  file(SHA256 "${ORES_MOBILE_LLVMC}" actual_mobile_compiler_sha256)
  if(NOT actual_mobile_compiler_sha256 STREQUAL expected_mobile_compiler_sha256)
    message(FATAL_ERROR
      "ORES_MOBILE_LLVMC_SHA256 disagrees with the binary at the configured path")
  endif()
  find_program(ORES_MOBILE_BASH bash REQUIRED)
  find_program(ORES_MOBILE_CLANG clang REQUIRED)
  find_program(ORES_MOBILE_JAVA java REQUIRED)
  find_program(ORES_MOBILE_JAVAC javac REQUIRED)
  if(NOT DEFINED ENV{JAVA_HOME} OR NOT EXISTS "$ENV{JAVA_HOME}/include/jni.h")
    message(FATAL_ERROR "Required mobile LLVM/JNI guest proof needs JAVA_HOME with JNI headers")
  endif()
endif()

add_library(ores_mobile_bridge STATIC src/ores_mobile_bridge.c)
target_include_directories(ores_mobile_bridge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(ores_mobile_bridge PUBLIC c_std_11)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(ores_mobile_bridge PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
  if(ORES_MOBILE_REQUIRE_LLVM_GUEST)
    add_test(NAME mobile_real_llvm_jni_guest
      COMMAND "${ORES_MOBILE_BASH}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-llvm-jni-guest.sh"
        "${ORES_MOBILE_LLVMC}" "${expected_mobile_compiler_sha256}")
    set_tests_properties(mobile_real_llvm_jni_guest PROPERTIES
      TIMEOUT 120 LABELS "real-guest;compiler-required;jni")
  endif()
  add_executable(test_mobile_bridge tests/test_mobile_bridge.c)
  if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_mobile_bridge PRIVATE -UNDEBUG)
  endif()
  target_link_libraries(test_mobile_bridge PRIVATE ores_mobile_bridge)
  add_test(NAME mobile_bridge_contract COMMAND test_mobile_bridge)
  add_executable(mobile_host_demo examples/host_demo.c)
  target_link_libraries(mobile_host_demo PRIVATE ores_mobile_bridge)
  add_test(NAME mobile_host_demo COMMAND mobile_host_demo)
endif()
