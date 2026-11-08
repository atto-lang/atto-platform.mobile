# atto-platform.mobile

Mobile compilation and runtime integration for AttoLang applications.

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

AttoLang is the public name; preserve existing Oreslang/.ores compatibility and use canonical .atto for new portable library source. This README defines scope, not feature readiness.

**Acceptance:** name exact source SHA/tree, runtime/toolchain, positive and negative checks and execution receipt before claiming support. Host-only tests and empty CI steps do not prove guest execution.
