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

## Copy-isolated dispatch

`ores_mobile_dispatch` borrows the caller's original input. Use
`ores_mobile_dispatch_copy` when a synchronous callback needs a separate message
snapshot: supply an independent scratch buffer with at least `length` bytes.
The bridge copies before invoking the handler, then scrubs those bytes with
non-elidable stores on both success and callback failure. It allocates no memory.
Input and scratch may be adjacent but must not overlap. Rejected calls leave
scratch untouched. The callback may not retain its borrowed pointer; serialize
access and keep scratch alive until dispatch returns. This adds a host ownership
boundary, not a queued actor scheduler or device-level memory protection.

## JNI temporary-memory hygiene

The Android JNI `copyMessage` bridge copies a checked (at most 64 KiB)
Java array into temporary native-owned bytes. Before `free`, it now calls
`ores_mobile_clear_message` on **every** post-allocation exit path, including
JNI exceptions and native ABI errors. The bounded wipe uses volatile character
stores. Host CTest checks zeroization and rejected-argument no-write behavior;
the Java JNI smoke checks output equality and post-error recovery.

This limits residual bytes in the temporary native allocation. It does **not**
erase the original/result Java arrays, copy-on-write snapshots, allocator
metadata, crash dumps, device memory or other copies; it is not an iOS/Android
actor isolation or secure-erasure certification.

## Upstream integration

Compiler: [oreslang-source.llvm](https://github.com/ores-truffle-oreslang/oreslang-source.llvm). The LLVM source subset currently lacks general actor lowering and runtime parity; do not treat this repository as proof otherwise.

## iOS XCFramework reference target

On macOS with Xcode, `bash scripts/build-xcframework.sh` packages a portable
C reference bridge for device ARM64 and simulator ARM64/x86_64, including the
C header and module map. Source-local macOS CI is required to execute this
build. This is **not** an Oreslang compiler/native actor runtime, not a Swift
SDK, and not yet tested on an iOS device. See `ios/README.md`.

