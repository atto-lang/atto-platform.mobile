# iOS native-AOT integration (experimental reference ABI)

On macOS with Xcode installed:

```sh
bash scripts/build-xcframework.sh
```

This builds a **real iOS XCFramework for the portable C reference bridge**:
device arm64 plus simulator arm64/x86_64. The bundle includes the versioned
`ores_mobile_bridge.h` C ABI header and Clang `module.modulemap`.
The exact-head macOS workflow uses Xcode to compile and package the archives,
validates their architectures and public C symbol presence, and uploads the
reference-only bundle as a CI artifact.

`scripts/build-ios.sh` remains available for a single device ARM64 archive.
Neither script compiles Oreslang sources, exports Oreslang actors, publishes
an App Store-ready SDK, or proves Android/iOS runtime parity. No Swift package,
Swift actor binding, code signing, real simulator invocation, device UI tests
or app-store artifact is included.

The iOS deployment profile disallows JIT and arbitrary hot executable-code
loading. Future target gates include Oreslang LLVM native AOT object emission,
actor runtime/ownership ABI, Swift integration and physical-device testing.
