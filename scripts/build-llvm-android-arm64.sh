#!/usr/bin/env bash
# Build real Oreslang scalar IR into Android ARM64 artifacts with the NDK.
# This proves NDK retargeting of the current scalar LLVM subset; oreslang-llvmc
# itself does not yet expose a certified Android target.
set -euo pipefail
umask 077

if [ "$#" -lt 1 ] || [ "$#" -gt 3 ]; then
  echo "Usage: bash scripts/build-llvm-android-arm64.sh /path/to/oreslang-llvmc [expected-sha256] [api-level]" >&2
  exit 64
fi
if [ ! -f "$1" ] || [ -L "$1" ] || [ ! -x "$1" ]; then
  echo "Compiler path must be an executable regular file and not a symlink" >&2
  exit 64
fi

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
compiler="$1"
expected_sha256="${2:-}"
api="${3:-24}"
: "${ANDROID_NDK_HOME:?ANDROID_NDK_HOME must point to an Android NDK}"
command -v cmake >/dev/null || { echo "CMake required." >&2; exit 69; }

host_tag="linux-x86_64"
case "$(uname -s)-$(uname -m)" in
  Darwin-arm64) host_tag="darwin-x86_64" ;;
  Darwin-x86_64) host_tag="darwin-x86_64" ;;
  Linux-x86_64) host_tag="linux-x86_64" ;;
  *) echo "Unsupported NDK host for this proof" >&2; exit 69 ;;
esac
toolbin="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$host_tag/bin"
cc="$toolbin/aarch64-linux-android${api}-clang"
readelf_bin="$toolbin/llvm-readelf"
test -x "$cc" || { echo "Android ARM64 Clang not found: $cc" >&2; exit 69; }
test -x "$readelf_bin" || { echo "llvm-readelf not found in NDK" >&2; exit 69; }

work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT

actual_sha256="$(cmake -E sha256sum "$compiler")"
actual_sha256="${actual_sha256%% *}"
if [ -n "$expected_sha256" ]; then
  expected_sha256="$(printf '%s' "$expected_sha256" | tr '[:upper:]' '[:lower:]')"
  if [ "$actual_sha256" != "$expected_sha256" ]; then
    echo "Compiler SHA-256 changed before Android ARM64 build" >&2
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

"$compiler_exec" --tree-shake --tree-shake-root=mobile_guest_step   --target=host "$root/tests/fixtures/mobile_guest.ores" > "$work/guest.ll"
count="$(grep -Ec '^define[[:space:]].*i64[[:space:]]+@"?mobile_guest_step"?\(i64[^,)]*\)' "$work/guest.ll" || true)"
if [ "$count" -ne 1 ]; then
  echo "mobile guest symbol has missing/duplicate/wrong i64 ABI: mobile_guest_step" >&2
  exit 1
fi

ir_sha256="$(cmake -E sha256sum "$work/guest.ll")"
ir_sha256="${ir_sha256%% *}"
printf 'ORES_MOBILE_ANDROID_IR_SHA256=%s\n' "$ir_sha256"

"$cc" -x ir -fPIC -c "$work/guest.ll" -o "$work/guest-android-arm64.o"
"$cc" -std=c11 -Wall -Wextra -Wpedantic -Werror -fPIC -shared   "$root/tests/mobile_guest_jni.c" "$work/guest-android-arm64.o"   -o "$work/libores_mobile_guest_jni.so"
"$cc" -std=c11 -Wall -Wextra -Wpedantic -Werror   "$root/tests/mobile_guest_android_main.c" "$work/guest-android-arm64.o"   -o "$work/ores_mobile_guest_android"

"$readelf_bin" -h "$work/guest-android-arm64.o" | grep -Eq 'Machine:[[:space:]]+AArch64'
"$readelf_bin" -h "$work/libores_mobile_guest_jni.so" | grep -Eq 'Machine:[[:space:]]+AArch64'
"$readelf_bin" -h "$work/ores_mobile_guest_android" | grep -Eq 'Machine:[[:space:]]+AArch64'
"$readelf_bin" -h "$work/libores_mobile_guest_jni.so" | grep -Eq 'Type:[[:space:]]+DYN'
"$readelf_bin" -h "$work/ores_mobile_guest_android" | grep -Eq 'Type:[[:space:]]+DYN'

object_sha="$(cmake -E sha256sum "$work/guest-android-arm64.o")"; object_sha="${object_sha%% *}"
jni_sha="$(cmake -E sha256sum "$work/libores_mobile_guest_jni.so")"; jni_sha="${jni_sha%% *}"
exe_sha="$(cmake -E sha256sum "$work/ores_mobile_guest_android")"; exe_sha="${exe_sha%% *}"
printf 'ORES_MOBILE_ANDROID_OBJECT_SHA256=%s\n' "$object_sha"
printf 'ORES_MOBILE_ANDROID_JNI_SHA256=%s\n' "$jni_sha"
printf 'ORES_MOBILE_ANDROID_EXECUTABLE_SHA256=%s\n' "$exe_sha"
printf 'ORES_MOBILE_ANDROID_API=%s\n' "$api"

out_dir="${ORES_MOBILE_ANDROID_OUT_DIR:-}"
if [ -n "$out_dir" ]; then
  mkdir -p "$out_dir"
  cp "$work/guest.ll" "$out_dir/guest.ll"
  cp "$work/guest-android-arm64.o" "$out_dir/guest-android-arm64.o"
  cp "$work/libores_mobile_guest_jni.so" "$out_dir/libores_mobile_guest_jni.so"
  cp "$work/ores_mobile_guest_android" "$out_dir/ores_mobile_guest_android"
fi
