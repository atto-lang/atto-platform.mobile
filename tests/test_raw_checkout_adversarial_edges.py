"""Real-Git adversarial cases for the pinned, no-filter source checkout verifier.

This is a host-side security regression suite, not compiler or target execution.
Run on Linux: python3 -m unittest -v tests/test_raw_checkout_adversarial_edges.py
"""
import os
import socket
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


@unittest.skipUnless(sys.platform.startswith("linux"), "Linux source checkout gate")
class RawCheckoutAdversarialEdges(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="raw-checkout-edges-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        git(self.root, "init", "-q")
        git(self.root, "config", "user.email", "test@example.invalid")
        git(self.root, "config", "user.name", "Boundary Test")
        (self.root / "tracked.txt").write_text("safe\n")
        self.commit("baseline")

    def commit(self, message):
        git(self.root, "add", "-A")
        git(self.root, "commit", "-qm", message)
        self.head = git(self.root, "rev-parse", "HEAD")

    def check(self, expected=None, extra_env=None):
        env = {k: v for k, v in os.environ.items() if not k.startswith("GIT_")}
        env["EXPECTED_SHA"] = expected if expected is not None else self.head
        env.update(extra_env or {})
        return subprocess.run(
            [sys.executable, str(SCRIPT)], cwd=self.root, env=env,
            capture_output=True, text=True, timeout=15,
        )

    def deny(self):
        result = self.check()
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn("RAW_CHECKOUT_DENIED:", result.stderr)

    def test_clean_checkout_is_admitted(self):
        result = self.check()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("RAW_CHECKOUT_EXACT_HEAD_PASS", result.stdout)

    def test_clean_filter_does_not_execute_even_when_tracked_bytes_change(self):
        (self.root / ".gitattributes").write_text("tracked.txt filter=hostile\n")
        self.commit("track attrs before configuring filter")
        marker = self.root / "filter-triggered"
        git(self.root, "config", "filter.hostile.clean", "sh -c 'touch filter-triggered; cat'")
        (self.root / "tracked.txt").write_text("evil\n")  # same length
        self.deny()
        self.assertFalse(marker.exists(), "clean filter executed during checkout verification")

    def test_untracked_filename_case_alias_with_untracked_cache_enabled(self):
        git(self.root, "config", "core.ignorecase", "true")
        git(self.root, "config", "core.untrackedCache", "true")
        (self.root / "TRACKED.txt").write_text("injected\n")
        self.assertEqual(git(self.root, "ls-files", "--others"), "")
        self.assertIn("TRACKED.txt", git(self.root, "-c", "core.ignorecase=false", "ls-files", "--others"))
        self.deny()

    def test_untracked_parent_case_alias_with_cache_enabled(self):
        (self.root / "pkg").mkdir()
        (self.root / "pkg" / "host.atto").write_text("trusted\n")
        self.commit("tracked nested file")
        git(self.root, "config", "core.ignorecase", "true")
        git(self.root, "config", "core.untrackedCache", "true")
        (self.root / "PKG").mkdir()
        (self.root / "PKG" / "host.atto").write_text("injected\n")
        self.assertEqual(git(self.root, "ls-files", "--others"), "")
        self.deny()

    def test_ignored_untracked_source_tree_is_rejected(self):
        (self.root / ".gitignore").write_text("generated/\n")
        self.commit("ignore generated files")
        (self.root / "generated").mkdir()
        (self.root / "generated" / "injected.java").write_text("class Injected {}\n")
        self.deny()

    def test_untracked_symlink_is_rejected(self):
        (self.root / "shortcut").symlink_to("tracked.txt")
        self.deny()

    def test_untracked_fifo_is_rejected(self):
        os.mkfifo(self.root / "injected-pipe")
        self.deny()

    def test_untracked_empty_directory_is_rejected(self):
        (self.root / "untracked-empty-dir").mkdir()
        self.deny()

    def test_untracked_unix_socket_is_rejected(self):
        sock=socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.addCleanup(sock.close)
        sock.bind(str(self.root / "untracked.sock"))
        self.deny()

    def test_executable_bit_substitution_even_with_core_filemode_disabled(self):
        git(self.root, "config", "core.filemode", "false")
        (self.root / "tracked.txt").chmod(0o755)
        self.deny()

    def test_unicode_decomposed_tracked_path_is_denied(self):
        (self.root / "e\u0301.atto").write_text("source\n")
        self.commit("non-NFC path")
        self.deny()

    def test_windows_reserved_tracked_filename_is_denied_on_linux(self):
        (self.root / "CON.txt").write_text("unsafe on windows\n")
        self.commit("reserved name")
        self.deny()

    def test_unicode_casefold_collision_in_tracked_tree_is_denied(self):
        (self.root / "Stra\u00dfe.atto").write_text("a\n")
        (self.root / "STRASSE.atto").write_text("b\n")
        self.commit("casefold collision")
        self.deny()

    def test_out_of_budget_single_tracked_file_is_denied(self):
        (self.root / "big.bin").write_bytes(b"x" * (8 * 1024 * 1024 + 1))
        self.commit("oversized blob")
        self.deny()

    def test_index_only_staged_extra_file_is_denied(self):
        (self.root / "staged.txt").write_text("staged\n")
        git(self.root, "add", "staged.txt")
        self.deny()

    def test_unapproved_git_control_environment_is_denied(self):
        result = self.check(extra_env={"GIT_INDEX_FILE": str(self.root / ".git" / "index")})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("RAW_CHECKOUT_DENIED", result.stderr)


if __name__ == "__main__":
    unittest.main()
