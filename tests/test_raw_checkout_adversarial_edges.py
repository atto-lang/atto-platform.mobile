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


    def test_tracked_executable_with_owner_execute_is_accepted(self):
        tracked = self.root / "tracked.txt"
        tracked.chmod(0o700)
        self.commit("tracked executable")
        result = self.check()
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_tracked_executable_group_only_execute_is_denied(self):
        tracked = self.root / "tracked.txt"
        tracked.chmod(0o755)
        self.commit("tracked executable")
        # Owner can read the bytes; only group may execute.
        tracked.chmod(0o450)
        self.deny()

    def test_tracked_executable_other_only_execute_is_denied(self):
        tracked = self.root / "tracked.txt"
        tracked.chmod(0o755)
        self.commit("tracked executable")
        # Previously bool(st_mode & 0o111) incorrectly accepted this.
        tracked.chmod(0o405)
        self.deny()

    def test_tracked_nonexecutable_group_execute_is_denied(self):
        (self.root / "tracked.txt").chmod(0o450)
        self.deny()

    def test_tracked_external_hardlink_even_with_identical_bytes_is_denied(self):
        with tempfile.TemporaryDirectory(prefix="raw-checkout-outside-") as outside:
            other = Path(outside) / "shared-source"
            os.link(self.root / "tracked.txt", other)
            self.assertEqual((self.root / "tracked.txt").stat().st_nlink, 2)
            self.deny()

    def test_tracked_setuid_bit_not_in_git_mode_is_denied(self):
        (self.root / "tracked.txt").chmod(0o4644)
        self.deny()

    def test_tracked_setgid_bit_not_in_git_mode_is_denied(self):
        (self.root / "tracked.txt").chmod(0o2644)
        self.deny()

    def test_tracked_sticky_bit_not_in_git_mode_is_denied(self):
        (self.root / "tracked.txt").chmod(0o1644)
        self.deny()


    def test_metadata_race_changes_mode_or_link_count_even_if_times_match(self):
        # Deterministic post-read metadata substitution: no timing-sensitive thread.
        # The original source compared timestamps but not mode/nlink snapshots.
        import runpy
        import stat
        from types import SimpleNamespace
        from unittest.mock import patch

        raw_contents = runpy.run_path(str(SCRIPT))["raw_contents"]
        root_fd = os.open(self.root, os.O_RDONLY | os.O_DIRECTORY | os.O_CLOEXEC)
        try:
            native_fstat = os.fstat
            for name, forged in (("st_mode", stat.S_IFREG | 0o755),
                                 ("st_nlink", 2)):
                with self.subTest(forged_metadata=name):
                    calls = [0]

                    def fstat_after_read(fd):
                        calls[0] += 1
                        metadata = native_fstat(fd)
                        if calls[0] != 2:
                            return metadata
                        keys = ("st_dev", "st_ino", "st_size",
                                "st_mtime_ns", "st_ctime_ns",
                                "st_mode", "st_nlink")
                        values = {key: getattr(metadata, key) for key in keys}
                        values[name] = forged
                        return SimpleNamespace(**values)

                    with patch.object(os, "fstat", side_effect=fstat_after_read):
                        with self.assertRaises(ValueError):
                            raw_contents(root_fd, b"tracked.txt", b"100644")
                    self.assertEqual(calls[0], 2)
        finally:
            os.close(root_fd)


if __name__ == "__main__":
    unittest.main()
