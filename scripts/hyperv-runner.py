#!/usr/bin/env python3
"""Run the current worktree's gates in the shared, SSH-accessible Hyper-V guests."""

import argparse
import base64
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

ROOT = Path(__file__).resolve().parents[1]
STATE = ROOT / "build/runners/hyperv"
KEY = Path.home() / ".ssh/id_ed25519"
READY = Path.home() / ".codex/skills/runners/scripts/hyperv_ready.py"


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


def remote(record, script):
    windows = record["name"] == "runner-windows"
    return invoke([*record["ssh_argv"],
                   powershell("& ([ScriptBlock]::Create([Console]::In.ReadToEnd()))")
                   if windows else "bash -se"], input=script)


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
    entries.add(b"scripts/hyperv-runner.py")
    revision = json.loads((ROOT / "docs/upstream-baseline.json").read_text())["revision"]
    bundle = destination.parent / "upstream.bundle"
    # A bundle requires a named ref. Create it only in a disposable mirror,
    # preserving the source checkout's refs, index and files.
    with tempfile.TemporaryDirectory(dir=destination.parent) as temporary:
        mirror = Path(temporary) / "source.git"
        invoke(["git", "clone", "-q", "--bare", "--shared", str(upstream), str(mirror)])
        invoke(["git", "-C", str(mirror), "update-ref", "refs/heads/runner-baseline", revision])
        invoke(["git", "-C", str(mirror), "bundle", "create", str(bundle), "runner-baseline"])
    with tarfile.open(destination, "w:gz", dereference=False) as archive:
        for raw in sorted(entries):
            name = os.fsdecode(raw)
            if Path(name).parts[0] not in roots and name not in top_files:
                continue
            path = ROOT / name
            if path.exists() or path.is_symlink():
                archive.add(path, arcname=name, recursive=False)
        archive.add(bundle, arcname="upstream.bundle", recursive=False)
    return hashlib.sha256(destination.read_bytes()).hexdigest()


def check(record, platform, preset, upstream):
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%SZ")
    run_id = stamp + "-" + uuid.uuid4().hex[:8]
    local = STATE / run_id
    local.mkdir()
    archive = local / "source.tar.gz"
    digest = snapshot(archive, upstream)
    root = ("C:/runner/sevenzip-desktop/runs/" if platform == "windows" else
            "/home/runner/sevenzip-desktop/runs/") + run_id
    record = dict(record, directory=root, sha256=digest, preset=preset,
                  revision=subprocess.check_output(
                      ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                  status="running")
    run_record = local / "run.json"
    run_record.write_text(json.dumps(record, indent=2) + "\n")
    print(f"Guest: {record['name']} ({record['ip']}); sources/logs: {root}", flush=True)
    try:
        execute_check(record, platform, preset, archive, digest, root)
    except subprocess.CalledProcessError as error:
        record["status"] = "failed"
        record["exit_code"] = error.returncode
        raise
    except KeyboardInterrupt:
        record["status"] = "interrupted"
        raise
    except BaseException:
        record["status"] = "error"
        raise
    else:
        record["status"] = "passed"
    finally:
        run_record.write_text(json.dumps(record, indent=2) + "\n")


def execute_check(record, platform, preset, archive, digest, root):
    if platform == "linux":
        remote(record, f"mkdir -p {shlex.quote(root)}\n")
    else:
        remote(record, f"New-Item -ItemType Directory '{root}' | Out-Null")
    invoke(["scp", *guest_options(record), str(archive),
            f"runner@{record['ip']}:{root}/source.tar.gz"])
    if platform == "linux":
        script = f"""cd {shlex.quote(root)}
printf '%s  source.tar.gz\\n' {digest} | sha256sum -c -
tar xzf source.tar.gz
git init -q
printf '/source.tar.gz\\n/check.log\\n/upstream.bundle\\n/upstream/\\n' >> .git/info/exclude
git add -A
git -c user.name='Runner snapshot' -c user.email='runner@example.invalid' commit -qm 'Runner snapshot'
export CMAKE_BUILD_PARALLEL_LEVEL=4 SEVENZIP_SCRIPT_OUTPUT=compact
export PATH="/home/runner/sevenzip-desktop/tools/formatter/bin:$PATH"
# The VM has no accelerated GPU. Serialize native display fixtures to avoid
# competing software compositors and foreground-input timing failures.
export LIBGL_ALWAYS_SOFTWARE=1 SEVENZIP_CTEST_JOBS=1
set +e
(
set -e
git clone -q -b runner-baseline upstream.bundle upstream
./scripts/build-codec-runtime.sh "$PWD/upstream"
./scripts/build-sfx.sh "$PWD/upstream"
make -s -C upstream/CPP/7zip/Bundles/Alone2 -f makefile.gcc -j4
cp upstream/CPP/7zip/Bundles/Alone2/_o/7zz build/codec-runtime/7zz
export PATH="$PWD/build/codec-runtime:$PATH"
export SEVENZIP_CODEC_MODULE_DIR="$PWD/build/codec-runtime"
./scripts/check.sh
"""
        if preset == "release":
            script += "./scripts/build.sh release\n./scripts/ctest.sh --preset release --output-on-failure\n"
        script += ") > check.log 2>&1\ncode=$?\ntail -n 80 check.log\nexit \"$code\"\n"
        script = "set -euo pipefail\n" + script
    else:
        kit = "C:/runner/htopqt"
        commands = ['@echo off', f'call "{kit}/VS/VC/Auxiliary/Build/vcvars64.bat"',
                    'if errorlevel 1 exit /b %errorlevel%',
                    f'set "PATH={root}/build/codec-runtime;C:/runner/sevenzip-desktop/tools/formatter/Scripts;{kit}/Python;{kit}/Python/Scripts;{kit}/Git/bin;{kit}/Qt/6.8.3/msvc2022_64/bin;%PATH%"',
                    f'set "CMAKE_PREFIX_PATH={kit}/Qt/6.8.3/msvc2022_64"',
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
                    'echo /source.tar.gz>>.git/info/exclude',
                    'echo /upstream.bundle>>.git/info/exclude',
                    'echo /upstream/>>.git/info/exclude',
                    'git add -A', 'if errorlevel 1 exit /b %errorlevel%',
                    'git -c user.name="Runner snapshot" -c user.email="runner@example.invalid" commit -qm "Runner snapshot"',
                    'if errorlevel 1 exit /b %errorlevel%',
                    'git clone -q -b runner-baseline upstream.bundle upstream',
                    'if errorlevel 1 exit /b %errorlevel%',
                    'powershell.exe -NoProfile -ExecutionPolicy RemoteSigned -File scripts/build-codec-runtime.ps1 -UpstreamCheckout upstream -Architecture x64',
                    'if errorlevel 1 exit /b %errorlevel%',
                    'powershell.exe -NoProfile -ExecutionPolicy RemoteSigned -File scripts/build-sfx.ps1 -UpstreamCheckout upstream -Architecture x64',
                    'if errorlevel 1 exit /b %errorlevel%',
                    'bash scripts/check.sh', 'if errorlevel 1 exit /b %errorlevel%']
        if preset == "release":
            commands += ['bash scripts/build.sh release', 'if errorlevel 1 exit /b %errorlevel%',
                         'bash scripts/ctest.sh --preset release --output-on-failure',
                         'if errorlevel 1 exit /b %errorlevel%']
        commands += ['exit /b 0']
        batch = "\r\n".join(commands) + "\r\n"
        encoded = base64.b64encode(batch.encode()).decode()
        script = f"""Set-Location '{root}'
if ((Get-FileHash source.tar.gz -Algorithm SHA256).Hash.ToLower() -ne '{digest}') {{ throw 'Snapshot hash mismatch' }}
tar.exe -xzf source.tar.gz
if ($LASTEXITCODE -ne 0) {{ throw 'Extraction failed' }}
[IO.File]::WriteAllBytes('{root}/check.cmd', [Convert]::FromBase64String('{encoded}'))
& cmd.exe /d /c 'check.cmd > check.log 2>&1'
$code=$LASTEXITCODE
Get-Content check.log -Tail 80
if ($code -ne 0) {{ exit $code }}
"""
    remote(record, script)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["probe", "check"])
    parser.add_argument("platform", choices=["linux", "windows"])
    parser.add_argument("--preset", choices=["dev", "release"], default="dev")
    parser.add_argument("--upstream", type=Path, default=ROOT.parent / "7zip-upstream",
                        help="read-only checkout containing the pinned engine revision")
    args = parser.parse_args()
    record = discover(args.platform)
    if args.action == "check":
        check(record, args.platform, args.preset, args.upstream)
    elif args.platform == "linux":
        remote(record, "uname -a\ncmake --version\ngcc --version\nqmake6 -query QT_VERSION\n/home/runner/sevenzip-desktop/tools/formatter/bin/clang-format --version\n")
    else:
        remote(record, "Get-CimInstance Win32_OperatingSystem | Select Caption,Version,OSArchitecture | ConvertTo-Json; "
               "& C:/runner/htopqt/Python/Scripts/cmake.exe --version; "
               "& C:/runner/htopqt/Qt/6.8.3/msvc2022_64/bin/qmake.exe -query QT_VERSION; "
               "if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }; "
               "& C:/runner/sevenzip-desktop/tools/formatter/Scripts/clang-format.exe --version; "
               "if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }")


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        if error.stderr:
            print(error.stderr.strip(), file=sys.stderr)
        print(f"Runner command failed (exit {error.returncode}); see guest logs and {STATE}",
              file=sys.stderr)
        raise SystemExit(error.returncode)
