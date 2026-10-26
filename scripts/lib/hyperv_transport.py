"""Synchronize source into the single locked guest worktree, preserving outputs."""

import json
import shlex
import subprocess
from .runner_config import runner_config

PROTECTED = ("/.git/", "/build/", "/ui-reports/", "/runner-tmp/", "/upstream/", "/check.log", "/check.cmd",
             "/runner-command.sh", "/interactive-exit.txt", "/interactive-session.txt",
             "/.runner-owner.json", "/.runner-complete")


def sync_command(record, platform, source, root):
    target = root
    if platform == "windows":
        target = "/" + root[0].lower() + root[2:]
    server = (record.get("config") or runner_config(platform))["rsync"]
    if platform == "linux":
        server = shlex.quote(server)
    elif " " in server:
        server = '"' + server + '"'
    # Changed contents acquire the receiver's current modification time. A local
    # edit can predate the object built from the previous immutable snapshot.
    return ["rsync", "--recursive", "--links", "--perms", "--checksum",
            "--compress", "--compress-level=1",
            "--delete-delay", "--delay-updates", "--stats", "--no-whole-file",
            *["--exclude=" + name for name in PROTECTED],
            "--exclude=__pycache__/", "--exclude=*.pyc", "--exclude=.DS_Store",
            "--rsync-path=" + server, "-e", shlex.join(record["ssh_argv"][:-1]),
            str(source) + "/", f"{record['ssh_argv'][-1]}:{target}/"]


def collect_logs(record, root, directory, command, token, invoke, output):
    def download(source, name):
        options = ["-P" if arg == "-p" else arg for arg in record["ssh_argv"][1:-1]]
        invoke(["scp", *options, f"{record['ssh_argv'][-1]}:{root}/{source}",
                str(directory / name)], stdout=output, stderr=subprocess.STDOUT)

    download("check.log", "check.log")
    details = {}
    try:
        download("runner-tmp/timing.jsonl", "timing.jsonl")
    except subprocess.CalledProcessError:
        (directory / "timing.jsonl").unlink(missing_ok=True)
        details["timing_error"] = "Guest timing report is unavailable"
    if not command or len(command) < 2 or command[1] != "scripts/verify.py":
        return details
    preset = (command + [""])[command.index("--preset") + 1] if "--preset" in command else "dev"
    if preset not in ("dev", "release", "modules-dev", "modules-linux-dev", "modules-gcc-dev",
                       "vcpkg-dev", "vcpkg-release"):
        raise ValueError("Invalid verification preset")
    name = "verify-plan.json" if "--plan" in command else "verify-result.json"
    download(f"build/{preset}/{name}", name)
    verification = json.loads((directory / name).read_text())
    if verification.get("run_token") == token:
        return dict(details, verification=verification)
    (directory / name).unlink()
    return dict(details, verification_error="Report belongs to another invocation")
