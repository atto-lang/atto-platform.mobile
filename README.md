# atto-platform.mobile

Mobile compilation and runtime integration for AttoLang applications.

## Naming migration and language authority

This repository is maintained at [atto-lang/atto-platform.mobile](https://github.com/atto-lang/atto-platform.mobile); use `https://github.com/atto-lang/atto-platform.mobile.git` for Git remotes and canonical `.atto` source examples. The normative language contract is [atto-spec](https://github.com/atto-lang/atto-spec), with executable guest evidence tracked by [atto-platform-conformance](https://github.com/atto-lang/atto-platform-conformance). Follow current ownership, borrowing, effects, actor, and syntax rules there; do not infer implemented features from a README or unexecuted CI.

## Scope

Mobile target build chains, host APIs, lifecycle integration, and device testing.

## Portability contract

The AttoLang language contract must remain separate from target-specific capabilities. Document supported toolchains, devices, platform APIs, memory limits, runtime constraints, and unsupported features explicitly.

## Verification

Where feasible, use real devices or representative emulators in addition to host-side tests. Record toolchain versions, target profiles, exact commit SHAs, and execution evidence. A successful build alone is not proof of runtime semantics.

## Development status

This is a statement of intended project responsibilities. Consult source files and CI for verified implementation or release status; do not assume all targets are supported.

## Related projects

- [AttoLang](https://github.com/atto-lang)
- [Target profiles](https://github.com/atto-lang/atto-target-profiles)
- [Platform conformance](https://github.com/atto-lang/atto-platform-conformance)

## Repository ownership and contribution routing

**Belongs here:** Mobile OS host integration, Android/iOS app lifecycle, sandbox permissions, packaging/ABI and real-device tests.

**Belongs elsewhere:** Cross-device UI components belong in atto-runtime-ui; compiler lowering in atto-source.*; portable semantics in atto-spec.

**Security and portability boundary:** Do not promise perpetual background servers on stock mobile OSes; constrain device capabilities, signing, native handles and suspension/background lifecycle.

AttoLang is the canonical project name; new source, examples and documentation use `.atto` and the `atto-lang` repository organization. This README defines scope, not feature readiness.

**Acceptance:** name exact source SHA/tree, runtime/toolchain, positive and negative checks and execution receipt before claiming support. Host-only tests and empty CI steps do not prove guest execution.

## Mobile runtime implementation seed

The platform-neutral value-only C UI admission contract is in
[`shared/include/atto_mobile_ui_v1.h`](shared/include/atto_mobile_ui_v1.h),
with implementation and negative tests under `shared/src` and `shared/tests`.
Run `make test` on a C11 host. This verifies **only** scalar preflight checks:
it does not execute an AttoLang guest, UI renderer, actor scheduler or mobile device.

The `ios/` tree separates runtime, Objective-C/C ABI, Swift transport,
UIKit, future SwiftUI, actor-to-main-thread admission, lifecycle,
permissions, packaging and device tests. `android/` documents the JNI/
Kotlin native path and proposed ART alternative. Do not infer production
readiness from directory creation.

AttoXP integration PRs: [iOS #16](https://github.com/atto-xp/atto-xp-ios/pull/16),
[Android #15](https://github.com/atto-xp/atto-xp-android/pull/15),
[conformance #22](https://github.com/atto-xp/atto-xp-conformance/pull/22),
[docs #10](https://github.com/atto-xp/atto-xp-docs/pull/10).
