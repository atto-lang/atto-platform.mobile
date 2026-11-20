# AttoXP mobile M0: host reference vs proposed native ABI

This patch adds a **native C11 host-reference transaction model**, not a
native iOS/Android UI SDK or an implementation of the proposed public
`AttoXpUiHandleV1`/async batch ABI.

## Currently executable

- `include/atto_xp_ui.h`, `src/atto_xp_ui.c`: fixed-size 32-node,
  64-command, scalar counter transactions. Copy-validate-commit is all-or-nothing
  at the model level, with exact revisions, wrong-owner/scene/app rejection,
  terminal close, and stale generation fencing.
- `atto_xp_ui_open` reserves a **process-wide strictly increasing 24-bit
  scene epoch** via C11 atomic CAS. Epoch reuse even after destruction is
  rejected to prevent identical handles in two scenes. Exhaustion fails closed.
  Host code must never deserialize handles from an earlier process invocation.
- Thread check is a **trusted callback injection point**, not proof of OS
  main-thread identity. Actual Swift/Kotlin adapter must use OS thread APIs,
  not a caller-provided Boolean, and serialize access to each state.
- `tests/test_atto_xp_ui.c`: bounded/invalid batch, order, owner, revision,
  stale-handle and cross-scene epoch negative regressions.
- `tests/test_atto_xp_ui_property.c`: seeded 5000-round repeated
  transactional model with valid/invalid batches, ABA churn and no-write
  invariants.

## NOT interoperable with the proposed ABI v1

The ratification draft in `oreslang-platform-conformance` has **24-byte
unpredictable identity + 64-bit generation handles**, `AttoXpUiBatchV1`,
`attoxp_ui_submit_batch_v1`, an async queue, 256 operation maximum,
65,536 encoded-byte maximum, and strict actor/OS capability gates.

This reference model deliberately uses **64-bit local handles** and
synchronous in-memory state transitions. Its `ATTO_XP_UI_ABI_VERSION=1`
denotes the model's own C interface; it is **not** ABI-link evidence for the
ratified/target AttoXP wire ABI. Do not link or translate these types by
reinterpret_cast or assume struct binary compatibility.

## Next required M1 work

1. Implement `attoxp_ui_query_abi_v1`, `submit_batch_v1` and cancel in a
   separate production library with 24-byte identities, native handle registry,
   secure random issuance, owner/kind checks and bounded serialized operations.
2. Add complete input size/version/reserved-field checks and negative cases
   H01-H08/U01-U11/C01-C04/F01-F03 from the central conformance matrix.
3. Implement ownership-safe UIKit/Swift and Android View/JNI adapters,
   verifying the true main thread and lifecycle before and after dispatch.
4. Connect a real LLVM-compiled `.atto` guest to ABI, run iOS Simulator and
   Android Emulator, then device tests. .NET/Wasm shared scalar fixtures
   are not native mobile UI or actor parity.

## Evidence

Linux GCC C11 reference tests, ASan and UBSan executed locally. Exact-head
GitHub Actions at the source organization currently report completed failures
with **no recorded steps**; classify `INFRA_FAILED/NOT_EXECUTED`.
Never promote model, authored test files, or zero-step CI as guest/device proof.
