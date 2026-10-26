"""Synchronize through pinned cp2 and the configured authenticated SSH."""

import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

from . import cp2_install, cp2_snapshot, hyperv_cp2_install
from .hyperv_transport import PROTECTED
from .project_paths import harness_root

ROOT = Path(__file__).resolve().parents[2]


def ssh_launcher():
    source = ROOT / "tools/cp2/cp2-ssh.rs"
    tools = harness_root(ROOT) / "build/runners/tools"
    digest = hashlib.sha256(source.read_bytes()).hexdigest()[:20]
    directory = tools / "cp2-ssh" / digest
    binary = directory / ("ssh.exe" if os.name == "nt" else "ssh")
    if binary.exists():
        return binary
    directory.mkdir(parents=True, exist_ok=True)
    with cp2_install.install_lock(directory / "install.lock"):
        if not binary.exists():
            pin = hyperv_cp2_install.pin()
            cargo, environment = cp2_install.rust_environment(tools, directory, pin)
            with tempfile.TemporaryDirectory(dir=directory) as temporary:
                staged = Path(temporary) / binary.name
                subprocess.run([str(cargo.with_name("rustc" + (".exe" if os.name == "nt" else ""))),
                                "+" + pin["rust"], "--edition=2021", "-O", str(source), "-o", str(staged)],
                               env=environment, check=True)
                staged.replace(binary)
    return binary


def command(binary, record, source, root, receiver, checksum=False):
    argv = [str(binary), str(source) + "/", record["ssh_argv"][-1] + ":" + root,
            "--remote-path", receiver, "--no-auto-install", "--delete", "--stats", "--no-progress"]
    if record["platform"] == "windows":
        argv.append("--no-perms")
    if checksum:
        argv += ["--checksum", "--verify"]
    for pattern in (*PROTECTED, "__pycache__/", "*.pyc", ".DS_Store"):
        argv += ["--exclude", pattern]
    return argv


def synchronize(record, platform, source, root, token, directory, remote, powershell, output,
                checksum=False):
    started = time.monotonic()
    client = hyperv_cp2_install.local()["path"]
    launcher = ssh_launcher()
    receiver = hyperv_cp2_install.ensure(record, remote, output)
    preparation = time.monotonic() - started
    key = hashlib.sha256(json.dumps([record["ssh_argv"], root]).encode()).hexdigest()
    cache = harness_root(ROOT) / "build/runners/hyperv/cache/cp2-times" / (key + ".json")
    with tempfile.TemporaryDirectory(prefix="cp2-", dir=None if os.name == "nt" else "/tmp") as temporary:
        temporary = Path(temporary)
        staged = temporary / "source"
        entries = cp2_snapshot.prepare(source, staged, cache, receiver["time_ns"])
        argv = list(record["ssh_argv"])
        argv[0] = shutil.which(argv[0]) or argv[0]
        config = temporary / "ssh-argv"
        config.write_bytes("\0".join(argv).encode())
        environment = dict(os.environ, PATH=str(launcher.parent) + os.pathsep + os.environ["PATH"],
                           SEVENZIP_CP2_SSH_ARGV=str(config), TMPDIR=str(temporary))
        if platform == "windows":
            environment["SEVENZIP_CP2_WINDOWS_PROBE"] = powershell(
                "[Console]::Out.WriteLine($env:PROCESSOR_ARCHITECTURE)")
        try:
            started = time.monotonic()
            result = subprocess.run(command(client, record, staged, root, receiver["server_path"], checksum),
                                    env=environment, stdout=output, stderr=subprocess.STDOUT, check=True)
            transfer = time.monotonic() - started
            cp2_snapshot.commit(cache, entries)
        finally:
            # Close only masters created by this invocation; all sockets are
            # in its private short path. Native Windows cp2 doesn't multiplex.
            for socket in temporary.glob("cp2-ssh-*"):
                subprocess.run([*argv[:-1], "-S", str(socket), "-O", "exit", argv[-1]],
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    details = {"preparation_seconds": round(preparation, 3), "transfer_seconds": round(transfer, 3),
               "receiver": receiver, "checksum": checksum, "timestamp_cache": str(cache),
               "exit_code": result.returncode}
    (directory / "cp2-sync.json").write_text(json.dumps(details, indent=2) + "\n")
    return details
