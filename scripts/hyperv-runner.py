#!/usr/bin/env python3
"""Sync current source and run checks in one persistent worktree per Hyper-V guest."""

import argparse
import base64
from concurrent.futures import ThreadPoolExecutor
import datetime
import json
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
import time
import uuid

from hyperv_interactive import interactive_command
from hyperv_source import stage_tree
from hyperv_transport import collect_logs, sync_command
from hyperv_workspace import load_workspaces, mount_guard

ROOT = Path(__file__).resolve().parents[1]
STATE = ROOT / "build/runners/hyperv"
READY = Path.home() / ".codex/skills/runners/scripts/hyperv_ready.py"
QT_VERSION = "6.12.0"
QT_KITS = {"linux": f"/home/runner/sevenzip-desktop/tools/Qt/{QT_VERSION}/gcc_64",
           "windows": f"C:/runner/sevenzip-desktop/tools/Qt/{QT_VERSION}/msvc2022_64"}
WINDOWS_PYTHON = "C:/runner/htopqt/Python/python.exe"


def invoke(args, **kwargs):
    return subprocess.run(args, check=True, text=True, **kwargs)


def powershell(script):
    wrapped = ("$ProgressPreference='SilentlyContinue'; $ErrorActionPreference='Stop'; "
               "try {\n" + script + "\n} catch { Write-Error $_; exit 1 }")
    return "powershell.exe -NoProfile -NonInteractive -EncodedCommand " + base64.b64encode(
        wrapped.encode("utf-16le")).decode()


def discover(platform):
    result = invoke([sys.executable, str(READY), platform, "--start"], capture_output=True)
    record = json.loads(result.stdout)
    record.update(name=record["vm"], id=record["vm_id"])
    STATE.mkdir(parents=True, exist_ok=True)
    (STATE / f"{platform}.json").write_text(json.dumps(record, indent=2) + "\n")
    return record


def remote(record, script, **kwargs):
    windows = record["name"] == "runner-windows"
    return invoke([*record["ssh_argv"], powershell(
        "& ([ScriptBlock]::Create([Console]::In.ReadToEnd()))") if windows else "bash -se"],
        input=script, **kwargs)


def lock_command(record, root, token, action, **kwargs):
    code = (ROOT / "scripts/hyperv_lock.py").read_text()
    if action == "recover":
        code += f"\nroot=Path({root!r})\nrecord=json.loads((root.with_name(root.name+'.lock')/'owner.json').read_text())\n"
        code += "operate(root, record['token'], 'release-completed')\n"
    else:
        code += f"\noperate({root!r}, {token!r}, {action!r})\n"
    encoded = base64.b64encode(code.encode()).decode()
    statement = f"import base64; exec(base64.b64decode('{encoded}'))"
    if record["name"] == "runner-windows":
        script = f"& '{WINDOWS_PYTHON}' -c \"{statement}\"\nif ($LASTEXITCODE -ne 0) {{ exit $LASTEXITCODE }}\n"
    else:
        script = "python3 -c " + shlex.quote(statement) + "\n"
    return remote(record, script, **kwargs)


def command_payload(preset, command):
    commands = [command] if command is not None else [["bash", "scripts/check.sh"]]
    if command is None and preset == "release":
        commands += [["bash", "scripts/build.sh", "release"],
                     ["bash", "scripts/ctest.sh", "--preset", "release", "--output-on-failure"]]
    return "set -euo pipefail\n" + "".join(shlex.join(item) + "\n" for item in commands)


def execute_check(record, platform, preset, digest, root, token, command=None,
                  prepare_runtime=True, output=None, interactive=False, elevated=False):
    import os
    include_slow = os.environ.get("SEVENZIP_INCLUDE_SLOW_TESTS", "0")
    if include_slow not in ("0", "1"):
        raise ValueError("SEVENZIP_INCLUDE_SLOW_TESTS must be 0 or 1")
    payload = command_payload(preset, command)
    excludes = ["build/", "runner-tmp/", "upstream/", "upstream.bundle", ".runner-*",
                "check.log", "check.cmd", "runner-command.sh", "interactive-*.txt"]
    if platform == "linux":
        script = f"""set -euo pipefail
cd {shlex.quote(root)}
mkdir -p runner-tmp
export TMPDIR="$PWD/runner-tmp" TMP="$PWD/runner-tmp" TEMP="$PWD/runner-tmp"
export QT_QPA_PLATFORM="${{QT_QPA_PLATFORM:-offscreen}}"
export CMAKE_BUILD_PARALLEL_LEVEL=4 SEVENZIP_SCRIPT_OUTPUT=compact
export SEVENZIP_COMPACT_TEST_BINARIES=ON SEVENZIP_INCLUDE_SLOW_TESTS={include_slow}
export CMAKE_PREFIX_PATH="{QT_KITS['linux']}"
export PATH="{QT_KITS['linux']}/bin:/home/runner/sevenzip-desktop/tools/formatter/bin:$PATH"
export LIBGL_ALWAYS_SOFTWARE=1 SEVENZIP_CTEST_JOBS=1
export PYTHONUTF8=1 PYTHONIOENCODING=utf-8
export SEVENZIP_RUN_TOKEN={token}
export CCACHE_DIR=/home/runner/sevenzip-desktop/cache/ccache
export CCACHE_MAXSIZE=2G
set +e
(
set -e
df -Pk .
python3 scripts/hyperv_source.py {digest}
git init -q
printf '%s\\n' {' '.join(shlex.quote('/' + name) for name in excludes)} > .git/info/exclude
git add -A
git diff --cached --quiet || git -c user.name='Runner worktree' -c user.email='runner@example.invalid' commit -qm 'Current source'
"""
        if prepare_runtime:
            script += 'python3 scripts/prepare-runtime.py --upstream "$PWD/upstream"\nexport SEVENZIP_RUNTIME_PREPARED=1\n'
        script += ('export PATH="$PWD/build/codec-runtime:$PATH"\n'
                   'export SEVENZIP_CODEC_MODULE_DIR="$PWD/build/codec-runtime"\n' + payload)
        script += f") > check.log 2>&1\ncode=$?\nprintf '%s\\n' '{token}' > .runner-complete\ntail -n 80 check.log\nexit \"$code\"\n"
    else:
        kit = "C:/runner/htopqt"
        commands = ['@echo off', *[f'set "{key}={root}/runner-tmp"' for key in ("TEMP", "TMP", "TMPDIR")],
                    f'call "{kit}/VS/VC/Auxiliary/Build/vcvars64.bat"', 'if errorlevel 1 exit /b %errorlevel%',
                    f'set "PATH={root}/build/codec-runtime;C:/runner/sevenzip-desktop/tools/ccache;C:/runner/sevenzip-desktop/tools/formatter/Scripts;{kit}/Python;{kit}/Python/Scripts;{kit}/Git/bin;{QT_KITS["windows"]}/bin;%PATH%"',
                    'set "CCACHE_DIR=C:/runner/sevenzip-desktop/cache/ccache"', 'set "CCACHE_MAXSIZE=2G"',
                    f'set "CMAKE_PREFIX_PATH={QT_KITS["windows"]}"', 'set "CC=cl.exe"', 'set "CXX=cl.exe"',
                    'set "CMAKE_BUILD_PARALLEL_LEVEL=4"', 'set "SEVENZIP_COMPACT_TEST_BINARIES=ON"',
                    f'set "SEVENZIP_INCLUDE_SLOW_TESTS={include_slow}"', 'set "SEVENZIP_SCRIPT_OUTPUT=compact"',
                    'set "SEVENZIP_CTEST_JOBS=1"', f'set "SEVENZIP_CODEC_MODULE_DIR={root}/build/codec-runtime"',
                    'set "PYTHONUTF8=1"', 'set "PYTHONIOENCODING=utf-8"',
                    f'set "SEVENZIP_RUN_TOKEN={token}"',
                    f'cd /d "{root}"', f'python scripts/hyperv_source.py {digest}',
                    'if errorlevel 1 exit /b %errorlevel%', 'git init -q', 'if errorlevel 1 exit /b %errorlevel%',
                    'git config core.autocrlf false', 'if errorlevel 1 exit /b %errorlevel%']
        commands += [f'{">" if i == 0 else ">>"}.git/info/exclude echo /{name}' for i, name in enumerate(excludes)]
        commands += ['git add -A', 'if errorlevel 1 exit /b %errorlevel%',
                     'git diff --cached --quiet || git -c user.name="Runner worktree" -c user.email="runner@example.invalid" commit -qm "Current source"',
                     'if errorlevel 1 exit /b %errorlevel%']
        if prepare_runtime:
            commands += ['python scripts/prepare-runtime.py --upstream upstream',
                         'if errorlevel 1 exit /b %errorlevel%', 'set "SEVENZIP_RUNTIME_PREPARED=1"']
        commands += ['bash runner-command.sh', 'exit /b %errorlevel%']
        batch = base64.b64encode(("\r\n".join(commands) + "\r\n").encode()).decode()
        encoded_payload = base64.b64encode(payload.encode()).decode()
        launch = interactive_command(root, elevated) if interactive else (
            "& cmd.exe /d /c 'check.cmd > check.log 2>&1'\n$code=$LASTEXITCODE\n")
        script = f"""Set-Location '{root}'
New-Item -ItemType Directory '{root}/runner-tmp' -Force | Out-Null
$env:TEMP=$env:TMP=$env:TMPDIR='{root}/runner-tmp'
[IO.File]::WriteAllBytes('{root}/check.cmd', [Convert]::FromBase64String('{batch}'))
[IO.File]::WriteAllBytes('{root}/runner-command.sh', [Convert]::FromBase64String('{encoded_payload}'))
{launch}
[IO.File]::WriteAllText('{root}/.runner-complete', '{token}')
Get-Content check.log -Tail 80
if ($code -ne 0) {{ exit $code }}
"""
    return remote(record, script, **({"stdout": output, "stderr": subprocess.STDOUT} if output else {}))


def script_command(arguments, platforms):
    path = Path(arguments[0])
    if path.is_absolute() or ".." in path.parts or not (ROOT / path).is_file():
        raise ValueError("Script must be an existing project-relative path")
    from hyperv_source import source_name
    if not source_name(path.as_posix()):
        raise ValueError("Script is outside synchronized source directories")
    ignored = subprocess.run(["git", "check-ignore", "-q", "--", str(path)], cwd=ROOT)
    if ignored.returncode == 0:
        raise ValueError("Ignored scripts are not synchronized")
    interpreters = {".sh": ["bash"], ".py": ["python"],
                    ".ps1": ["powershell.exe", "-NoProfile", "-File"]}
    if path.suffix not in interpreters or (path.suffix == ".ps1" and platforms != ["windows"]):
        raise ValueError("Use .sh/.py, or .ps1 for Windows only")
    return [*interpreters[path.suffix], path.as_posix(), *arguments[1:]]


def run_platform(platform, local, source, digest, preset, command, prepare_runtime,
                 interactive=False, elevated=False, workspace=None):
    directory = local / platform
    directory.mkdir()
    workspace = workspace or load_workspaces(STATE)[platform]
    root = workspace["root"] + "/current"
    token = local.name
    result = {"platform": platform, "sha256": digest, "status": "running", "transport": "rsync",
              "directory": root, "interactive": interactive and platform == "windows",
              "elevated": elevated and platform == "windows", "seconds": {}}
    acquired = launched = interrupted_sync = False
    print(f"[{platform}] Persistent worktree: {root}; log: {directory / 'transport.log'}", flush=True)
    with (directory / "transport.log").open("w") as output:
        transport = {"stdout": output, "stderr": subprocess.STDOUT}
        try:
            started = time.monotonic()
            record = discover(platform)
            result.update(record)
            if "filesystem_uuid" in workspace:
                remote(record, mount_guard(workspace), **transport)
            lock_command(record, root, token, "acquire", **transport)
            acquired = True
            result["seconds"]["ready_and_lock"] = round(time.monotonic() - started, 3)
            started = time.monotonic()
            invoke(sync_command(record, platform, source, root), **transport)
            result["seconds"]["sync"] = round(time.monotonic() - started, 3)
            resolved = list(command) if command else None
            if resolved and resolved[0] == "python" and platform == "linux":
                resolved[0] = "python3"
            launched = True
            started = time.monotonic()
            try:
                execute_check(record, platform, preset, digest, root, token, resolved, prepare_runtime,
                              output, result["interactive"], result["elevated"])
            finally:
                result["seconds"]["execute"] = round(time.monotonic() - started, 3)
            result.update(status="passed", exit_code=0)
        except subprocess.CalledProcessError as error:
            result.update(status="failed", exit_code=error.returncode, error=str(error))
            if error.stderr:
                result["error"] += ": " + error.stderr.strip()
                output.write(error.stderr)
        except KeyboardInterrupt:
            interrupted_sync = not launched
            result.update(status="interrupted", exit_code=130)
            raise
        except Exception as error:
            result.update(status="error", exit_code=1, error=str(error))
        finally:
            if acquired:
                try:
                    if launched:
                        result.update(collect_logs(record, root, directory, command, token, invoke, output))
                except (subprocess.CalledProcessError, OSError, ValueError) as error:
                    result["log_error"] = str(error)
                try:
                    if interrupted_sync:
                        raise ValueError("Sync interrupted; lock retained until its remote rsync process is checked")
                    # Hold the lock through log collection, so a new invocation
                    # cannot replace the evidence. Unknown children keep it.
                    lock_command(record, root, token, "release-completed" if launched else "release", **transport)
                except Exception as error:
                    result["lock_retained"] = str(error)
                    if result["status"] == "passed":
                        result.update(status="error", exit_code=1)
            (directory / "run.json").write_text(json.dumps(result, indent=2) + "\n")
    print(f"[{platform}] {result['status']}; timing: {result['seconds']}; logs: {directory}", flush=True)
    if result["status"] != "passed" or result.get("lock_retained"):
        log = directory / "check.log"
        if not log.exists():
            log = directory / "transport.log"
        for line in log.read_text(errors="replace").splitlines()[-20:]:
            print(f"[{platform}] {line}", flush=True)
    return result


def run_platforms(platforms, preset, upstream, command=None, prepare_runtime=True,
                  interactive=False, elevated=False):
    if elevated and (not interactive or "windows" not in platforms):
        raise ValueError("Elevated fixtures require an interactive Windows guest")
    workspaces = load_workspaces(STATE)
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    local = STATE / (stamp + "-" + uuid.uuid4().hex[:8])
    local.mkdir(parents=True)
    summary = {"run_id": local.name, "command": command, "preset": preset, "prepare_runtime": prepare_runtime,
               "transport": "rsync", "interactive": interactive, "elevated": elevated,
               "revision": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
               "status": "running", "platforms": []}
    report = local / "run.json"
    try:
        with tempfile.TemporaryDirectory(prefix="source-", dir=STATE) as temporary:
            started = time.monotonic()
            source = Path(temporary)
            summary["sha256"] = stage_tree(ROOT, upstream, source, STATE / "cache")
            summary["source_seconds"] = round(time.monotonic() - started, 3)
            report.write_text(json.dumps(summary, indent=2) + "\n")
            with ThreadPoolExecutor(max_workers=len(platforms)) as executor:
                futures = [executor.submit(run_platform, platform, local, source, summary["sha256"],
                                           preset, command, prepare_runtime, interactive, elevated,
                                           workspaces[platform]) for platform in platforms]
                summary["platforms"] = [future.result() for future in futures]
            summary["status"] = "passed" if all(p["status"] == "passed" for p in summary["platforms"]) else "failed"
    except BaseException as error:
        summary.update(status="interrupted" if isinstance(error, KeyboardInterrupt) else "error", error=str(error))
        raise
    finally:
        report.write_text(json.dumps(summary, indent=2) + "\n")
        print(f"Report: {report}", flush=True)
    return 0 if summary["status"] == "passed" else 1


def probe_platform(platform):
    record = discover(platform)
    if platform == "linux":
        script = ("set -euo pipefail\nuname -a\ncmake --version\ngcc --version\n"
                  "ccache --version\n"
                  f"{QT_KITS['linux']}/bin/qmake -query QT_VERSION\n"
                  "/home/runner/sevenzip-desktop/tools/formatter/bin/clang-format --version\n")
    else:
        script = "Get-CimInstance Win32_OperatingSystem | Select Caption,Version,OSArchitecture | ConvertTo-Json; "
        for tool in ["C:/runner/htopqt/Python/Scripts/cmake.exe --version",
                     "C:/runner/sevenzip-desktop/tools/ccache/ccache.exe --version",
                     f"{QT_KITS['windows']}/bin/qmake.exe -query QT_VERSION",
                     "C:/runner/sevenzip-desktop/tools/formatter/Scripts/clang-format.exe --version"]:
            script += f"& {tool}; if ($LASTEXITCODE -ne 0) {{ exit $LASTEXITCODE }}; "
    return remote(record, script, capture_output=True)


def probe_platforms(platforms):
    failed = False
    with ThreadPoolExecutor(max_workers=len(platforms)) as executor:
        futures = [(p, executor.submit(probe_platform, p)) for p in platforms]
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
    for action in ("probe", "check", "run", "status", "recover"):
        subparser = actions.add_parser(action)
        subparser.add_argument("platform", choices=["linux", "windows", "all"])
        if action in ("check", "run"):
            subparser.add_argument("--interactive", action="store_true")
            subparser.add_argument("--elevated", action="store_true")
            subparser.add_argument("--transport", choices=["rsync"], default="rsync")
            subparser.add_argument("--upstream", type=Path, default=ROOT.parent / "7zip-upstream")
            subparser.add_argument("--skip-runtime", action="store_true")
        if action == "check":
            subparser.add_argument("--preset", choices=["dev", "release"], default="dev")
        if action == "run":
            subparser.epilog = "Use -- <project-script> [arguments] after runner options; new nonignored scripts are included."
    separator = argv.index("--") if "--" in argv else len(argv)
    args = parser.parse_args(argv[:separator])
    args.platforms = ["linux", "windows"] if args.platform == "all" else [args.platform]
    if getattr(args, "interactive", False) and args.platform == "linux":
        parser.error("--interactive requires Windows")
    if getattr(args, "elevated", False) and not getattr(args, "interactive", False):
        parser.error("--elevated requires --interactive")
    args.command = None
    if args.action == "run":
        if separator == len(argv) or not argv[separator + 1:]:
            parser.error("run requires -- <project-script> [arguments]")
        try:
            args.command = script_command(argv[separator + 1:], args.platforms)
        except ValueError as error:
            parser.error(str(error))
    elif separator != len(argv):
        parser.error("-- <project-script> is only accepted by run")
    return args


def main():
    args = parse_arguments(sys.argv[1:])
    if args.action == "probe":
        return probe_platforms(args.platforms)
    if args.action in ("status", "recover"):
        for platform in args.platforms:
            record = discover(platform)
            workspace = load_workspaces(STATE)[platform]
            if "filesystem_uuid" in workspace:
                remote(record, mount_guard(workspace))
            lock_command(record, workspace["root"] + "/current", "", args.action)
        return 0
    return run_platforms(args.platforms, getattr(args, "preset", "dev"), args.upstream,
                         args.command, not args.skip_runtime, args.interactive, args.elevated)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        print(f"Runner failed: {error}; see {STATE}", file=sys.stderr)
        raise SystemExit(error.returncode if isinstance(error, subprocess.CalledProcessError) else 1)
