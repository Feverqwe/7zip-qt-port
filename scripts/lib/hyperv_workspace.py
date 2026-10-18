"""Allowlisted guest workspace roots and fail-closed data-volume identity."""

import shlex
from .runner_config import load_config


def load_workspaces():
    return {platform: entry["workspace"] for platform, entry in load_config().items()}


def mount_guard(workspace):
    return ('test "$(findmnt -n -o UUID --mountpoint ' + shlex.quote(workspace["mount"]) + ')" = '
            + shlex.quote(workspace["filesystem_uuid"]) + '\n')
