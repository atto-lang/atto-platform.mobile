# OresMobile

Mobile platform integration for **Oreslang**, targeting Android and iOS through the native-AOT toolchain.

> **Status: bootstrap.** This repository currently provides a small, portable C ABI boundary, host tests, and platform target specifications. It does **not** yet compile Oreslang apps, ship an actor runtime, provide JNI/Swift packages, or claim iOS/Android target parity.

## Scope

- Android `arm64-v8a`: eventually build an AOT native library for apps and expose a versioned C ABI.
- iOS `arm64`: eventually build an AOT static library/XCFramework for Swift or Objective-C.
- App integrations (Kotlin, Swift, Flutter) must use explicit ownership and message copies across the ABI.
- App Store/iOS profile is **static AOT only**; do not enable runtime JIT or network-fetched executable code.

## Quick start (host ABI conformance)

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The current build compiles and tests **only the host bridge C reference**, not the Oreslang compiler. See `docs/ARCHITECTURE.md`, `docs/TARGET_CONTRACT.md`, and `profiles/`.

## Upstream integration

Compiler: [oreslang-source.llvm](https://github.com/ores-truffle-oreslang/oreslang-source.llvm). The LLVM source subset currently lacks general actor lowering and runtime parity; do not treat this repository as proof otherwise.

## iOS XCFramework reference target

On macOS with Xcode, `bash scripts/build-xcframework.sh` packages a portable
C reference bridge for device ARM64 and simulator ARM64/x86_64, including the
C header and module map. Source-local macOS CI is required to execute this
build. This is **not** an Oreslang compiler/native actor runtime, not a Swift
SDK, and not yet tested on an iOS device. See `ios/README.md`.
