"""Transfer immutable source snapshots; each run owns its destination."""

import json
import re
import shlex

WINDOWS_RSYNC = "C:/runner/sevenzip-desktop/tools/rsync/msys64/usr/bin/rsync.exe"


def previous_snapshot(state, platform):
    prefix = ("C:/runner/sevenzip-desktop/runs/" if platform == "windows" else
              "/home/runner/sevenzip-desktop/runs/")
    for path in sorted(state.glob(f"*/{platform}/run.json"), reverse=True):
        try:
            record = json.loads(path.read_text())
        except (ValueError, OSError):
            continue
        directory = record.get("directory", "")
        if (record.get("status") == "passed" and record.get("transport") == "rsync"
                and directory.startswith(prefix)
                and re.fullmatch(r"[0-9]{8}T[0-9]{6}Z-[0-9a-f]{8}", directory[len(prefix):])):
            return directory
    return None


def transfer(record, platform, archive, root, mode, previous, invoke, remote, output):
    options = record["ssh_argv"][1:-1]
    if mode == "scp":
        invoke(["scp", *options, str(archive),
                f"runner@{record['ip']}:{root}/{archive.name}"], **output)
        return
    if previous:
        source = previous + "/" + archive.name
        destination = root + "/" + archive.name
        if platform == "windows":
            remote(record, f"if (Test-Path '{source}') {{ Copy-Item '{source}' '{destination}' }}\n",
                   **output)
        else:
            remote(record, f"if test -f {shlex.quote(source)}; then "
                   f"cp {shlex.quote(source)} {shlex.quote(destination)}; fi\n", **output)
    target = root
    server = "rsync"
    if platform == "windows":
        # MSYS2 maps /c/... to the guest's C: drive. SSH still uses Windows OpenSSH.
        target = "/c/" + root.removeprefix("C:/")
        server = WINDOWS_RSYNC
    invoke(["rsync", "--times", "--checksum", "--partial", "--stats", "--no-whole-file",
            "--rsync-path=" + server, "-e", shlex.join(record["ssh_argv"][:-1]),
            str(archive), f"runner@{record['ip']}:{target}/{archive.name}"], **output)
