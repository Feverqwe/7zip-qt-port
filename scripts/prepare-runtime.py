#!/usr/bin/env python3
"""Reuse verified official runtime artifacts, rebuilding only on cache misses."""

import argparse
import hashlib
import json
import os
import sys
sys.dont_write_bytecode = True
from pathlib import Path
import platform
import shlex
import subprocess
import time
from lib.runtime_layout import codec_runtime, runtime_root, sanitizer_startup, sfx_runtime
from lib.build_paths import directory as workspace_directory

ROOT = Path(__file__).resolve().parents[1]
BUILDERS = ("build-codec-runtime.sh", "build-codec-runtime.ps1", "build-sfx.sh", "build-sfx.ps1",
            "upstream_baseline.py", "build-native-dependencies.py")
MODULE_BUILD_INPUTS = ("tools/native-dependencies/CMakeLists.txt", "cmake/CxxStandard.cmake",
                       "cmake/ModuleToolchain.cmake",
                       "cmake/StaticAnalysis.cmake", ".clang-tidy",
                       "scripts/lib/msvc-toolchain.ps1",
                       "scripts/lib/module_cache_launcher.py", "scripts/lib/clang_tidy_launcher.py")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fingerprint(root, sanitizer_runtime=True):
    windows = platform.system() == "Windows"
    compiler = shlex.split(os.environ.get("CXX", "cl" if windows else "c++"))
    version = subprocess.run([*compiler, "/Bv" if windows else "--version"],
                             capture_output=True).stdout
    # MSVC emits some version details on stderr and exits nonzero without input.
    if windows:
        probe = subprocess.run([*compiler], capture_output=True)
        version += probe.stdout + probe.stderr
    baseline = json.loads((root / "docs/upstream-baseline.json").read_text())
    exact_overrides = sorted(set(baseline.get("overrides", {})) & set(baseline.get("exact", [])))
    return {"baseline": {key: baseline.get(key) for key in
                         ("repository", "revision", "overrides", "runtime_format_sources")},
            "runtime_sources": {path: sha(root / path) for path in
                                baseline.get("runtime_adapted", []) + exact_overrides +
                                baseline.get("runtime_dependencies", [])},
            "builders": {name: sha(root / "scripts" / name) for name in BUILDERS},
            "module_build": {name: sha(root / name) for name in MODULE_BUILD_INPUTS},
            "cache_version": sha(Path(__file__)),
            "system": platform.system(), "architecture": platform.machine(),
            "sanitizer_runtime": bool(sanitizer_runtime),
            "compiler": compiler, "compiler_version": hashlib.sha256(version).hexdigest(),
            "environment": {key: os.environ.get(key, "") for key in
                            ("CC", "CXX", "CFLAGS", "CXXFLAGS", "LDFLAGS", "SDKROOT",
                             "CPLUS_INCLUDE_PATH", "VCToolsVersion", "WindowsSDKVersion",
                             "VSCMD_ARG_TGT_ARCH", "SEVENZIP_ENABLE_CLANG_TIDY")}}


def artifact_paths(windows, runtime_prefix="codec-runtime", sfx_prefix="sfx"):
    runtime = ["7z.exe", "7z.dll"] if windows else ["7z", "7z.so"]
    sfx = ["7z.sfx", "7z.sfx.json", "7z-LICENSE.txt"] if windows else [
        "7zCon.sfx", "7zCon.sfx.json", "7zCon-LICENSE.txt"]
    return [runtime_prefix + "/" + name for name in runtime + ["runtime.json", "LICENSE.txt"]] + [
        sfx_prefix + "/" + name for name in sfx]


def cache_valid(build, record, key, paths):
    if record.get("fingerprint") != key or set(record.get("artifacts", {})) != set(paths):
        return False
    return all(not (build / name).is_symlink() and (build / name).is_file()
               and sha(build / name) == record["artifacts"][name] for name in paths)


def prepare_upstream(root, upstream, baseline):
    revision = baseline["revision"]

    def contains_revision(directory):
        return directory.exists() and subprocess.run(
            ["git", "-C", str(directory), "cat-file", "-e", revision + "^{commit}"],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0

    if contains_revision(upstream):
        return upstream
    runner_store = upstream == root / "upstream"
    store = upstream if runner_store else workspace_directory(root, "cache") / "upstream.git"
    if contains_revision(store):
        return store
    if not store.exists():
        store.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(["git", "init", "-q", "--bare", str(store)], check=True)
    source = str(root / "upstream.bundle") if runner_store else baseline["repository"]
    ref = "refs/heads/runner-baseline" if runner_store else revision
    print(f"Preparing pinned upstream {revision} in {store}", flush=True)
    subprocess.run(["git", "-C", str(store), "fetch", source, ref], check=True)
    if not contains_revision(store):
        raise ValueError(f"Upstream source {source} did not provide pinned commit {revision}")
    return store


def prepare(root, upstream, codec_only=False, sanitizer_runtime=True, preset="dev"):
    root = root.resolve()
    upstream = (root / upstream).resolve() if not upstream.is_absolute() else upstream.resolve()
    started = time.monotonic()
    build = runtime_root(root, preset)
    directory = codec_runtime(root, preset)
    sfx_directory = sfx_runtime(root, preset)
    directory.parent.mkdir(parents=True, exist_ok=True)
    stamp = directory.parent / ("codec-runtime-cache.json" if codec_only else "runtime-cache.json")
    key = fingerprint(root, sanitizer_runtime)
    runtime_prefix = directory.relative_to(build).as_posix()
    paths = artifact_paths(platform.system() == "Windows", runtime_prefix,
                           sfx_directory.relative_to(build).as_posix())
    if codec_only:
        paths = [path for path in paths if path.startswith(runtime_prefix + "/")]
    try:
        record = json.loads(stamp.read_text())
    except (OSError, ValueError):
        record = {}
    if cache_valid(build, record, key, paths):
        print(f"Runtime cache: hit ({time.monotonic() - started:.2f}s)", flush=True)
        return
    print("Runtime cache: miss; building pinned engine" + ("" if codec_only else " and SFX"), flush=True)
    if stamp.is_symlink():
        raise ValueError("Refusing symlink runtime cache record")
    stamp.unlink(missing_ok=True)
    upstream = prepare_upstream(root, upstream, key["baseline"])
    if platform.system() == "Windows":
        for name in (("build-codec-runtime.ps1",) if codec_only else
                     ("build-codec-runtime.ps1", "build-sfx.ps1")):
            environment = dict(os.environ)
            environment["SEVENZIP_RUNTIME_SANITIZERS"] = str(int(
                sanitizer_runtime and name == "build-codec-runtime.ps1"))
            command = ["powershell.exe", "-NoProfile", "-ExecutionPolicy", "RemoteSigned",
                       "-File", str(root / "scripts" / name), "-UpstreamCheckout", str(upstream),
                       "-Architecture", os.environ.get("VSCMD_ARG_TGT_ARCH", "x64")]
            command += ["-OutputDirectory", str(directory if name == "build-codec-runtime.ps1"
                                                else sfx_directory)]
            subprocess.run(command,
                           cwd=root, env=environment, check=True)
    else:
        for name in (("build-codec-runtime.sh",) if codec_only else
                     ("build-codec-runtime.sh", "build-sfx.sh")):
            environment = dict(os.environ)
            environment["SEVENZIP_RUNTIME_TOOLCHAIN"] = hashlib.sha256(json.dumps(
                {field: key.get(field) for field in
                 ("compiler", "compiler_version", "environment")}, sort_keys=True).encode()).hexdigest()
            environment["SEVENZIP_RUNTIME_SANITIZERS"] = str(int(
                sanitizer_runtime and name == "build-codec-runtime.sh"))
            command = ["bash", str(root / "scripts" / name), str(upstream)]
            command.append(str(directory if name == "build-codec-runtime.sh" else sfx_directory))
            subprocess.run(command,
                           cwd=root, env=environment, check=True)
    record = {"fingerprint": key, "artifacts": {name: sha(build / name) for name in paths}}
    temporary = stamp.with_suffix(".tmp")
    temporary.write_text(json.dumps(record, indent=2) + "\n")
    temporary.replace(stamp)
    print(f"Runtime prepared ({time.monotonic() - started:.2f}s)", flush=True)


def main():
    from lib.toolchain import apply_environment, harness_root
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", type=Path, default=harness_root(ROOT).parent / "7zip-upstream")
    parser.add_argument("--codec-only", action="store_true", help="Prepare engine variants without SFX")
    parser.add_argument("--preset", default="dev", help="Use the build preset's shared toolchain")
    parser.add_argument("--sanitizer-runtime", choices=["auto", "on", "off"], default="auto",
                        help="Link the Unix console host with sanitizer runtimes for instrumented plugins")
    args = parser.parse_args()
    apply_environment(ROOT, args.preset, require_qt=False)
    startup = (sanitizer_startup(ROOT, args.preset) if args.sanitizer_runtime == "auto"
               else args.sanitizer_runtime == "on")
    prepare(ROOT, args.upstream, args.codec_only, startup, args.preset)


if __name__ == "__main__":
    main()
