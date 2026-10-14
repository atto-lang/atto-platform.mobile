#!/usr/bin/env bash
set -euo pipefail

# Produce a small iOS ARM64 static library containing ONLY the C reference ABI.
# Needs macOS + Xcode tools. No Oreslang compilation or XCFramework generated.
cd "$(dirname "$0")/.."
mkdir -p build-ios
SDK="$(xcrun --sdk iphoneos --show-sdk-path)"
CLANG="$(xcrun --sdk iphoneos --find clang)"
"$CLANG" -target arm64-apple-ios15.0 -isysroot "$SDK" -std=c11 -O2 \
  -fvisibility=hidden -I include -c src/ores_mobile_bridge.c -o build-ios/ores_mobile_bridge.o
xcrun --sdk iphoneos ar rcs build-ios/libores_mobile_bridge.a build-ios/ores_mobile_bridge.o
echo "Created build-ios/libores_mobile_bridge.a (C reference ABI only)"
