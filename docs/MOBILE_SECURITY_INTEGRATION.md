# Mobile draft security integration: source-local, no export

This temporary branch combines the **changed files**, not a blind branch merge, of these private-repository drafts:
- #13 `17c2e2a56e9d2b40ababfdc81d511be1df30ac36` (JNI, NDK, iOS supply-chain)
- #17 `2c4dcd3941e1c6802e4b72d1d6ee3c125dad4676` (CODEOWNERS, runner canary, metadata admission)
- #18 `292677d91d165a6fe302810e1f709732270b8c87` (iOS five-symbol ABI tests; **stacked on #13**)
- #19 `edb4ee5260467ababfe80c0868d77fad87053e26` (raw Git blob checkout verifier)

Base: latest reviewed `main` commit `9cbfdafe42e620ed2703f0ac7c7c27e5cd2556bf`, tree `0f5e238f4b3f38b8605041d9b22300d59816eab8`.

**Conflict decisions:** Native build/JNI/symbol scripts come from #18 (which includes #13). Source-only security scripts and dual-OS source-free canary come from #17. Raw checkout verifier/tests come from #19. Both overlapping YAML workflows retain #18's JNI/NDK/Xcode gates, #17's source-local metadata negative tests and #19's filter-free, exact-head raw checkout preflight. Legacy `git status` preflight is removed from both. The same pinned `actions/checkout` release and `persist-credentials: false` must be present in every build job.

`scripts/test_mobile_ci_integration.py` is a source-free composite workflow check, not a substitute for parsing YAML with a full Actions parser. **Local execution:** five regression tests ran and passed in an isolated Python workspace, using the exact Git-blob-matched integration test source (`65d990cc5cd8492866875ee783a48f1e28064de7`), host workflow (`aba2da62fa78c4af5ced8a6ed9d4864fa43eb00b`), iOS workflow (`f881e92133ac68cf4f777253755fd04d3bdbc4ad`), Linux/macOS runner canary (`a516b349908a6cf5bc7114603b1e356915d29fb3`) and CODEOWNERS (`6a7f3ca6a7f546fbb989c377d49331be34823b52`). Five tests passed, zero failures. This verifies these *file-level* checks, not full integrated backend builds. The existing source-org CI has had `runner_id=0` / zero steps. Only an executed exact-integration-head Linux GCC/Clang/JNI/ASan+UBSan, Android NDK ABIs and actual macOS Xcode build can establish platform CI behavior. The synthetic iOS symbol test does not prove real signing, installation or guest native AOT.

**Security hold:** `.github/CODEOWNERS` is declarative and names `@ORESoftware` only. An independent reviewer and effective private-repository protection/ruleset enforcement remain separate required gates. No CI carrier, source export or permission allowance is provided. Do not merge this branch or supersede/close prior drafts until a distinct reviewer has reconciled changes and immutable commit evidence.

## Exact-head hosted CI admission

On initial integration commit `3b457e7a177fe0d9728db9a8e045091b3187eed1`, GitHub Actions runs had **zero executed steps**, with `runner_id=0` for every sampled job:
- [host GCC/Clang, sanitizer, Android NDK CI run 41235961524](https://github.com/ores-truffle-oreslang/oreslang-platform.mobile/actions/runs/41235961524)
- [iOS XCFramework run 41235961562](https://github.com/ores-truffle-oreslang/oreslang-platform.mobile/actions/runs/41235961562)
- [Linux/macOS source-free canary run 41235961529](https://github.com/ores-truffle-oreslang/oreslang-platform.mobile/actions/runs/41235961529)

These are **not executed**, not failures of code tests; source-org Actions admission, protected-main governance and independent security review are still unresolved. This is no permission to export private source or merge.
