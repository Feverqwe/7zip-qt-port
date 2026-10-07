"""Allowlisted guest workspace roots and fail-closed data-volume identity."""

import json
import uuid

DEFAULT_ROOTS = {
    "linux": "/home/runner/sevenzip-desktop/runs",
    "windows": "C:/runner/sevenzip-desktop/runs",
}
LINUX_DATA_ROOT = "/mnt/sevenzip-desktop/runs"


def load_workspaces(state):
    path = state / "workspaces.json"
    if not path.exists():
        return {platform: {"root": root} for platform, root in DEFAULT_ROOTS.items()}
    values = json.loads(path.read_text())
    if not isinstance(values, dict) or set(values) != set(DEFAULT_ROOTS):
        raise ValueError("workspaces.json must define exactly linux and windows")
    for platform, entry in values.items():
        if not isinstance(entry, dict):
            raise ValueError("Workspace entry must specify a project-owned root")
        root = entry.get("root")
        if platform == "linux" and root == LINUX_DATA_ROOT:
            if set(entry) != {"root", "filesystem_uuid"}:
                raise ValueError("Linux data workspace requires its filesystem UUID")
            value = entry["filesystem_uuid"]
            if not isinstance(value, str) or str(uuid.UUID(value)) != value:
                raise ValueError("Workspace UUID must use canonical form")
        elif root != DEFAULT_ROOTS[platform] or set(entry) != {"root"}:
            raise ValueError("Workspace must be an allowlisted project runs directory")
    return values


def mount_guard(workspace):
    # The loader validates the fixed mount root and UUID before any guest I/O.
    return ('test "$(findmnt -n -o UUID --mountpoint /mnt/sevenzip-desktop)" = '
            "'" + workspace["filesystem_uuid"] + "'\n")
