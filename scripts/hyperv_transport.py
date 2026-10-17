"""Synchronize source into the single locked guest worktree, preserving outputs."""

import json
import shlex
import subprocess

WINDOWS_RSYNC = "C:/runner/sevenzip-desktop/tools/rsync/msys64/usr/bin/rsync.exe"
PROTECTED = ("/.git/", "/build/", "/ui-reports/", "/runner-tmp/", "/upstream/", "/check.log", "/check.cmd",
             "/runner-command.sh", "/interactive-exit.txt", "/interactive-session.txt",
             "/.runner-owner.json", "/.runner-complete")


def sync_command(record, platform, source, root):
    target = root
    server = "rsync"
    if platform == "windows":
        target = "/c/" + root.removeprefix("C:/")
        server = WINDOWS_RSYNC
    return ["rsync", "--recursive", "--links", "--perms", "--times", "--checksum",
            "--compress", "--compress-level=1",
            "--delete-delay", "--delay-updates", "--stats", "--no-whole-file",
            *["--exclude=" + name for name in PROTECTED],
            "--exclude=__pycache__/", "--exclude=*.pyc", "--exclude=.DS_Store",
            "--rsync-path=" + server, "-e", shlex.join(record["ssh_argv"][:-1]),
            str(source) + "/", f"runner@{record['ip']}:{target}/"]


def collect_logs(record, root, directory, command, token, invoke, output):
    def download(source, name):
        invoke(["scp", *record["ssh_argv"][1:-1], f"runner@{record['ip']}:{root}/{source}",
                str(directory / name)], stdout=output, stderr=subprocess.STDOUT)

    download("check.log", "check.log")
    if not command or len(command) < 2 or command[1] != "scripts/verify.py":
        return {}
    preset = (command + [""])[command.index("--preset") + 1] if "--preset" in command else "dev"
    if preset not in ("dev", "release"):
        raise ValueError("Invalid verification preset")
    name = "verify-plan.json" if "--plan" in command else "verify-result.json"
    download(f"build/{preset}/{name}", name)
    verification = json.loads((directory / name).read_text())
    if verification.get("run_token") == token:
        return {"verification": verification}
    (directory / name).unlink()
    return {"verification_error": "Report belongs to another invocation"}
