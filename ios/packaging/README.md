# Signed iOS packaging gate

Ship LLVM-compiled AOT code and the native runtime inside a signed bundle. Record exact compiler SHA, source SHA, toolchain SDK, artifact digest, platform signing identity and emulator/device execution. No release claim until signing and physical-device tests are executed. Do not package downloaded code that changes application features.
