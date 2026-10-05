#!/usr/bin/env python3
"""Bump the CMake application version and atomically push its commit and tag."""
import argparse
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
VERSION = r"(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)"
PROJECT = re.compile(r"(project\(sevenzip_desktop VERSION )(" + VERSION + r")(?= LANGUAGES C CXX\))")


def git(*args, check=True):
    return subprocess.run(["git", *args], cwd=ROOT, check=check,
                          capture_output=True, text=True)


def current_version():
    match = PROJECT.search((ROOT / "CMakeLists.txt").read_text())
    if not match:
        raise ValueError("Application version not found in CMakeLists.txt")
    return match.group(2)


def next_version(current, choice):
    parts = [int(value) for value in current.split(".")]
    if choice in ("patch", "minor", "major"):
        index = {"major": 0, "minor": 1, "patch": 2}[choice]
        parts[index] += 1
        parts[index + 1:] = [0] * (2 - index)
        return ".".join(map(str, parts))
    if not re.fullmatch(VERSION, choice):
        raise ValueError("Use patch, minor, major or a version such as 0.2.0")
    if tuple(map(int, choice.split("."))) <= tuple(parts):
        raise ValueError("New version must be greater than the current version")
    return choice


def check_tag(tag):
    expected = "v" + current_version()
    if tag != expected:
        raise ValueError(f"Tag {tag!r} differs from CMake version {expected!r}")


def release(choice, remote, dry_run):
    if git("status", "--porcelain").stdout.strip():
        raise ValueError("Commit or stash all changes before releasing")
    branch = git("symbolic-ref", "--quiet", "--short", "HEAD").stdout.strip()
    current = current_version()
    version = next_version(current, choice)
    tag = "v" + version
    git("remote", "get-url", remote)
    if git("show-ref", "--verify", "--quiet", "refs/tags/" + tag, check=False).returncode == 0:
        raise ValueError(f"Local tag {tag} already exists")
    if git("ls-remote", "--tags", remote, "refs/tags/" + tag).stdout.strip():
        raise ValueError(f"Remote tag {tag} already exists")
    print(f"Release {current} -> {version} on {branch}; remote {remote}", flush=True)
    push = ("push", "--atomic", remote, f"HEAD:refs/heads/{branch}", f"refs/tags/{tag}")
    if dry_run:
        print(f"Would update CMakeLists.txt, commit {tag}, annotate {tag}, then git {' '.join(push)}")
        return
    path = ROOT / "CMakeLists.txt"
    original = path.read_text()
    path.write_text(PROJECT.sub(lambda match: match.group(1) + version, original, count=1))
    try:
        git("add", "--", "CMakeLists.txt")
        git("commit", "-m", tag, "--", "CMakeLists.txt")
    except subprocess.CalledProcessError:
        # No release commit exists; restore only this script's version edit.
        path.write_text(original)
        git("reset", "--", "CMakeLists.txt")
        raise
    git("tag", "-a", tag, "-m", "Release " + tag)
    try:
        git(*push)
    except subprocess.CalledProcessError:
        print(f"Push failed; local release commit and {tag} retained. Retry: git {' '.join(push)}",
              file=sys.stderr)
        raise
    print(f"Pushed {tag}; GitHub Actions will build runtime artifacts for all platforms.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version", nargs="?", help="patch, minor, major or MAJOR.MINOR.PATCH")
    parser.add_argument("--remote", default="origin")
    parser.add_argument("--dry-run", action="store_true", help="Check without changing files, commits or tags")
    parser.add_argument("--check-tag", help="CI: require tag to match the CMake application version")
    args = parser.parse_args()
    try:
        if args.check_tag is not None:
            check_tag(args.check_tag)
            return
        choice = args.version
        if choice is None:
            print(f"Current version: {current_version()}\n1) patch\n2) minor\n3) major")
            choice = {"1": "patch", "2": "minor", "3": "major"}.get(input("Choose 1-3: ").strip())
            if choice is None:
                raise ValueError("Choose 1, 2 or 3")
        release(choice, args.remote, args.dry_run)
    except (ValueError, EOFError, subprocess.CalledProcessError) as error:
        detail = error.stderr if isinstance(error, subprocess.CalledProcessError) else str(error)
        parser.exit(1, f"Release failed: {detail}\n")


if __name__ == "__main__":
    main()
