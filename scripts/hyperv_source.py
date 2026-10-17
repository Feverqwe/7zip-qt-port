"""Materialize and verify the current source tree without build or private state."""

import hashlib
import json
import os
import re
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import tempfile

SOURCE_DIRS = {".agents", "cmake", "docs", "scripts", "src", "tests", "third_party", "resources"}
SOURCE_FILES = {".clang-format", ".gitattributes", ".gitignore", "AGENTS.md", "CMakeLists.txt",
                "CMakePresets.json", "LICENSE", "README.md", "upstream.lock.json", "windows-runtime.lock.json"}
MANIFEST = ".runner-source.json"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_name(name):
    path = PurePosixPath(name)
    return (not path.is_absolute() and ".." not in path.parts and bool(path.parts)
            and (path.parts[0] in SOURCE_DIRS or name in SOURCE_FILES or name == "upstream.bundle"))


def stage_tree(root, upstream, destination, cache):
    for args in (["git", "diff", "--check"], ["git", "diff", "--cached", "--check"]):
        subprocess.run(args, cwd=root, check=True)
    files = subprocess.check_output(["git", "ls-files", "-z", "--cached", "--others",
                                     "--exclude-standard"], cwd=root)
    names = sorted({os.fsdecode(name) for name in files.split(b"\0") if name})
    records = {}
    for name in names:
        if not source_name(name):
            continue
        source = root / name
        if not source.exists() and not source.is_symlink():
            continue
        if source.is_symlink():
            target = os.readlink(source)
            if Path(target).is_absolute() or root.resolve() not in source.resolve().parents:
                raise ValueError(f"Source link leaves project: {name}")
            record = {"link": target}
        elif source.is_file():
            record = {"sha256": sha(source), "executable": bool(source.stat().st_mode & 0o111)}
        else:
            raise ValueError(f"Unsupported source entry: {name}")
        target = destination / name
        target.parent.mkdir(parents=True, exist_ok=True)
        if source.is_symlink():
            target.symlink_to(record["link"])
        else:
            shutil.copy2(source, target)
            if sha(target) != record["sha256"]:
                raise ValueError(f"Source changed while copying: {name}; retry after editing finishes")
        records[name] = record
    baseline = json.loads((destination / "docs/upstream-baseline.json").read_text())
    revision = baseline["revision"]
    if not re.fullmatch(r"[0-9a-f]{40}", revision):
        raise ValueError("Invalid pinned upstream revision")
    cache.mkdir(parents=True, exist_ok=True)
    bundle = cache / f"upstream-{revision}.bundle"
    if not bundle.exists():
        with tempfile.TemporaryDirectory(dir=cache) as temporary:
            mirror = Path(temporary) / "source.git"
            subprocess.run(["git", "clone", "-q", "--bare", "--shared", str(upstream), str(mirror)], check=True)
            subprocess.run(["git", "-C", str(mirror), "update-ref", "refs/heads/runner-baseline", revision], check=True)
            staged = Path(temporary) / "upstream.bundle"
            subprocess.run(["git", "-C", str(mirror), "bundle", "create", str(staged), "runner-baseline"], check=True)
            staged.replace(bundle)
    shutil.copy2(bundle, destination / "upstream.bundle")
    records["upstream.bundle"] = {"sha256": sha(bundle), "executable": False}
    manifest = destination / MANIFEST
    manifest.write_text(json.dumps(records, sort_keys=True, ensure_ascii=True) + "\n")
    return sha(manifest)


def verify_tree(root, digest):
    manifest = root / MANIFEST
    if sha(manifest) != digest:
        raise ValueError("Source manifest checksum mismatch")
    records = json.loads(manifest.read_text())
    for name, record in records.items():
        if not source_name(name):
            raise ValueError(f"Invalid source path: {name}")
        path = root / name
        if any(parent.is_symlink() for parent in path.parents if parent != root and root in parent.parents):
            raise ValueError(f"Symlink parent in source path: {name}")
        if "link" in record:
            if not path.is_symlink() or os.readlink(path) != record["link"] or root.resolve() not in path.resolve().parents:
                raise ValueError(f"Source link mismatch: {name}")
        elif path.is_symlink() or not path.is_file() or sha(path) != record["sha256"]:
            raise ValueError(f"Source checksum mismatch: {name}")
        elif os.name != "nt" and bool(path.stat().st_mode & 0o111) != record["executable"]:
            raise ValueError(f"Source executable mode mismatch: {name}")
    print(f"Verified source: {digest} ({len(records)} files)", flush=True)


if __name__ == "__main__":
    import sys
    verify_tree(Path.cwd(), sys.argv[1])
