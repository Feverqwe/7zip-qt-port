"""Materialize current source without build outputs or private state."""

from concurrent.futures import ThreadPoolExecutor
import json
import os
import re
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import tempfile

from .source_snapshot_time import version_time_ns

SOURCE_DIRS = {".agents", "cmake", "docs", "ports", "scripts", "src", "tests", "third_party", "resources",
               "translations", "tools", "triplets"}
SOURCE_FILES = {".clang-format", ".clang-tidy", ".gitattributes", ".gitignore", "AGENTS.md",
                "CMakeLists.txt", "CMakePresets.json", "LICENSE", "README.md",
                "upstream.lock.json", "vcpkg-configuration.json", "vcpkg.json",
                "windows-runtime.lock.json"}
MANIFEST = ".runner-source.json"
CONTEXT = ".runner-context.json"


def source_name(name):
    path = PurePosixPath(name)
    if path.parts[:2] == ("scripts", "config") and path.name.endswith(".local.json"):
        return False
    return (not path.is_absolute() and ".." not in path.parts and bool(path.parts)
            and (path.parts[0] in SOURCE_DIRS or name in SOURCE_FILES or name == "upstream.bundle"))


def stage_tree(root, upstream, destination, cache, source_times=False):
    for args in (["git", "diff", "--check"], ["git", "diff", "--cached", "--check"]):
        subprocess.run(args, cwd=root, check=True)
    files = subprocess.check_output(["git", "ls-files", "-z", "--cached", "--others",
                                     "--exclude-standard"], cwd=root)
    names = sorted({os.fsdecode(name) for name in files.split(b"\0") if name})
    names = [name for name in names if source_name(name)]

    def copy_source(name):
        source = root / name
        if not source.exists() and not source.is_symlink():
            return None
        link_target = None
        if source.is_symlink():
            link_target = os.readlink(source)
            if Path(link_target).is_absolute() or root.resolve() not in source.resolve().parents:
                raise ValueError(f"Source link leaves project: {name}")
        elif not source.is_file():
            raise ValueError(f"Unsupported source entry: {name}")
        target = destination / name
        target.parent.mkdir(parents=True, exist_ok=True)
        if link_target is not None:
            target.symlink_to(link_target)
        else:
            for attempt in range(3):
                before = source.stat()
                stamp = version_time_ns(source, before) if source_times else before.st_mtime_ns
                shutil.copy2(source, target)
                after = source.stat()
                latest = version_time_ns(source, after) if source_times else after.st_mtime_ns
                if (before.st_ino, before.st_size, stamp) == (after.st_ino, after.st_size, latest):
                    if source_times:
                        os.utime(target, ns=(before.st_atime_ns, stamp))
                    break
            else:
                raise ValueError(f"Source kept changing while creating snapshot: {name}")
        return name

    names_copied = []
    with ThreadPoolExecutor(max_workers=8) as pool:
        try:
            for index, name in enumerate(pool.map(copy_source, names), 1):
                if name is not None:
                    names_copied.append(name)
                if index % 256 == 0 or index == len(names):
                    print(f"Source snapshot: {index}/{len(names)} files", flush=True)
        except BaseException:
            pool.shutdown(wait=True, cancel_futures=True)
            raise
    baseline = json.loads((destination / "docs/upstream-baseline.json").read_text())
    revision = baseline["revision"]
    if not re.fullmatch(r"[0-9a-f]{40}", revision):
        raise ValueError("Invalid pinned upstream revision")
    cache.mkdir(parents=True, exist_ok=True)
    bundle = cache / f"upstream-{revision}.bundle"
    if not bundle.exists():
        with tempfile.TemporaryDirectory(dir=cache) as temporary:
            mirror = Path(temporary) / "source.git"
            subprocess.run(["git", "init", "-q", "--bare", str(mirror)], check=True)
            available = upstream.exists() and subprocess.run(
                ["git", "-C", str(upstream), "cat-file", "-e", revision + "^{commit}"],
                env=dict(os.environ, GIT_NO_LAZY_FETCH="1"),
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
            origin = str(upstream) if available else baseline["repository"]
            subprocess.run(["git", "-C", str(mirror), "fetch", origin, revision], check=True)
            subprocess.run(["git", "-C", str(mirror), "update-ref", "refs/heads/runner-baseline", revision], check=True)
            staged = Path(temporary) / "upstream.bundle"
            subprocess.run(["git", "-C", str(mirror), "bundle", "create", str(staged), "runner-baseline"], check=True)
            staged.replace(bundle)
    shutil.copy2(bundle, destination / "upstream.bundle")
    diff = subprocess.check_output(["git", "diff", "HEAD", "--", "src", "CMakeLists.txt", "cmake"], cwd=root)
    context = {"revision": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(),
               "branch": subprocess.check_output(["git", "symbolic-ref", "--quiet", "--short", "HEAD"], cwd=root, text=True).strip(),
               "sourceState": {"modified": bool(diff)}}
    (destination / CONTEXT).write_text(json.dumps(context) + "\n")
    manifest = destination / MANIFEST
    manifest.write_text(json.dumps(sorted(names_copied), ensure_ascii=True) + "\n")


def runner_context(root):
    if os.environ.get("SEVENZIP_RUN_TOKEN"):
        return json.loads((Path(root) / CONTEXT).read_text())
    return None
