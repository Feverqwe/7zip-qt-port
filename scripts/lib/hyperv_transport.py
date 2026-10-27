"""Shared cp2 exclusions and collection of guest diagnostics."""

import json
import subprocess
from .runner_config import runner_config
from .build_paths import branch_component

PROTECTED = ("/.git/", "/build/", "/ui-reports/", "/runner-tmp/", "/upstream/", "/check.log", "/check.cmd",
             "/runner-command.sh", "/interactive-exit.txt", "/interactive-session.txt",
             "/.runner-owner.json", "/.runner-complete", "/workspace-env.cmd")


def collect_logs(record, root, directory, command, token, invoke, output, branch):
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
    # A synchronized source lives under build/sources/<branch>; reports are
    # in the persistent branch workspace beside sources, outside volatile RAM.
    tag = record["platform"] + "-x86_64"
    relative = f"../../branches/{branch_component(branch)}/logs/{tag}/{preset}/{name}"
    download(relative, name)
    verification = json.loads((directory / name).read_text())
    if verification.get("run_token") == token:
        return dict(details, verification=verification)
    (directory / name).unlink()
    return dict(details, verification_error="Report belongs to another invocation")
