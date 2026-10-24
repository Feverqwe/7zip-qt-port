"""Allowlisted guest workspace roots and fail-closed data-volume identity."""

import hashlib
import re
import shlex
import subprocess
from .runner_config import load_config


def load_workspaces():
    return {platform: entry["workspace"] for platform, entry in load_config().items()}


def current_branch(root):
    result = subprocess.run(["git", "symbolic-ref", "--quiet", "--short", "HEAD"],
                            cwd=root, text=True, capture_output=True)
    if result.returncode == 1:
        raise ValueError("Runner requires a Git branch; switch from detached HEAD before running")
    result.check_returncode()
    return result.stdout.strip()


def worktree_root(workspace, branch):
    # A bounded ASCII component works on Windows too. Hash the original name
    # so slash replacement, case folding and truncation cannot merge branches.
    label = re.sub(r"[^a-zA-Z0-9_-]", "-", branch)[:48]
    digest = hashlib.sha256(branch.encode("utf-8")).hexdigest()[:20]
    return workspace["root"] + f"/branch-{label}-{digest}"


def mount_guard(workspace):
    return ('test "$(findmnt -n -o UUID --mountpoint ' + shlex.quote(workspace["mount"]) + ')" = '
            + shlex.quote(workspace["filesystem_uuid"]) + '\n')
