#!/usr/bin/env bash
set -euo pipefail

# Produce a small iOS ARM64 static library containing ONLY the C reference ABI.
# Needs macOS + Xcode tools. No Oreslang compilation or XCFramework generated.
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
out="$root/build-ios"
test ! -L "$out" || { echo "refusing symlink build-ios directory" >&2; exit 2; }
mkdir -p "$out"
out="$(cd "$out" && pwd -P)"
[[ "$out" == "$root/"* ]] || { echo "build output escaped repository root" >&2; exit 2; }
SDK="$(xcrun --sdk iphoneos --show-sdk-path)"
CLANG="$(xcrun --sdk iphoneos --find clang)"
"$CLANG" -target arm64-apple-ios15.0 -isysroot "$SDK" -std=c11 -O2 \
  -Wall -Wextra -Wpedantic -Werror -fvisibility=hidden \
  -I "$root/include" -c "$root/src/ores_mobile_bridge.c" -o "$out/ores_mobile_bridge.o"
xcrun --sdk iphoneos ar rcs "$out/libores_mobile_bridge.a" "$out/ores_mobile_bridge.o"
echo "Created $out/libores_mobile_bridge.a (C reference ABI only)"
