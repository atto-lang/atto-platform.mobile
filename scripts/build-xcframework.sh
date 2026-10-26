#!/usr/bin/env bash
# Build distributable device + simulator XCFramework of the C REFERENCE ABI.
# No Oreslang compiler, actor runtime or Swift bridge is embedded.
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
for tool in xcrun xcodebuild lipo; do
  command -v "$tool" >/dev/null || { echo "REQUIRED_TOOL_MISSING: $tool" >&2; exit 2; }
done
test -s "$root/include/ores_mobile_bridge.h"
test -s "$root/ios/module.modulemap"
out="$root/build-ios"
test ! -L "$out" || { echo "refusing symlink build-ios directory" >&2; exit 2; }
mkdir -p "$out"
out="$(cd "$out" && pwd -P)"
[[ "$out" == "$root/"* ]] || { echo "build output escaped repository root" >&2; exit 2; }
stage="$(mktemp -d "$out/.xcframework-staging.XXXXXXXX")"
trap 'rm -rf "$stage"' EXIT
mkdir -p "$stage/headers" "$stage/device" "$stage/simulator-arm64" "$stage/simulator-x86_64" "$stage/simulator"
cp "$root/include/ores_mobile_bridge.h" "$stage/headers/"
cp "$root/ios/module.modulemap" "$stage/headers/"
build_one() {
  local sdk="$1" arch="$2" target="$3" output="$4"
  local clang sdkroot
  clang="$(xcrun --sdk "$sdk" --find clang)"
  sdkroot="$(xcrun --sdk "$sdk" --show-sdk-path)"
  "$clang" -target "$target" -isysroot "$sdkroot" -std=c11 -O2 \
    -Wall -Wextra -Wpedantic -Werror -fvisibility=hidden \
    -I"$root/include" -c "$root/src/ores_mobile_bridge.c" \
    -o "$output/ores_mobile_bridge.o"
  xcrun --sdk "$sdk" ar rcs "$output/libores_mobile_bridge.a" \
    "$output/ores_mobile_bridge.o"
  lipo "$output/libores_mobile_bridge.a" -verify_arch "$arch"
}
build_one iphoneos arm64 arm64-apple-ios15.0 "$stage/device"
build_one iphonesimulator arm64 arm64-apple-ios15.0-simulator "$stage/simulator-arm64"
build_one iphonesimulator x86_64 x86_64-apple-ios15.0-simulator "$stage/simulator-x86_64"
lipo -create "$stage/simulator-arm64/libores_mobile_bridge.a" \
  "$stage/simulator-x86_64/libores_mobile_bridge.a" \
  -output "$stage/simulator/libores_mobile_bridge.a"
lipo "$stage/simulator/libores_mobile_bridge.a" -verify_arch arm64 x86_64
xcodebuild -create-xcframework \
  -library "$stage/device/libores_mobile_bridge.a" -headers "$stage/headers" \
  -library "$stage/simulator/libores_mobile_bridge.a" -headers "$stage/headers" \
  -output "$stage/OresMobileBridge.xcframework"
test -s "$stage/OresMobileBridge.xcframework/Info.plist"
# Check expected public C entrypoints in both archives before publishing.
for archive in "$stage/device/libores_mobile_bridge.a" "$stage/simulator/libores_mobile_bridge.a"; do
  nm_output="$(xcrun nm -g "$archive")"
  grep -q '_ores_mobile_abi_version' <<<"$nm_output"
  grep -q '_ores_mobile_copy_message' <<<"$nm_output"
  grep -q '_ores_mobile_dispatch' <<<"$nm_output"
done
# Do not replace a previous successful bundle on build/check failure.
# The destination is generated output, never arbitrary user-provided source.
target="$out/OresMobileBridge.xcframework"
test ! -L "$target" || { echo "refusing symlink XCFramework destination" >&2; exit 2; }
if test -e "$target"; then
  mv "$target" "$stage/previous.xcframework"
fi
if ! mv "$stage/OresMobileBridge.xcframework" "$target"; then
  test ! -e "$stage/previous.xcframework" || mv "$stage/previous.xcframework" "$target"
  exit 1
fi
echo "Created $target (C reference ABI only; device arm64, simulator arm64/x86_64)"
