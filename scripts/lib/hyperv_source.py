"""Materialize current source for rsync without build or private state."""

import json
import os
import re
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import tempfile

SOURCE_DIRS = {".agents", "cmake", "docs", "scripts", "src", "tests", "third_party", "resources",
               "translations"}
SOURCE_FILES = {".clang-format", ".gitattributes", ".gitignore", "AGENTS.md", "CMakeLists.txt",
                "CMakePresets.json", "LICENSE", "README.md", "upstream.lock.json", "windows-runtime.lock.json"}
MANIFEST = ".runner-source.json"
CONTEXT = ".runner-context.json"


def source_name(name):
    path = PurePosixPath(name)
    if path.parts[:2] == ("scripts", "config") and path.name.endswith(".local.json"):
        return False
    return (not path.is_absolute() and ".." not in path.parts and bool(path.parts)
            and (path.parts[0] in SOURCE_DIRS or name in SOURCE_FILES or name == "upstream.bundle"))


def stage_tree(root, upstream, destination, cache):
    for args in (["git", "diff", "--check"], ["git", "diff", "--cached", "--check"]):
        subprocess.run(args, cwd=root, check=True)
    files = subprocess.check_output(["git", "ls-files", "-z", "--cached", "--others",
                                     "--exclude-standard"], cwd=root)
    names = sorted({os.fsdecode(name) for name in files.split(b"\0") if name})
    names_copied = []
    for name in names:
        if not source_name(name):
            continue
        source = root / name
        if not source.exists() and not source.is_symlink():
            continue
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
            shutil.copy2(source, target)
        names_copied.append(name)
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
    diff = subprocess.check_output(["git", "diff", "HEAD", "--", "src", "CMakeLists.txt", "cmake"], cwd=root)
    context = {"revision": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(),
               "sourceState": {"modified": bool(diff)}}
    (destination / CONTEXT).write_text(json.dumps(context) + "\n")
    manifest = destination / MANIFEST
    manifest.write_text(json.dumps(sorted(names_copied), ensure_ascii=True) + "\n")


def runner_context(root):
    if os.environ.get("SEVENZIP_RUN_TOKEN"):
        return json.loads((Path(root) / CONTEXT).read_text())
    return None
