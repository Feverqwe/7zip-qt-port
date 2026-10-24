#!/usr/bin/env python3
"""Reuse verified official runtime artifacts, rebuilding only on cache misses."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
BUILDERS = ("build-codec-runtime.sh", "build-codec-runtime.ps1", "build-sfx.sh", "build-sfx.ps1",
            "upstream_baseline.py")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fingerprint(root):
    windows = platform.system() == "Windows"
    compiler = shlex.split(os.environ.get("CXX", "cl" if windows else "c++"))
    version = subprocess.run([*compiler, "/Bv" if windows else "--version"],
                             capture_output=True).stdout
    # MSVC emits some version details on stderr and exits nonzero without input.
    if windows:
        probe = subprocess.run([*compiler], capture_output=True)
        version += probe.stdout + probe.stderr
    baseline = json.loads((root / "docs/upstream-baseline.json").read_text())
    return {"baseline": {key: baseline.get(key) for key in ("repository", "revision")},
            "runtime_sources": {path: sha(root / path) for path in baseline.get("runtime_adapted", []) +
                                baseline.get("runtime_dependencies", [])},
            "builders": {name: sha(root / "scripts" / name) for name in BUILDERS},
            "cache_version": sha(Path(__file__)),
            "system": platform.system(), "architecture": platform.machine(),
            "compiler": compiler, "compiler_version": hashlib.sha256(version).hexdigest(),
            "environment": {key: os.environ.get(key, "") for key in
                            ("CC", "CXX", "CFLAGS", "CXXFLAGS", "LDFLAGS", "SDKROOT",
                             "CPLUS_INCLUDE_PATH", "VCToolsVersion", "WindowsSDKVersion",
                             "VSCMD_ARG_TGT_ARCH")}}


def artifact_paths(windows):
    runtime = ["7z.exe", "7z.dll"] if windows else ["7z", "7z.so"]
    sfx = ["7z.sfx", "7z.sfx.json", "7z-LICENSE.txt"] if windows else [
        "7zCon.sfx", "7zCon.sfx.json", "7zCon-LICENSE.txt"]
    return ["codec-runtime/" + name for name in runtime + ["runtime.json", "LICENSE.txt"]] + [
        "sfx/" + name for name in sfx]


def cache_valid(build, record, key, paths):
    if record.get("fingerprint") != key or set(record.get("artifacts", {})) != set(paths):
        return False
    return all(not (build / name).is_symlink() and (build / name).is_file()
               and sha(build / name) == record["artifacts"][name] for name in paths)


def prepare(root, upstream, codec_only=False):
    root = root.resolve()
    upstream = (root / upstream).resolve() if not upstream.is_absolute() else upstream.resolve()
    started = time.monotonic()
    build = root / "build"
    stamp = build / ("codec-runtime-cache.json" if codec_only else "runtime-cache.json")
    key = fingerprint(root)
    paths = artifact_paths(platform.system() == "Windows")
    if codec_only:
        paths = [path for path in paths if path.startswith("codec-runtime/")]
    try:
        record = json.loads(stamp.read_text())
    except (OSError, ValueError):
        record = {}
    if cache_valid(build, record, key, paths):
        print(f"Runtime cache: hit ({time.monotonic() - started:.2f}s)", flush=True)
        return
    print("Runtime cache: miss; building pinned engine" + ("" if codec_only else " and SFX"), flush=True)
    if upstream == root / "upstream":
        # Runner-owned bare object store; builders archive the pinned commit and
        # never compile in or mutate a sibling source checkout.
        if not upstream.exists():
            subprocess.run(["git", "init", "-q", "--bare", str(upstream)], check=True)
        revision = key["baseline"]["revision"]
        found = subprocess.run(["git", "-C", str(upstream), "cat-file", "-e", revision + "^{commit}"],
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        if found.returncode:
            subprocess.run(["git", "-C", str(upstream), "fetch", str(root / "upstream.bundle"),
                            "refs/heads/runner-baseline"], check=True)
    if stamp.is_symlink():
        raise ValueError("Refusing symlink runtime cache record")
    stamp.unlink(missing_ok=True)
    if platform.system() == "Windows":
        for name in (("build-codec-runtime.ps1",) if codec_only else
                     ("build-codec-runtime.ps1", "build-sfx.ps1")):
            subprocess.run(["powershell.exe", "-NoProfile", "-ExecutionPolicy", "RemoteSigned",
                            "-File", str(root / "scripts" / name), "-UpstreamCheckout", str(upstream),
                            "-Architecture", os.environ.get("VSCMD_ARG_TGT_ARCH", "x64")],
                           cwd=root, check=True)
    else:
        for name in (("build-codec-runtime.sh",) if codec_only else
                     ("build-codec-runtime.sh", "build-sfx.sh")):
            subprocess.run(["bash", str(root / "scripts" / name), str(upstream)], cwd=root, check=True)
    record = {"fingerprint": key, "artifacts": {name: sha(build / name) for name in paths}}
    temporary = stamp.with_suffix(".tmp")
    temporary.write_text(json.dumps(record, indent=2) + "\n")
    temporary.replace(stamp)
    print(f"Runtime prepared ({time.monotonic() - started:.2f}s)", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", type=Path, default=ROOT.parent / "7zip-upstream")
    parser.add_argument("--codec-only", action="store_true", help="Prepare engine variants without SFX")
    args = parser.parse_args()
    prepare(ROOT, args.upstream, args.codec_only)


if __name__ == "__main__":
    main()
