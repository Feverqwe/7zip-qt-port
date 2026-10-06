#!/usr/bin/env python3
"""Sync one worktree snapshot and run project scripts in the shared Hyper-V guests."""

import argparse
import base64
from concurrent.futures import ThreadPoolExecutor
import datetime
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tarfile
import tempfile
import uuid

from hyperv_transport import previous_snapshot, transfer
from hyperv_interactive import interactive_command

ROOT = Path(__file__).resolve().parents[1]
STATE = ROOT / "build/runners/hyperv"
KEY = Path.home() / ".ssh/id_ed25519"
READY = Path.home() / ".codex/skills/runners/scripts/hyperv_ready.py"
QT_VERSION = "6.12.0"
QT_KITS = {
    "linux": f"/home/runner/sevenzip-desktop/tools/Qt/{QT_VERSION}/gcc_64",
    "windows": f"C:/runner/sevenzip-desktop/tools/Qt/{QT_VERSION}/msvc2022_64",
}


def invoke(args, **kwargs):
    return subprocess.run(args, check=True, text=True, **kwargs)


def powershell(script):
    wrapped = ("$ProgressPreference='SilentlyContinue'; $ErrorActionPreference='Stop'; "
               "try {\n" + script + "\n} catch { Write-Error $_; exit 1 }")
    encoded = base64.b64encode(wrapped.encode("utf-16le")).decode()
    return "powershell.exe -NoProfile -NonInteractive -EncodedCommand " + encoded


def discover(platform):
    result = invoke([sys.executable, str(READY), platform, "--start"], capture_output=True)
    record = json.loads(result.stdout)
    record["name"] = record["vm"]
    record["id"] = record["vm_id"]
    STATE.mkdir(parents=True, exist_ok=True)
    record["key_path"] = str(KEY)
    (STATE / f"{platform}.json").write_text(json.dumps(record, indent=2) + "\n")
    return record


def guest_options(record):
    return record["ssh_argv"][1:-1]


def remote(record, script, **kwargs):
    windows = record["name"] == "runner-windows"
    return invoke([*record["ssh_argv"],
                   powershell("& ([ScriptBlock]::Create([Console]::In.ReadToEnd()))")
                   if windows else "bash -se"], input=script, **kwargs)


def snapshot(destination, upstream):
    # Validate actual project changes locally; upstream notices contain
    # historical whitespace that must not become newly staged guest changes.
    invoke(["git", "diff", "--check"], cwd=ROOT)
    invoke(["git", "diff", "--cached", "--check"], cwd=ROOT)
    files = subprocess.check_output(
        ["git", "ls-files", "-z", "--cached"], cwd=ROOT)
    # Only reviewed project content goes to the shared runners. New unrelated
    # files, ignored state and the Git database are never included.
    roots = {".agents", "cmake", "docs", "scripts", "src", "tests", "third_party", "resources"}
    top_files = {".clang-format", ".gitattributes", ".gitignore", "AGENTS.md",
                 "CMakeLists.txt", "CMakePresets.json", "LICENSE", "README.md",
                 "upstream.lock.json", "windows-runtime.lock.json"}
    entries = set(files.split(b"\0")) - {b""}
    untracked = subprocess.check_output(
        ["git", "ls-files", "-z", "--others", "--exclude-standard"], cwd=ROOT)
    bundled = {b"scripts/hyperv-runner.py", b"scripts/hyperv_transport.py"}
    omitted = [os.fsdecode(name) for name in untracked.split(b"\0")
               if name and name not in bundled and Path(os.fsdecode(name)).parts[0] in roots]
    if omitted:
        print("Not synced (use git add to include): " + ", ".join(omitted), file=sys.stderr)
    entries.update(bundled)
    revision = json.loads((ROOT / "docs/upstream-baseline.json").read_text())["revision"]
    bundle = destination.parent / "upstream.bundle"
    # A bundle requires a named ref. Create it only in a disposable mirror,
    # preserving the source checkout's refs, index and files.
    with tempfile.TemporaryDirectory(dir=destination.parent) as temporary:
        mirror = Path(temporary) / "source.git"
        invoke(["git", "clone", "-q", "--bare", "--shared", str(upstream), str(mirror)])
        invoke(["git", "-C", str(mirror), "update-ref", "refs/heads/runner-baseline", revision])
        invoke(["git", "-C", str(mirror), "bundle", "create", str(bundle), "runner-baseline"])
    with tarfile.open(destination, "w" if destination.suffix == ".tar" else "w:gz",
                      dereference=False) as archive:
        for raw in sorted(entries):
            name = os.fsdecode(raw)
            if Path(name).parts[0] not in roots and name not in top_files:
                continue
            path = ROOT / name
            if path.exists() or path.is_symlink():
                archive.add(path, arcname=name, recursive=False)
        archive.add(bundle, arcname="upstream.bundle", recursive=False)
    return hashlib.sha256(destination.read_bytes()).hexdigest()


def execute_check(record, platform, preset, archive, digest, root, command=None,
                  prepare_runtime=True, output=None, mode="scp", previous=None, interactive=False,
                  elevated=False):
    transport = {} if output is None else {"stdout": output, "stderr": subprocess.STDOUT}
    payload = command_payload(preset, command)
    if platform == "linux":
        remote(record, f"mkdir -p {shlex.quote(root)}\n", **transport)
    else:
        remote(record, f"New-Item -ItemType Directory '{root}' | Out-Null", **transport)
    transfer(record, platform, archive, root, mode, previous, invoke, remote, transport)
    if platform == "linux":
        script = f"""cd {shlex.quote(root)}
printf '%s  {archive.name}\\n' {digest} | sha256sum -c -
tar xf {archive.name}
git init -q
printf '/source.tar.gz\\n/source.tar\\n/check.log\\n/upstream.bundle\\n/upstream/\\n' >> .git/info/exclude
git add -A
git -c user.name='Runner snapshot' -c user.email='runner@example.invalid' commit -qm 'Runner snapshot'
export CMAKE_BUILD_PARALLEL_LEVEL=4 SEVENZIP_SCRIPT_OUTPUT=compact
export CMAKE_PREFIX_PATH="{QT_KITS['linux']}"
export PATH="{QT_KITS['linux']}/bin:/home/runner/sevenzip-desktop/tools/formatter/bin:$PATH"
# The VM has no accelerated GPU. Serialize native display fixtures to avoid
# competing software compositors and foreground-input timing failures.
export LIBGL_ALWAYS_SOFTWARE=1 SEVENZIP_CTEST_JOBS=1
set +e
(
set -e
"""
        if prepare_runtime:
            script += """git clone -q -b runner-baseline upstream.bundle upstream
./scripts/build-codec-runtime.sh "$PWD/upstream"
./scripts/build-sfx.sh "$PWD/upstream"
make -s -C upstream/CPP/7zip/Bundles/Alone2 -f makefile.gcc -j4
cp upstream/CPP/7zip/Bundles/Alone2/_o/7zz build/codec-runtime/7zz
export PATH="$PWD/build/codec-runtime:$PATH"
export SEVENZIP_CODEC_MODULE_DIR="$PWD/build/codec-runtime"
"""
        script += payload
        script += ") > check.log 2>&1\ncode=$?\ntail -n 80 check.log\nexit \"$code\"\n"
        script = "set -euo pipefail\n" + script
    else:
        kit = "C:/runner/htopqt"
        commands = ['@echo off', f'call "{kit}/VS/VC/Auxiliary/Build/vcvars64.bat"',
                    'if errorlevel 1 exit /b %errorlevel%',
                    f'set "PATH={root}/build/codec-runtime;C:/runner/sevenzip-desktop/tools/formatter/Scripts;{kit}/Python;{kit}/Python/Scripts;{kit}/Git/bin;{QT_KITS["windows"]}/bin;%PATH%"',
                    f'set "CMAKE_PREFIX_PATH={QT_KITS["windows"]}"',
                    'set "CC=cl.exe"',
                    'set "CXX=cl.exe"',
                    'set "CMAKE_BUILD_PARALLEL_LEVEL=4"',
                    'set "SEVENZIP_SCRIPT_OUTPUT=compact"',
                    'set "SEVENZIP_CTEST_JOBS=1"',
                    f'set "SEVENZIP_CODEC_MODULE_DIR={root}/build/codec-runtime"',
                    f'cd /d "{root}"',
                    'git init -q', 'if errorlevel 1 exit /b %errorlevel%',
                    'git config core.autocrlf false',
                    'if errorlevel 1 exit /b %errorlevel%',
                    'echo /check.cmd>>.git/info/exclude',
                    'echo /check.log>>.git/info/exclude',
                    'echo /runner-command.sh>>.git/info/exclude',
                    'echo /source.tar.gz>>.git/info/exclude',
                    'echo /source.tar>>.git/info/exclude',
                    'echo /upstream.bundle>>.git/info/exclude',
                    'echo /upstream/>>.git/info/exclude',
                    'git add -A', 'if errorlevel 1 exit /b %errorlevel%',
                    'git -c user.name="Runner snapshot" -c user.email="runner@example.invalid" commit -qm "Runner snapshot"',
                    'if errorlevel 1 exit /b %errorlevel%']
        if prepare_runtime:
            commands += ['git clone -q -b runner-baseline upstream.bundle upstream',
                    'if errorlevel 1 exit /b %errorlevel%',
                    'powershell.exe -NoProfile -ExecutionPolicy RemoteSigned -File scripts/build-codec-runtime.ps1 -UpstreamCheckout upstream -Architecture x64',
                    'if errorlevel 1 exit /b %errorlevel%',
                    'powershell.exe -NoProfile -ExecutionPolicy RemoteSigned -File scripts/build-sfx.ps1 -UpstreamCheckout upstream -Architecture x64',
                    'if errorlevel 1 exit /b %errorlevel%']
        commands += ['bash runner-command.sh', 'if errorlevel 1 exit /b %errorlevel%']
        commands += ['exit /b 0']
        batch = "\r\n".join(commands) + "\r\n"
        encoded = base64.b64encode(batch.encode()).decode()
        payload_encoded = base64.b64encode(payload.encode("utf-8")).decode()
        launch = (interactive_command(root, elevated) if interactive else
                  "& cmd.exe /d /c 'check.cmd > check.log 2>&1'\n$code=$LASTEXITCODE\n")
        script = f"""Set-Location '{root}'
if ((Get-FileHash {archive.name} -Algorithm SHA256).Hash.ToLower() -ne '{digest}') {{ throw 'Snapshot hash mismatch' }}
tar.exe -xf {archive.name}
if ($LASTEXITCODE -ne 0) {{ throw 'Extraction failed' }}
[IO.File]::WriteAllBytes('{root}/check.cmd', [Convert]::FromBase64String('{encoded}'))
[IO.File]::WriteAllBytes('{root}/runner-command.sh', [Convert]::FromBase64String('{payload_encoded}'))
{launch}
Get-Content check.log -Tail 80
if ($code -ne 0) {{ exit $code }}
"""
    remote(record, script, **transport)


def command_payload(preset, command):
    """Use the same fail-fast Bash commands in both guest environments."""
    commands = [command] if command is not None else [["bash", "scripts/check.sh"]]
    if command is None and preset == "release":
        commands += [["bash", "scripts/build.sh", "release"],
                     ["bash", "scripts/ctest.sh", "--preset", "release", "--output-on-failure"]]
    return "set -euo pipefail\n" + "".join(shlex.join(item) + "\n" for item in commands)


def script_command(arguments, platforms):
    """Resolve a project script without interpreting arguments as shell code."""
    path = Path(arguments[0])
    if path.is_absolute() or ".." in path.parts or not (ROOT / path).is_file():
        raise ValueError("Script must be an existing path relative to the project root")
    tracked = subprocess.check_output(["git", "ls-files", "-z", "--cached"], cwd=ROOT)
    if os.fsencode(path.as_posix()) not in tracked.split(b"\0"):
        raise ValueError("Stage the selected script with git add before syncing")
    if path.parts[0] not in {"scripts", "tests", "src", "resources", "docs", "cmake", ".agents", "third_party"}:
        raise ValueError("Selected script is outside the synced project directories")
    interpreters = {".sh": ["bash"], ".py": ["python3"] if platforms == ["linux"] else ["python"],
                    ".ps1": ["powershell.exe", "-NoProfile", "-File"]}
    if path.suffix not in interpreters:
        raise ValueError("Supported project scripts: .sh, .py, .ps1")
    if path.suffix == ".ps1" and platforms != ["windows"]:
        raise ValueError("PowerShell scripts require the windows platform")
    return [*interpreters[path.suffix], path.as_posix(), *arguments[1:]]


def run_platform(platform, local, archive, digest, preset, command, prepare_runtime, mode="rsync",
                 interactive=False, elevated=False):
    result = {"platform": platform, "sha256": digest, "status": "running", "transport": mode,
              "interactive": interactive and platform == "windows",
              "elevated": elevated and platform == "windows"}
    directory = local / platform
    directory.mkdir()
    root = ("C:/runner/sevenzip-desktop/runs/" if platform == "windows" else
            "/home/runner/sevenzip-desktop/runs/") + local.name
    result["directory"] = root
    print(f"[{platform}] Preparing runner; log: {directory / 'transport.log'}", flush=True)
    with (directory / "transport.log").open("w") as output:
        try:
            record = discover(platform)
            result.update(record)
            print(f"[{platform}] Syncing snapshot and running script", flush=True)
            resolved = list(command) if command else None
            if resolved and resolved[0] == "python" and platform == "linux":
                resolved[0] = "python3"
            execute_check(record, platform, preset, archive, digest, root,
                          resolved, prepare_runtime, output, mode,
                          previous_snapshot(STATE, platform) if mode == "rsync" else None,
                          interactive and platform == "windows", elevated and platform == "windows")
            result.update(status="passed", exit_code=0)
        except subprocess.CalledProcessError as error:
            result.update(status="failed", exit_code=error.returncode, error=str(error))
            if error.stderr:
                result["error"] += ": " + error.stderr.strip()
                output.write(error.stderr)
        except KeyboardInterrupt:
            result.update(status="interrupted", exit_code=130)
            raise
        except Exception as error:
            result.update(status="error", exit_code=1, error=str(error))
        finally:
            if "ssh_argv" in result:
                try:
                    invoke(["scp", *guest_options(result),
                            f"runner@{result['ip']}:{root}/check.log",
                            str(directory / "check.log")], stdout=output, stderr=subprocess.STDOUT)
                except subprocess.CalledProcessError as error:
                    result["log_error"] = str(error)
            (directory / "run.json").write_text(json.dumps(result, indent=2) + "\n")
    print(f"[{platform}] {result['status']}; logs: {directory}", flush=True)
    if result["status"] != "passed":
        detail = (f"exit {result['exit_code']}" if result["status"] == "failed"
                  else result.get("error", result["status"]))
        print(f"[{platform}] {detail}", flush=True)
        log = directory / "check.log"
        if not log.exists():
            log = directory / "transport.log"
        for line in log.read_text(errors="replace").splitlines()[-20:]:
            print(f"[{platform}] {line}", flush=True)
    return result


def run_platforms(platforms, preset, upstream, command=None, prepare_runtime=True, mode="rsync",
                  interactive=False, elevated=False):
    if elevated and (not interactive or "windows" not in platforms):
        raise ValueError("Elevated fixtures require an interactive Windows guest")
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    local = STATE / (stamp + "-" + uuid.uuid4().hex[:8])
    local.mkdir(parents=True)
    archive = local / ("source.tar" if mode == "rsync" else "source.tar.gz")
    digest = snapshot(archive, upstream)
    summary = {"run_id": local.name, "sha256": digest, "command": command,
               "preset": preset, "prepare_runtime": prepare_runtime, "transport": mode,
               "interactive": interactive, "elevated": elevated,
               "revision": subprocess.check_output(
                   ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
               "status": "running", "platforms": []}
    report = local / "run.json"
    report.write_text(json.dumps(summary, indent=2) + "\n")
    try:
        with ThreadPoolExecutor(max_workers=len(platforms)) as executor:
            futures = [executor.submit(run_platform, platform, local, archive, digest,
                                       preset, command, prepare_runtime, mode,
                                       interactive, elevated) for platform in platforms]
            summary["platforms"] = [future.result() for future in futures]
        summary["status"] = ("passed" if all(item["status"] == "passed"
                                              for item in summary["platforms"]) else "failed")
    except KeyboardInterrupt:
        summary["status"] = "interrupted"
        raise
    finally:
        report.write_text(json.dumps(summary, indent=2) + "\n")
        print(f"Report: {report}", flush=True)
    return 0 if summary["status"] == "passed" else 1


def probe_platform(platform):
    """Capture each guest's probe output so parallel results stay readable."""
    record = discover(platform)
    if platform == "linux":
        script = ("set -euo pipefail\nuname -a\ncmake --version\ngcc --version\n"
                  f"{QT_KITS['linux']}/bin/qmake -query QT_VERSION\n"
                  "/home/runner/sevenzip-desktop/tools/formatter/bin/clang-format --version\n")
    else:
        script = "Get-CimInstance Win32_OperatingSystem | Select Caption,Version,OSArchitecture | ConvertTo-Json; "
        for tool in ["C:/runner/htopqt/Python/Scripts/cmake.exe --version",
                     f"{QT_KITS['windows']}/bin/qmake.exe -query QT_VERSION",
                     "C:/runner/sevenzip-desktop/tools/formatter/Scripts/clang-format.exe --version"]:
            script += f"& {tool}; if ($LASTEXITCODE -ne 0) {{ exit $LASTEXITCODE }}; "
    return remote(record, script, capture_output=True)


def probe_platforms(platforms):
    failed = False
    with ThreadPoolExecutor(max_workers=len(platforms)) as executor:
        futures = [(platform, executor.submit(probe_platform, platform)) for platform in platforms]
        for platform, future in futures:
            try:
                result = future.result()
                print(f"[{platform}] passed\n{result.stdout}", end="", flush=True)
                if result.stderr:
                    print(result.stderr, end="", file=sys.stderr)
            except Exception as error:
                failed = True
                print(f"[{platform}] failed: {error}", file=sys.stderr, flush=True)
                if isinstance(error, subprocess.CalledProcessError):
                    for output in (error.stdout, error.stderr):
                        if output:
                            print(output, end="", file=sys.stderr)
    return 1 if failed else 0


def parse_arguments(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    actions = parser.add_subparsers(dest="action", required=True)
    descriptions = {"probe": "Start selected guests and inspect their toolchains",
                    "check": "Sync a snapshot and run the project validation gate",
                    "run": "Sync a snapshot and run a staged project script"}
    for action, description in descriptions.items():
        subparser = actions.add_parser(action, help=description, description=description)
        subparser.add_argument("platform", choices=["linux", "windows", "all"],
                               help="guest platform, or both guests in parallel")
        if action in ("check", "run"):
            subparser.add_argument("--interactive", action="store_true",
                                   help="run Windows checks in the logged-on runner's desktop session")
            subparser.add_argument("--elevated", action="store_true",
                                   help="use that runner's administrator token for privileged Windows fixtures (requires --interactive)")
            subparser.add_argument("--transport", choices=["rsync", "scp"], default="rsync",
                                   help="rsync delta transfer (default) or compressed archive over scp")
            subparser.add_argument("--upstream", type=Path, default=ROOT.parent / "7zip-upstream",
                                   help="read-only checkout containing the pinned engine revision")
            subparser.add_argument("--skip-runtime", action="store_true",
                                   help="skip official engine/SFX builds for scripts that do not need them")
        if action == "check":
            subparser.add_argument("--preset", choices=["dev", "release"], default="dev",
                                   help="dev gate (default), or dev gate followed by Release checks")
        if action == "run":
            subparser.epilog = "Use -- <project-script> [arguments] after runner options."
    separator = argv.index("--") if "--" in argv else len(argv)
    args = parser.parse_args(argv[:separator])
    arguments = argv[separator + 1:]
    args.platforms = ["linux", "windows"] if args.platform == "all" else [args.platform]
    if getattr(args, "interactive", False) and args.platform == "linux":
        parser.error("--interactive requires a Windows guest")
    if getattr(args, "elevated", False) and not getattr(args, "interactive", False):
        parser.error("--elevated requires --interactive and a Windows guest")
    args.command = None
    if args.action == "run":
        if not arguments:
            parser.error("run requires -- <project-script> [arguments]")
        try:
            args.command = script_command(arguments, args.platforms)
        except ValueError as error:
            parser.error(str(error))
    elif separator != len(argv):
        parser.error("-- <project-script> [arguments] is only accepted by run")
    return args


def main():
    args = parse_arguments(sys.argv[1:])
    if args.action == "probe":
        return probe_platforms(args.platforms)
    return run_platforms(args.platforms, getattr(args, "preset", "dev"), args.upstream,
                         args.command, not args.skip_runtime, args.transport, args.interactive,
                         args.elevated)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except subprocess.CalledProcessError as error:
        if error.stderr:
            print(error.stderr.strip(), file=sys.stderr)
        print(f"Runner command failed (exit {error.returncode}); see guest logs and {STATE}",
              file=sys.stderr)
        raise SystemExit(error.returncode)
