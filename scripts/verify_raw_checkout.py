#!/usr/bin/env python3
"""Linux Actions preflight: verify exact Git HEAD using raw worktree bytes.

Do not use git status/diff/hash-object for worktree checks: repository
.gitattributes clean filters may execute during those Git commands.
"""
import hashlib
import os
from pathlib import Path
import re
import unicodedata
import stat
import subprocess
import sys

OID = re.compile(rb"[0-9a-f]{40}\Z")
MAX_FILES = 2000
MAX_FILE = 8 * 1024 * 1024
MAX_TOTAL = 64 * 1024 * 1024


def git(*args):
    env = dict(os.environ, GIT_NO_REPLACE_OBJECTS="1")
    return subprocess.check_output(
        ["git", "-c", "core.fsmonitor=false", "--no-optional-locks", *args],
        stderr=subprocess.DEVNULL, env=env,
    )


WIN_RESERVED = re.compile(r"^(?:CONIN\$|CONOUT\$|CON|PRN|AUX|NUL|COM[1-9¹²³]|LPT[1-9¹²³])(?:\.|$)", re.I)


def safe_path(path):
    if not path or path.startswith(b"/") or b"\\" in path:
        return False
    try:
        name = path.decode("utf-8", "strict")
    except UnicodeDecodeError:
        return False
    if unicodedata.normalize("NFC", name) != name:
        return False
    for part in name.split("/"):
        if (part in ("", ".", "..") or part.lower() == ".git" or
            part.endswith((" ", ".")) or WIN_RESERVED.match(part) or
            any(ord(ch) < 32 or ord(ch) == 127 or ch in '<>:"\\|?*' for ch in part)):
            return False
    return True

def tree_entries():
    result = {}
    folded_paths = set()
    folded_prefixes = {}
    for row in git("ls-tree", "-r", "-z", "HEAD").split(b"\0"):
        if not row:
            continue
        metadata, path = row.split(b"\t", 1)
        mode, kind, oid = metadata.split()
        if kind != b"blob" or mode not in (b"100644", b"100755") or not OID.fullmatch(oid):
            raise ValueError("unsupported tracked object")
        if not safe_path(path):
            raise ValueError("nonportable tracked path")
        name = path.decode("utf-8")
        folded = name.casefold()
        if path in result or folded in folded_paths:
            raise ValueError("duplicate or case-colliding tracked path")
        prefix = []
        for piece in name.split("/"):
            prefix.append(piece)
            key = tuple(component.casefold() for component in prefix)
            spelled = tuple(prefix)
            old_spelling = folded_prefixes.get(key)
            if old_spelling is not None and old_spelling != spelled:
                raise ValueError("case-colliding parent path component")
            folded_prefixes[key] = spelled
        folded_paths.add(folded)
        result[path] = (mode, oid)
    if not result or len(result) > MAX_FILES:
        raise ValueError("unexpected tracked file count")
    return result


def index_entries():
    result = {}
    for row in git("ls-files", "--stage", "-z").split(b"\0"):
        if not row:
            continue
        metadata, path = row.split(b"\t", 1)
        mode, oid, stage = metadata.split()
        if stage != b"0" or not safe_path(path) or path in result or not OID.fullmatch(oid):
            raise ValueError("unmerged/invalid index entry")
        result[path] = (mode, oid)
    for row in git("ls-files", "-v", "-z").split(b"\0"):
        if row and row[:2] != b"H ":
            raise ValueError("index contains assume-unchanged or skip-worktree entries")
    return result


def raw_contents(root_fd, path, mode):
    parts = path.split(b"/")
    parent = root_fd
    opened = []
    try:
        for item in parts[:-1]:
            parent = os.open(
                item, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW | os.O_CLOEXEC,
                dir_fd=parent,
            )
            opened.append(parent)
        fd = os.open(
            parts[-1], os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK | os.O_CLOEXEC,
            dir_fd=parent,
        )
        opened.append(fd)
        before = os.fstat(fd)
        if not stat.S_ISREG(before.st_mode) or before.st_size > MAX_FILE:
            raise ValueError("nonregular or oversized worktree entry")
        if bool(before.st_mode & 0o111) != (mode == b"100755"):
            raise ValueError("wrong executable bit")
        buf = bytearray()
        while len(buf) <= MAX_FILE:
            block = os.read(fd, min(65536, MAX_FILE + 1 - len(buf)))
            if not block:
                break
            buf.extend(block)
        after = os.fstat(fd)
        if len(buf) > MAX_FILE or len(buf) != before.st_size or (
            before.st_dev, before.st_ino, before.st_size,
            before.st_mtime_ns, before.st_ctime_ns
        ) != (
            after.st_dev, after.st_ino, after.st_size,
            after.st_mtime_ns, after.st_ctime_ns
        ):
            raise ValueError("worktree entry changed during verification")
        return bytes(buf)
    finally:
        for fd in reversed(opened):
            os.close(fd)


def verify(expected):
    if not re.fullmatch(r"[0-9a-f]{40}", expected or ""):
        raise ValueError("missing or invalid full expected source SHA")
    if os.environ.get("GIT_REPLACE_REF_BASE"):
        raise ValueError("custom Git replacement-ref namespace is forbidden")
    if git("for-each-ref", "--format=%(refname)", "refs/replace").strip():
        raise ValueError("Git replace refs are forbidden in source checkout")
    if git("rev-parse", "--show-object-format").strip() != b"sha1":
        raise ValueError("unsupported Git object format")
    if git("rev-parse", "HEAD").strip().decode("ascii") != expected:
        raise ValueError("source HEAD not pinned")
    if Path.cwd().resolve() != Path(
        os.fsdecode(git("rev-parse", "--show-toplevel").strip())
    ).resolve():
        raise ValueError("verification must run at checkout root")
    expected_tree = tree_entries()
    if index_entries() != expected_tree:
        raise ValueError("index does not match exact source HEAD")
    if git("ls-files", "--others", "-z"):
        raise ValueError("untracked or ignored files in checkout")
    total = 0
    root_fd = os.open(b".", os.O_RDONLY | os.O_DIRECTORY | os.O_CLOEXEC)
    try:
        for path, (mode, oid) in expected_tree.items():
            data = raw_contents(root_fd, path, mode)
            total += len(data)
            if total > MAX_TOTAL:
                raise ValueError("raw checkout exceeds byte budget")
            actual = hashlib.sha1(
                b"blob " + str(len(data)).encode("ascii") + b"\0" + data
            ).hexdigest().encode()
            if actual != oid:
                raise ValueError(
                    "raw worktree blob differs from pinned HEAD: " + os.fsdecode(path)
                )
    finally:
        os.close(root_fd)
    if git("rev-parse", "HEAD").strip().decode("ascii") != expected or (
        index_entries() != expected_tree
    ):
        raise ValueError("HEAD/index changed during verification")
    if git("ls-files", "--others", "-z"):
        raise ValueError("untracked files appeared during verification")


if __name__ == "__main__":
    try:
        verify(os.environ.get("EXPECTED_SHA", ""))
        print("RAW_CHECKOUT_EXACT_HEAD_PASS")
    except (OSError, ValueError, subprocess.CalledProcessError, UnicodeError) as exc:
        print(f"RAW_CHECKOUT_DENIED: {exc}", file=sys.stderr)
        sys.exit(1)
