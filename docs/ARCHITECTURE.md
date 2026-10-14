# Mobile integration architecture

## Contract

`oreslang-source.llvm` is the intended shared source-to-native compiler. This repository owns target selection, packaging, host-language bindings, app lifecycle, and capability policy. Its C library is a **host-only reference for ABI handling**, not the Oreslang VM or a compiler.

## Future end-to-end pipeline

1. Frontend and type/borrow verifier accept a supported Oreslang source subset.
2. Target-specific lowering emits native objects for Android AArch64 and iOS ARM64.
3. Link a static-AOT actor runtime and a **versioned C ABI** with no C++ exceptions crossing boundaries.
4. Use JNI on Android or Swift/Objective-C interop on iOS. Flutter can bind the C ABI via Dart FFI.
5. Package Android .so per ABI or signed iOS XCFramework; perform platform CI and device tests.

## ABI invariants

- Every buffer has explicit length, capacity and ownership; this reference API copies using `memmove`.
- Callbacks are synchronous and borrow their data only for the duration of the call.
- Caller supplies output allocations; message length is limited to 64 KiB.
- Status codes are explicit; no exceptions/panics cross ABI boundaries.
- The ABI version is a number, not a compatibility guarantee until real compiler/runtime conformance is delivered.
- Oreslang actor isolation is **not** provided by this reference component.

## Security/lifecycle

iOS profile is static AOT only; do not assume runtime JIT or hot-loaded executable code is App Store compliant. App lifecycle, OS permissions and background execution restrictions must be respected. All privileged host APIs require explicit capability grants. Signed binaries are built as part of platform-specific release tooling (not present here).
