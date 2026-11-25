#!/usr/bin/env python3
"""Source-free composite CI admission regression (does not execute platform builds)."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


def job_blocks(workflow):
    """Split top-level workflow job entries without third-party YAML parsers."""
    text = (ROOT / workflow).read_text(encoding="utf-8")
    match = re.search(r"(?m)^jobs:\s*$", text)
    if not match:
        raise AssertionError("workflow has no jobs")
    jobs = {}
    name = None
    for line in text[match.end():].splitlines(keepends=True):
        found = re.match(r"^  ([A-Za-z][A-Za-z0-9_-]*):\s*$", line)
        if found:
            name = found.group(1)
            if name in jobs:
                raise AssertionError("duplicate CI job")
            jobs[name] = []
        elif name:
            jobs[name].append(line)
    return text, {key: "".join(lines) for key, lines in jobs.items()}


class MobileCiIntegration(unittest.TestCase):
    def test_every_critical_job_admits_exact_raw_checkout(self):
        for file, names in (
            (".github/workflows/ci.yml", ("host", "sanitizers", "android-ndk-cross-build")),
            (".github/workflows/ios-reference-xcframework.yml", ("ios-reference-xcframework",)),
        ):
            with self.subTest(file=file):
                text, jobs = job_blocks(file)
                self.assertEqual(set(jobs), set(names))
                self.assertNotIn("git status", text)
                for name, body in jobs.items():
                    with self.subTest(job=name):
                        self.assertIn("actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683", body)
                        self.assertIn("persist-credentials: false", body)
                        self.assertIn("github.event.pull_request.head.sha || github.sha", body)
                        self.assertIn("python3 scripts/verify_raw_checkout.py", body)
                        self.assertIn("test_verify_raw_checkout.py", body)
                        self.assertIn('test "$(git rev-parse --verify HEAD)" = "$EXPECTED_SHA"', body)

    def test_all_three_host_security_gates_present(self):
        text, jobs = job_blocks(".github/workflows/ci.yml")
        host = jobs["host"]
        for command in (
            "python3 scripts/test_mobile_runner_admission.py",
            "python3 scripts/test_mobile_branch_protection.py",
            "python3 scripts/test_mobile_ci_integration.py",
            "bash scripts/test-ios-reference-symbols.sh",
            "bash scripts/test-jni-host.sh",
        ):
            self.assertIn(command, host)
        self.assertIn("matrix:\n", host)
        self.assertIn("cc: [gcc, clang]", host)
        self.assertIn("ASAN_OPTIONS:", jobs["sanitizers"])
        self.assertIn("UBSAN_OPTIONS:", jobs["sanitizers"])
        self.assertIn("matrix:", jobs["android-ndk-cross-build"])
        self.assertIn("arm64-v8a, x86_64", jobs["android-ndk-cross-build"])

    def test_ios_preserves_real_xcode_and_closed_symbol_check(self):
        text, jobs = job_blocks(".github/workflows/ios-reference-xcframework.yml")
        job = jobs["ios-reference-xcframework"]
        self.assertIn("macos-15", job)
        self.assertIn("bash scripts/test-ios-reference-symbols.sh", job)
        self.assertIn("xcodebuild -version", job)
        self.assertIn("bash scripts/build-xcframework.sh", job)
        self.assertIn("OresMobileBridge.xcframework", job)
        self.assertIn("actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02", job)

    def test_host_policy_does_not_persist_checkout_credentials(self):
        text, jobs = job_blocks(".github/workflows/mobile-host-policy.yml")
        self.assertEqual(set(jobs), {"c11-host"})
        job = jobs["c11-host"]
        self.assertIn("actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683", job)
        self.assertIn("persist-credentials: false", job)
        self.assertIn("github.event.pull_request.head.sha || github.sha", job)

    def test_runner_canary_source_free_and_two_os_admission(self):
        text, jobs = job_blocks(".github/workflows/source-local-runner-allocation-canary.yml")
        self.assertEqual(set(jobs), {"allocation-only", "allocation-macos-only"})
        self.assertIn("permissions: {}", text)
        for name, job in jobs.items():
            with self.subTest(name=name):
                self.assertNotIn("actions/checkout", job)
                self.assertNotIn("curl ", job)
                self.assertNotIn("git clone", job)
                self.assertIn("SOURCE_CHECKOUT=false", job)
                self.assertIn("SOURCE_EXPORT=false", job)
                self.assertIn("TESTS_EXECUTED=false", job)

    def test_owner_declaration_is_not_self_approval(self):
        owners = (ROOT / ".github/CODEOWNERS").read_text(encoding="utf-8")
        self.assertIn("* @ORESoftware", owners)
        self.assertIn("declarative only", owners)
        self.assertIn("genuinely", owners)


if __name__ == "__main__":
    unittest.main()
