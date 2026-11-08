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


## Repository ownership and migration (2026-11-08)

**Current home:** `ores-truffle-oreslang/oreslang-platform.mobile` (legacy Oreslang); public language name **AttoLang**. Retain existing `.ores` files and `oreslang-*` dependencies/CLI compatibility; don't infer backend or device capabilities from branding.

- **Owns:** iOS/Android native target/host bindings, lifecycle constraints and secure device capability tests.
- **Excludes:** unrestricted 24/7 mobile daemon guarantees or commercial hosting control plane.
- **Organizations:** [atto-lang](https://github.com/atto-lang) owns compiler/runtime target adapters, language portability, stdlib and conformance. [atto-co](https://github.com/atto-co) owns hosted control planes, deployment, tenancy, billing and managed infrastructure. A similarly named destination repo is not an executed transfer or equivalent runtime.
- **Proposed initial migration:** approved *ownership transfer* of this repository into `atto-lang/oreslang-platform.mobile` **without renaming**. Preserve the numeric GitHub ID and Git tree. No direct `atto-co` migration is authorized in the current wave; later commercial carve-outs require a separate review.

**Per-repo safety gates:** claim in [coordination issue #26](https://github.com/ores-truffle-oreslang/.github/issues/26), follow private [migration PR #25's runbook](https://github.com/ores-truffle-oreslang/.github/pull/25); record exact head SHA/tree, numeric repository ID, independent security + both admin approvals, target branch protection/rulesets and access, permissions for Actions/secrets/apps/deploy keys, PRs, releases, packages, Pages and dependencies; approve rollback. Never mirror private code without authorization. After a real GitHub transfer, recheck unchanged ID/tree/history, re-establish permissions, update consumer references and rerun exact-head executable tests. `steps=[]` or `null` is `NOT_EXECUTED`; host-only checks do not demonstrate language guest parity, zero-GC, AOT or safe production use.
