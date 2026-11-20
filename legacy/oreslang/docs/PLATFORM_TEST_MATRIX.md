# Verification matrix

| Target | Bootstrap artifact | Available automated evidence | Missing |
| --- | --- | --- | --- |
| Linux x86_64 | static C bridge | CMake + GCC/Clang CTest | Oreslang compiler parity |
| Android ARM64 | JNI shared library | Source and NDK build instructions | NDK CI, instrumentation, Oreslang runtime |
| iOS ARM64 | static C library | macOS build script (manual) | macOS CI, signed package, Swift binding, device tests |

`OresMobileNative.copyMessage` copies a byte array synchronously and has a 64 KiB limit; it is **not** an actor message send API. Treat mobile target profiles as draft target contracts until an Oreslang backend supports them.
