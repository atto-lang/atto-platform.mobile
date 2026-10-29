# Mobile CI admission and private-carrier authorization: administrator runbook

This document is review material. It **does not authorize private source export** and is not evidence that any requirement has been met. The source organization, security reviewer, and incident owner must each supply independently authenticated evidence.

## Source snapshot (re-pin after any changes)

- Source repository: `ores-truffle-oreslang/oreslang-platform.mobile`; numeric GitHub repository ID **1419865409**; private.
- Reference mobile ABI PR: [#18](https://github.com/ores-truffle-oreslang/oreslang-platform.mobile/pull/18).
- PR #18 head: `0c682853b178c388bb13d6f1c08800f25597c430`.
- PR #18 root Git tree: `6b3d6626cfff825842dd033be6513a9103c7c1af`.
- Latest main checked for this runbook: `9cbfdafe42e620ed2703f0ac7c7c27e5cd2556bf`, `protected=false`.
- Source-free alternate carrier candidate: private `StreemPilot/streempilot-docs`, numeric ID **1351003552**. **Candidate only; not approved.**

## 1. Restore source-org Actions admission (admin)

The following exact-head PR #18 workflow jobs were retried but failed before runner assignment:
- iOS [run 40812719884, attempt 2](https://github.com/ores-truffle-oreslang/oreslang-platform.mobile/actions/runs/40812719884): `runner_id=0`, `steps=[]`.
- Host + Android [run 40812719702, attempt 2](https://github.com/ores-truffle-oreslang/oreslang-platform.mobile/actions/runs/40812719702): all GCC, Clang, sanitizers, arm64-v8a and x86_64 jobs `runner_id=0`, `steps=[]`.
- Source-free Linux/macOS canary [run 40816051513](https://github.com/ores-truffle-oreslang/oreslang-platform.mobile/actions/runs/40816051513): both failed before allocation.

Admin must review **organization** Actions billing/spend caps, hosted/private runner entitlement, Actions allowed policies and repository execution permissions, runner groups and GitHub's job diagnostic banners. The observed status does *not* establish a specific root cause. If settings and billing allow execution yet jobs still fail before admission, supply the exact run/job IDs to GitHub Support.

After correcting the issue, rerun the **source-free** Linux/macOS canary from PR #17. Require `runner_id > 0`, `steps` with a completed proof step, success and correct exact source head and run attempt; no checkout or external source transfer. The read-only `scripts/check-mobile-runner-admission.py` verifies saved job metadata (with independently authenticated API responses). A positive canary proves allocation only.

## 2. Enforce protected `main` (admin; no export before independent verification)

### Check subscription eligibility before attempting protection

As of 2026-10-29, authenticated `GET /repos/ores-truffle-oreslang/oreslang-platform.mobile/rulesets?includes_parents=true` returned **403** with GitHub's message `Upgrade to GitHub Pro or make this repository public to enable this feature.` The repository owner is an **organization**, so the administrator should check **GitHub Team or Enterprise plan eligibility** for private repository protections; GitHub Pro applies to personal accounts and the error text is generic. This does not independently prove the current plan or the reason for source-org Actions admission failures. GitHub documents private-repository branch protection and rulesets as paid-plan features. **Do not make the repository public** to bypass a plan gate. If the organization lacks eligible features, correct subscription/entitlement before attempting protection, and preserve the private-source export hold.

Independently check current plan in the GitHub organization Billing and licensing UI (or with a properly authorized administrative API), then separately check Actions budget and hosted runner settings. The connector's `GET /branches/main/protection` returns **403 Resource not accessible by integration**, which by itself does not distinguish integration permissions from plan eligibility. The two 403s mean different things and must not be conflated.

Using GitHub UI / a privileged authenticated `gh` CLI, configure:
- Require PRs, at least one approving review and CODEOWNER review; stale approval dismissal.
- Enforce administrator rules; zero reviewed user/team/app PR-review bypasses; disable force push and branch deletion.
- Require strict status checks from the trusted GitHub Actions application, and verify matching check names and app IDs.
- Independently inspect repository and inherited organization rulesets, including enforcement and bypass lists. A CODEOWNERS file without enforcement is not a security gate.

Read-only *metadata* collection in an authorized admin environment (never paste tokens or secrets into issue comments):

```bash
repo='ores-truffle-oreslang/oreslang-platform.mobile'
gh api "repos/$repo" > repo.json
gh api "repos/$repo/branches/main" > branch.json
gh api "repos/$repo/branches/main/protection" > protection.json
gh api "repos/$repo/rulesets?includes_parents=true" > rulesets.json
python3 scripts/check_mobile_branch_protection.py \
  --repo-json repo.json --branch-json branch.json \
  --protection-json protection.json \
  --expected-main-sha '9cbfdafe42e620ed2703f0ac7c7c27e5cd2556bf'
```

The `expected-main-sha` is a reviewed pin; **update it when main changes**. This Python verifier covers *legacy branch protection metadata only*; independently review inherited rulesets and the API response provenance. Connector API access currently yields HTTP 403 for branch-protection details. A missing metadata field **fails closed**.

**Avoid deadlock when enabling checks:** PR #17 currently branches from `main`, before the Android NDK jobs added through the PR #13/#18 stack. Initially enforce only exact checks that can run on the commit being protected; after the foundation is accepted, include all six mobile CI names: `host (gcc)`, `host (clang)`, `sanitizers`, `android-ndk-cross-build (arm64-v8a)`, `android-ndk-cross-build (x86_64)`, and `ios-reference-xcframework`. Do not require the path-filtered allocation-only canary on unrelated PRs: its job may be absent. **No private-source export until full checked and enforced policy is proven.**

## 3. Independent incident/security clearance (not automatable by code author)

Required blockers:
- [source publisher authorization #962](https://github.com/ores-truffle-oreslang/oreslang-source.java/issues/962)
- [platform protection #170](https://github.com/ores-truffle-oreslang/oreslang-platform-conformance/issues/170)
- [historical public exposure #22](https://github.com/ores-truffle-oreslang/oreslang-platform-conformance/issues/22)
- [other-carrier investigation #77](https://github.com/ores-truffle-oreslang/oreslang-platform-conformance/issues/77)

An independent authorized reviewer must document incident-owner disposition and a reviewed allowlist binding immutable **source repository ID 1419865409**, exact source SHA/tree above, destination full name and numeric **destination repository ID 1351003552**, verified private visibility/permissions, reviewed pinned carrier workflow/commit/test plan, source-main enforced policy, transfer retention and purge handling. Revalidate private destination identity *before each write*. An old successful source-free carrier run is not an authorization.

Do **not** transfer source via connector Git blobs/trees, fork, checkout, Actions artifact or another channel until approval is recorded. No token in the workflow is necessary or sufficient to make unauthorized transfer safe.

## 4. Acceptance once a permitted runner is available

Preferred: source-org exact-head CI after runner recovery. Alternative: independently approved, exact-tree, private carrier only after section 3.

The actual PR #18 exact head requires:
- Apple macOS: build real device arm64, simulator arm64 and simulator x86_64 reference archives and XCFramework; prove all five C ABI exports on **each thin slice** and inspect the saved artifact.
- Linux: GCC, Clang CTests; real JNI -> Java tests; Clang ASan/UBSan.
- Android: NDK arm64-v8a + x86_64 shared JNI library builds; distinguish cross-compilation from emulator/device execution.
- Verify source commit AND root tree, exact workflow/runner job steps and the result artifacts. Do not treat historical, local, source-free or synthetic passes as exact-head certification.

## 5. Merge governance

Keep #17 and #13 as draft until the protection and hosted CI gates pass. Verify PR #17 independently (governance), review #13 (foundation), then rebase/test the stacked #18 (ABI). Never record passing manual statuses for unexecuted checks. Review requests from an operator-controlled CODEOWNER are not independent approval for transferring private source.

## Observed source-free capacity, not product certification

- [StreemPilot ARM64 Linux run #40159064386](https://github.com/StreemPilot/streempilot-docs/actions/runs/40159064386), attempt 2: runner assigned, successful.
- [StreemPilot Xcode 16.4 synthetic XCFramework #40815217332](https://github.com/StreemPilot/streempilot-docs/actions/runs/40815217332): successful.
- [StreemPilot synthetic five-export / three-slice matrix #40817333512](https://github.com/StreemPilot/streempilot-docs/actions/runs/40817333512): real Mach-O, 15 positive symbol checks, 15 deliberately omitted exports rejected, XCFramework assembled, successful.

**No private Oreslang source was included in those source-free probes.**
