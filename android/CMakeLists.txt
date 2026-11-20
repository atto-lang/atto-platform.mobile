cmake_minimum_required(VERSION 3.20)
project(oresmobile_jni LANGUAGES C)

# Standalone Android NDK build of the reference C ABI with a JNI adapter.
# This is not a compiled Oreslang actor runtime.
add_library(oresmobile_jni SHARED
  src/main/cpp/ores_mobile_jni.c
  ../src/ores_mobile_bridge.c)
target_include_directories(oresmobile_jni PRIVATE ../include)
target_compile_features(oresmobile_jni PRIVATE c_std_11)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(oresmobile_jni PRIVATE
    -Wall -Wextra -Wpedantic -Werror
    -fvisibility=hidden -fstack-protector-strong)
  target_link_options(oresmobile_jni PRIVATE
    -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack)
endif()
