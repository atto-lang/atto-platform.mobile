# iOS native-AOT integration (bootstrap)

On a macOS host with Xcode installed, `bash scripts/build-ios.sh` compiles the portable reference C bridge to an iOS ARM64 static library. It does **not** compile Oreslang source code.

Future target requirements: ARM64 object emission from the Oreslang LLVM compiler, linked native actor runtime, a module map and Swift package, device/simulator archives, an XCFramework, codesigning and on-device lifecycle tests.

The iOS deployment profile disallows JIT and arbitrary hot executable-code loading. Data and configuration updates remain separate from application code distribution.
