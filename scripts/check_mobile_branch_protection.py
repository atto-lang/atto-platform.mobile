#!/usr/bin/env python3
"""Strict offline GitHub *legacy branch-protection* evidence check for mobile main.

For metadata obtained with authenticated GitHub API calls. Saved JSON alone is NOT
independently authenticated, is NOT source-export authorization, and does NOT
prove organization rulesets or independent approvals. No network or source reads.
"""
import argparse
import json
import sys
from pathlib import Path

SOURCE_NAME = "atto-lang/atto-platform.mobile"
SOURCE_ID = 1448719611
GITHUB_ACTIONS_APP_ID = 15368
REQUIRED_CHECKS = frozenset((
    "host (gcc)",
    "host (clang)",
    "sanitizers",
    "android-ndk-cross-build (arm64-v8a)",
    "android-ndk-cross-build (x86_64)",
    "ios-reference-xcframework",
))


def is_enabled(obj):
    return isinstance(obj, dict) and obj.get("enabled") is True


def is_disabled(obj):
    return isinstance(obj, dict) and obj.get("enabled") is False


def validate(repo, branch, protection, expected_main_sha):
    problems = []
    if not all(isinstance(v, dict) for v in (repo, branch, protection)):
        return ["repository, branch, and protection must be JSON objects"]
    if repo.get("id") != SOURCE_ID or type(repo.get("id")) is not int:
        problems.append("source numeric repository ID mismatch")
    if repo.get("full_name") != SOURCE_NAME:
        problems.append("source repository name mismatch")
    if repo.get("private") is not True or repo.get("visibility") != "private":
        problems.append("source must be private")
    if repo.get("archived") is not False or repo.get("disabled") is not False:
        problems.append("source must be active and not archived")
    if branch.get("name") != "main" or branch.get("protected") is not True:
        problems.append("main not GitHub-protected")
    commit = branch.get("commit")
    if not isinstance(commit, dict) or commit.get("sha") != expected_main_sha:
        problems.append("main commit does not match independently expected SHA")

    checks = protection.get("required_status_checks")
    if not isinstance(checks, dict) or checks.get("strict") is not True:
        problems.append("strict required status checks are not enforced")
    else:
        check_list = checks.get("checks")
        if not isinstance(check_list, list):
            problems.append("status checks app-bound list is absent")
        else:
            matched = set()
            for item in check_list:
                if not isinstance(item, dict):
                    problems.append("malformed status check")
                    continue
                context = item.get("context")
                if not isinstance(context, str):
                    problems.append("malformed status check context")
                    continue
                if context in REQUIRED_CHECKS:
                    if context in matched:
                        problems.append("duplicate required status check: " + context)
                    matched.add(context)
                    if type(item.get("app_id")) is not int or item.get("app_id") != GITHUB_ACTIONS_APP_ID:
                        problems.append("required check has untrusted app binding: " + context)
            for missing in sorted(REQUIRED_CHECKS - matched):
                problems.append("missing required GitHub Actions status check: " + missing)

    review = protection.get("required_pull_request_reviews")
    if not isinstance(review, dict):
        problems.append("pull request reviews not required")
    else:
        count = review.get("required_approving_review_count")
        if type(count) is not int or count < 1:
            problems.append("approving review count is insufficient")
        if review.get("require_code_owner_reviews") is not True:
            problems.append("CODEOWNER review is not required")
        if review.get("dismiss_stale_reviews") is not True:
            problems.append("stale approvals are not dismissed")
        bypass = review.get("bypass_pull_request_allowances")
        if not isinstance(bypass, dict):
            problems.append("PR-review bypass allowance metadata missing")
        else:
            for key in ("users", "teams", "apps"):
                if bypass.get(key) != []:
                    problems.append("PR-review bypass list not provably empty: " + key)
    if not is_enabled(protection.get("enforce_admins")):
        problems.append("branch protection is not enforced for administrators")
    if not is_disabled(protection.get("allow_force_pushes")):
        problems.append("force pushes are not explicitly disabled")
    if not is_disabled(protection.get("allow_deletions")):
        problems.append("branch deletions are not explicitly disabled")
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-json", type=Path, required=True)
    parser.add_argument("--branch-json", type=Path, required=True)
    parser.add_argument("--protection-json", type=Path, required=True)
    parser.add_argument("--expected-main-sha", required=True)
    args = parser.parse_args()
    sha = args.expected_main_sha
    if len(sha) != 40 or any(x not in "0123456789abcdef" for x in sha):
        parser.error("expected main SHA must be a lowercase 40-digit Git SHA")
    try:
        data = [json.loads(p.read_text(encoding="utf-8")) for p in
                (args.repo_json, args.branch_json, args.protection_json)]
    except (OSError, ValueError) as exc:
        print("PROTECTION_FAIL: cannot read authenticated API snapshot: " + type(exc).__name__, file=sys.stderr)
        return 1
    errors = validate(*data, sha)
    if errors:
        for problem in errors:
            print("PROTECTION_FAIL: " + problem, file=sys.stderr)
        return 1
    print("PROTECTION_METADATA_PASS: protected private main, required pinned CI, and review gates")
    print("NOT_A_TRANSFER_APPROVAL: independently verify rulesets, API authenticity and incident clearance")
    return 0


if __name__ == "__main__":
    sys.exit(main())
