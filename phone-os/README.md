# Oreslang Phone — Linux security-domain prototype

This directory begins an experimental **Linux ARM64 phone-OS** integration. It is separate from the existing Android JNI/iOS C ABI bridges.

The current component validates **trusted** app launch-policy records with exhaustive negative host cases. **It is not a sandbox and does not execute applications.**

Build locally from the repository root:

    cmake -S phone-os -B build-phone-policy -DCMAKE_C_COMPILER=clang -DBUILD_TESTING=ON
    cmake --build build-phone-policy
    ctest --test-dir build-phone-policy --output-on-failure

Read docs/SECURITY_MODEL.md before integrating with any privileged process launcher. Independent apps need kernel-enforced security domains; one app may contain many lightweight Oreslang actors/arenas. A policy pass is not a launched security domain.
