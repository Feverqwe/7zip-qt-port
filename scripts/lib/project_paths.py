"""Locate shared local tooling without changing the worktree being developed."""

from pathlib import Path
import subprocess
import sys


def physical_path(path):
    path = Path(path).absolute()
    # Some Windows filesystem drivers resolve only one enclosing junction
    # per call. Continue until nested aliases have all been removed.
    while True:
        resolved = path.resolve()
        if resolved == path:
            return resolved
        path = resolved


def harness_root(source_root):
    root = Path(source_root).resolve()
    if not (root / ".git").is_file():
        return root
    common = subprocess.check_output(
        ["git", "rev-parse", "--path-format=absolute", "--git-common-dir"],
        cwd=root, text=True).strip()
    return Path(common).resolve().parent


if __name__ == "__main__":
    print(physical_path(sys.argv[1]).as_posix())
