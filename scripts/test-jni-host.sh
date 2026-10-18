#!/usr/bin/env bash
set -euo pipefail

# Linux/host JNI smoke: compiles and loads the REAL JNI wrapper and C ABI.
# It does not compile Oreslang or exercise Android/iOS devices.
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cc="${1:-cc}"
: "${JAVA_HOME:?JAVA_HOME must point to a host JDK with JNI headers}"
test -f "$JAVA_HOME/include/jni.h"
test -f "$JAVA_HOME/include/linux/jni_md.h"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
"$cc" -std=c11 -Wall -Wextra -Wpedantic -Werror -fPIC -shared \
  -I"$repo_root/include" -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/linux" \
  "$repo_root/src/ores_mobile_bridge.c" \
  "$repo_root/android/src/main/cpp/ores_mobile_jni.c" \
  -o "$work/libores_mobile_jni.so"
javac -d "$work" \
  "$repo_root/tests/java/dev/oreslang/mobile/OresMobileNative.java" \
  "$repo_root/tests/java/dev/oreslang/mobile/OresMobileNativeTest.java"
java -Djava.library.path="$work" -cp "$work" \
  dev.oreslang.mobile.OresMobileNativeTest
