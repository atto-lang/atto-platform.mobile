"""Adversarial real-Git subprocess coverage for filter-free Actions preflight."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "scripts/verify_raw_checkout.py"


def git(root, *args):
    return subprocess.check_output(
        ["git", "-C", str(root), *args], stderr=subprocess.DEVNULL
    ).decode().strip()


class RawCheckoutPreflightTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        git(self.root, "init", "-q")
        git(self.root, "config", "user.email", "test@example.invalid")
        git(self.root, "config", "user.name", "Test")
        (self.root / "tracked.txt").write_text("trusted\n")
        git(self.root, "add", ".")
        git(self.root, "commit", "-qm", "base")
        self.expected = git(self.root, "rev-parse", "HEAD")

    def run_checker(self, expected=None):
        env = dict(os.environ, EXPECTED_SHA=expected or self.expected)
        return subprocess.run(
            [sys.executable, str(SCRIPT)], cwd=self.root, env=env,
            text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            timeout=5,
        )

    def test_clean_checkout(self):
        self.assertEqual(self.run_checker().returncode, 0)

    def test_wrong_sha(self):
        self.assertNotEqual(self.run_checker("0" * 40).returncode, 0)

    def test_modified_bytes(self):
        (self.root / "tracked.txt").write_text("changed\n")
        self.assertNotEqual(self.run_checker().returncode, 0)

    def test_untracked(self):
        (self.root / "extra").write_text("x")
        self.assertNotEqual(self.run_checker().returncode, 0)

    def test_staged_change(self):
        (self.root / "tracked.txt").write_text("changed\n")
        git(self.root, "add", "tracked.txt")
        self.assertNotEqual(self.run_checker().returncode, 0)

    def test_symlink_substitution(self):
        (self.root / "tracked.txt").unlink()
        (self.root / "tracked.txt").symlink_to("/etc/hosts")
        self.assertNotEqual(self.run_checker().returncode, 0)

    def test_fifo_substitution(self):
        (self.root / "tracked.txt").unlink()
        os.mkfifo(self.root / "tracked.txt")
        self.assertNotEqual(self.run_checker().returncode, 0)

    def test_assume_unchanged(self):
        git(self.root, "update-index", "--assume-unchanged", "tracked.txt")
        self.assertNotEqual(self.run_checker().returncode, 0)

    def test_skip_worktree(self):
        git(self.root, "update-index", "--skip-worktree", "tracked.txt")
        self.assertNotEqual(self.run_checker().returncode, 0)

    def test_clean_filter_is_not_executed(self):
        marker = self.root / "filter-executed"
        (self.root / ".gitattributes").write_text("tracked.txt filter=malicious\n")
        git(self.root, "add", ".gitattributes")
        git(self.root, "commit", "-qm", "add attributes")
        self.expected = git(self.root, "rev-parse", "HEAD")
        git(self.root, "config", "filter.malicious.clean", f"touch {marker}; cat")
        self.assertEqual(self.run_checker().returncode, 0)
        self.assertFalse(marker.exists(), "preflight executed a clean filter")

    def test_untracked_ignored(self):
        (self.root / ".gitignore").write_text("ignored.bin\n")
        git(self.root, "add", ".gitignore")
        git(self.root, "commit", "-qm", "ignore")
        self.expected = git(self.root, "rev-parse", "HEAD")
        (self.root / "ignored.bin").write_text("xx")
        self.assertNotEqual(self.run_checker().returncode, 0)


if __name__ == "__main__":
    unittest.main()
