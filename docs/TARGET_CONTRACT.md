# Target conformance gate (planned)

An Oreslang mobile target is not "supported" until all gates pass:

1. Build a nontrivial .ores fixture into a native library for the requested target triple.
2. Resolve actor mailbox and ownership runtime symbols with no interpreter fallback.
3. Verify C ABI calling conventions and exception-free boundary in a real Android/iOS process.
4. Test sendability, deep-copy/freeze behavior, cancellation, scheduling and app lifecycle.
5. Run target-device tests (not just Linux-host C unit tests).
6. Verify static AOT and package signing; benchmark peak memory, startup and battery.
7. Compile and test failures fail closed; do not silently enable host capabilities.
8. Publish exact compiler/runtime SHAs and target SDK versions.

`cmake --build build` currently verifies only `ores_mobile_bridge` on the **host** and cannot satisfy these gates. Target profiles in `profiles/` are design proposals until the compiler adopts a versioned manifest schema.
