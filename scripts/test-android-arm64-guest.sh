#!/usr/bin/env bash
# Cross-compile the real Oreslang mobile scalar guest into an Android arm64-v8a JNI library.
set -euo pipefail
umask 077

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  echo "Usage: bash scripts/test-android-arm64-guest.sh /path/to/oreslang-llvmc [expected-sha256]" >&2
  exit 64
fi
if [ ! -f "$1" ] || [ -L "$1" ] || [ ! -x "$1" ]; then
  echo "Compiler path must be an executable regular file and not a symlink" >&2
  exit 64
fi

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
compiler="$1"
expected_sha256="${2:-}"
command -v cmake >/dev/null || { echo "CMake required." >&2; exit 69; }

sdk="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
if [ -z "$sdk" ] || [ ! -d "$sdk/ndk" ]; then
  echo "Android SDK/NDK not found via ANDROID_SDK_ROOT or ANDROID_HOME" >&2
  exit 69
fi
ndk="${ANDROID_NDK_LATEST_HOME:-}"
if [ -z "$ndk" ] || [ ! -d "$ndk" ]; then
  ndk="$(find "$sdk/ndk" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -1)"
fi
tool="$ndk/toolchains/llvm/prebuilt/linux-x86_64/bin"
cc="$tool/aarch64-linux-android24-clang"
readelf="$tool/llvm-readelf"
if [ ! -x "$cc" ] || [ ! -x "$readelf" ]; then
  echo "Android NDK AArch64 API-24 toolchain unavailable" >&2
  exit 69
fi

work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT

actual_sha256="$(cmake -E sha256sum "$compiler")"
actual_sha256="${actual_sha256%% *}"
if [ -n "$expected_sha256" ]; then
  expected_sha256="$(printf '%s' "$expected_sha256" | tr '[:upper:]' '[:lower:]')"
  if [ "$actual_sha256" != "$expected_sha256" ]; then
    echo "Compiler SHA-256 changed before Android guest compilation" >&2
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
printf 'ORES_MOBILE_ANDROID_COMPILER_SHA256=%s\n' "$pinned_sha256"

"$compiler_exec"   --tree-shake --tree-shake-root=mobile_guest_step   --target=aarch64-unknown-linux-android24   "$root/tests/fixtures/mobile_guest.ores" > "$work/guest.android.ll"

grep -F 'target triple = "aarch64-unknown-linux-android24"' "$work/guest.android.ll" >/dev/null
grep -F 'target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128-Fn32"' "$work/guest.android.ll" >/dev/null
count="$(grep -Ec '^define[[:space:]].*i64[[:space:]]+@"?mobile_guest_step"?\(i64[^,)]*\)' "$work/guest.android.ll" || true)"
if [ "$count" -ne 1 ]; then
  echo "Android guest symbol has missing/duplicate/wrong i64 ABI: mobile_guest_step" >&2
  exit 1
fi

"$cc" -x ir -fPIC -c "$work/guest.android.ll" -o "$work/mobile_guest.arm64.o"
"$cc" -std=c11 -Wall -Wextra -Wpedantic -Werror -fPIC -shared   "$root/tests/mobile_guest_jni.c" "$work/mobile_guest.arm64.o"   -o "$work/libores_mobile_guest_jni.so"

"$readelf" -h "$work/mobile_guest.arm64.o" > "$work/object.elf.txt"
"$readelf" -h "$work/libores_mobile_guest_jni.so" > "$work/shared.elf.txt"
grep -F 'Machine:                           AArch64' "$work/object.elf.txt" >/dev/null
grep -F 'Machine:                           AArch64' "$work/shared.elf.txt" >/dev/null
"$readelf" -Ws "$work/libores_mobile_guest_jni.so" |   grep -E 'mobile_guest_step|Java_dev_oreslang_mobile_OresMobileGuestNative_step' >/dev/null

ir_sha="$(cmake -E sha256sum "$work/guest.android.ll")"; ir_sha="${ir_sha%% *}"
obj_sha="$(cmake -E sha256sum "$work/mobile_guest.arm64.o")"; obj_sha="${obj_sha%% *}"
so_sha="$(cmake -E sha256sum "$work/libores_mobile_guest_jni.so")"; so_sha="${so_sha%% *}"
printf 'ORES_MOBILE_ANDROID_NDK=%s\n' "$ndk"
printf 'ORES_MOBILE_ANDROID_IR_SHA256=%s\n' "$ir_sha"
printf 'ORES_MOBILE_ANDROID_OBJECT_SHA256=%s\n' "$obj_sha"
printf 'ORES_MOBILE_ANDROID_JNI_SHA256=%s\n' "$so_sha"
echo "mobile real Oreslang -> Android ARM64 JNI artifact: PASS"
