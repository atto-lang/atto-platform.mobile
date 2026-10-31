# Mobile draft security integration: source-local, no export

This temporary branch combines the **changed files**, not a blind branch merge, of these private-repository drafts:
- #13 `17c2e2a56e9d2b40ababfdc81d511be1df30ac36` (JNI, NDK, iOS supply-chain)
- #17 `2c4dcd3941e1c6802e4b72d1d6ee3c125dad4676` (CODEOWNERS, runner canary, metadata admission)
- #18 `292677d91d165a6fe302810e1f709732270b8c87` (iOS five-symbol ABI tests; **stacked on #13**)
- #19 `edb4ee5260467ababfe80c0868d77fad87053e26` (raw Git blob checkout verifier)

Base: latest reviewed `main` commit `9cbfdafe42e620ed2703f0ac7c7c27e5cd2556bf`, tree `0f5e238f4b3f38b8605041d9b22300d59816eab8`.

**Conflict decisions:** Native build/JNI/symbol scripts come from #18 (which includes #13). Source-only security scripts and dual-OS source-free canary come from #17. Raw checkout verifier/tests come from #19. Both overlapping YAML workflows retain #18's JNI/NDK/Xcode gates, #17's source-local metadata negative tests and #19's filter-free, exact-head raw checkout preflight. Legacy `git status` preflight is removed from both. The same pinned `actions/checkout` release and `persist-credentials: false` must be present in every build job.

`scripts/test_mobile_ci_integration.py` is a source-free composite workflow check, not a substitute for parsing YAML with a full Actions parser. The existing source-org CI has had `runner_id=0` / zero steps. Only an executed exact-integration-head Linux GCC/Clang/JNI/ASan+UBSan, Android NDK ABIs and actual macOS Xcode build can establish platform CI behavior. The synthetic iOS symbol test does not prove real signing, installation or guest native AOT.

**Security hold:** `.github/CODEOWNERS` is declarative and names `@ORESoftware` only. An independent reviewer and effective private-repository protection/ruleset enforcement remain separate required gates. No CI carrier, source export or permission allowance is provided. Do not merge this branch or supersede/close prior drafts until a distinct reviewer has reconciled changes and immutable commit evidence.
