#!/usr/bin/env bash
# Real Oreslang LLVM -> native object -> JNI shared library -> Java execution proof.
set -euo pipefail
umask 077

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  echo "Usage: bash scripts/test-llvm-jni-guest.sh /path/to/oreslang-llvmc [expected-sha256]" >&2
  exit 64
fi
if [ ! -f "$1" ] || [ -L "$1" ] || [ ! -x "$1" ]; then
  echo "Compiler path must be an executable regular file and not a symlink" >&2
  exit 64
fi

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
compiler="$1"
expected_sha256="${2:-}"
: "${JAVA_HOME:?JAVA_HOME must point to a host JDK with JNI headers}"
test -f "$JAVA_HOME/include/jni.h"
test -f "$JAVA_HOME/include/linux/jni_md.h"
command -v cmake >/dev/null || { echo "CMake required." >&2; exit 69; }
command -v clang >/dev/null || { echo "Clang required." >&2; exit 69; }
command -v javac >/dev/null || { echo "javac required." >&2; exit 69; }
command -v java >/dev/null || { echo "java required." >&2; exit 69; }

work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT

actual_sha256="$(cmake -E sha256sum "$compiler")"
actual_sha256="${actual_sha256%% *}"
if [ -n "$expected_sha256" ]; then
  expected_sha256="$(printf '%s' "$expected_sha256" | tr '[:upper:]' '[:lower:]')"
  if [ "$actual_sha256" != "$expected_sha256" ]; then
    echo "Compiler SHA-256 changed before mobile guest execution" >&2
    exit 65
  fi
fi

compiler_exec="$work/oreslang-llvmc"
cp "$compiler" "$compiler_exec"
chmod 500 "$compiler_exec"
pinned_sha256="$(cmake -E sha256sum "$compiler_exec")"
pinned_sha256="${pinned_sha256%% *}"
test "$pinned_sha256" = "$actual_sha256" || {
  echo "Compiler bytes changed while pinning private execution copy" >&2
  exit 65
}
printf 'ORES_MOBILE_COMPILER_SHA256=%s\n' "$pinned_sha256"

"$compiler_exec" --tree-shake   --tree-shake-root=mobile_guest_step   --target=host "$root/tests/fixtures/mobile_guest.ores" > "$work/guest.ll"

count="$(grep -Ec '^define[[:space:]].*i64[[:space:]]+@"?mobile_guest_step"?\(i64[^,)]*\)' "$work/guest.ll" || true)"
if [ "$count" -ne 1 ]; then
  echo "mobile guest symbol has missing/duplicate/wrong i64 ABI: mobile_guest_step" >&2
  exit 1
fi
ir_sha256="$(cmake -E sha256sum "$work/guest.ll")"
ir_sha256="${ir_sha256%% *}"
printf 'ORES_MOBILE_GUEST_IR_SHA256=%s\n' "$ir_sha256"

clang -x ir -fPIC -c "$work/guest.ll" -o "$work/guest.o"
object_sha256="$(cmake -E sha256sum "$work/guest.o")"
object_sha256="${object_sha256%% *}"
printf 'ORES_MOBILE_GUEST_OBJECT_SHA256=%s\n' "$object_sha256"

clang -std=c11 -Wall -Wextra -Wpedantic -Werror -fPIC -shared   -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/linux"   "$root/tests/mobile_guest_jni.c" "$work/guest.o"   -o "$work/libores_mobile_guest_jni.so"
library_before="$(cmake -E sha256sum "$work/libores_mobile_guest_jni.so")"
library_before="${library_before%% *}"
printf 'ORES_MOBILE_GUEST_JNI_SHA256=%s\n' "$library_before"

javac -d "$work"   "$root/tests/java/dev/oreslang/mobile/OresMobileGuestNative.java"   "$root/tests/java/dev/oreslang/mobile/OresMobileGuestNativeTest.java"
output="$(java -Djava.library.path="$work" -cp "$work"   dev.oreslang.mobile.OresMobileGuestNativeTest)"
printf '%s\n' "$output"
grep -Fq 'mobile real Oreslang LLVM -> JNI -> Java: PASS' <<<"$output"

library_after="$(cmake -E sha256sum "$work/libores_mobile_guest_jni.so")"
library_after="${library_after%% *}"
test "$library_after" = "$library_before" || {
  echo "JNI shared library changed during guest execution" >&2
  exit 66
}
printf 'ORES_MOBILE_GUEST_JNI_POSTRUN_SHA256=%s\n' "$library_after"
