# Rust compiler fixtures and native Oreslang core suite

This repository adds **two distinct, non-interchangeable test layers**:

1. `tools/compiler-fixtures` is a dependency-free Rust harness. Its 2 default tests validate the fixture inventory and fail-closed compiler-driver admission. Three **feature-gated** tests invoke the **real configured Oreslang compiler**: positive scalar arithmetic, undefined identifier rejection and type mismatch rejection. A passing host-only test is not compiler evidence.
2. `tests/test_platform_scalar_harness.ores` contains **3 native Oreslang** `TestCase` registrations using the compiler-owned `ores.std/testing` TestSuite, `run_suite` and `require_success`. These are scalar reference models for mobile; they **do not** call native mobile implementation APIs and must not be credited as platform-runtime, device, synthesis or ABI verification.

## Host-only self-tests (no compiler)

```sh
cargo test --manifest-path tools/compiler-fixtures/Cargo.toml
```

## Real compiler admission (not run by default)

Provide a **reviewed, absolute-path executable** in `ORESLANG_COMPILER_DRIVER`. The adapter receives exactly one absolute path to an Oreslang fixture and must invoke an actual compiler, returning its exit code. Keep the command adapter out of untrusted shell expansion; the Rust caller uses direct process execution. A negative result can still come from a broken adapter, so independently inspect real diagnostics, build provenance and captured CI steps.

```sh
ORESLANG_COMPILER_DRIVER=/absolute/path/to/reviewed-compiler-adapter \\
  cargo test --manifest-path tools/compiler-fixtures/Cargo.toml --features real-compiler
```

The configured driver has a 30-second **direct-child** timeout and no stdout/stderr capture. This is not a process-group kill, sandbox, diagnostic attestation, or runtime execution proof. A strict positive compile and two diagnostic-rejection exits are the only asserted semantics. The root fixture files are limited to 16 KiB each and symlinks are rejected at inspection time; hostile concurrent filesystem modification remains outside this guarantee.

## Native guest suite (separate evidence gate)

Compile and execute `tests/test_platform_scalar_harness.ores` with a trusted backend that packages the same `ores.std/testing` module contract as the canonical Java standard library. Require **three unique PASS events** under `ORES_TEST|PASS|mobile-scalar-reference/...`, an exact `ORES_TEST|SUMMARY|total=3|passed=3|failed=0|skipped=0|filtered=0` receipt, process exit code zero, input/compiler digests, and actual executed GitHub job steps. This repo does not currently contain a trusted backend-specific native guest launcher. The authored source alone **does not** demonstrate compiler-backed execution or semantic parity.

## Security / promotion

No cross-organization private-source mirroring, release/promotion, or merging on this change. A zero-step Actions job is runner-admission **not executed**, never a Rust or native guest test result. Require independently reviewed source-org exact-head compiler execution, stdlib import identity, diagnostics and native stdout receipts before promotion.
