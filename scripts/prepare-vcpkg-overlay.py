#!/usr/bin/env python3
"""Generate a pinned Qt overlay without PCH for storage-constrained builders."""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def prepare(vcpkg, output):
    baseline = json.loads((ROOT / "vcpkg-configuration.json").read_text())["default-registry"]["baseline"]
    revision = subprocess.check_output(["git", "-C", str(vcpkg), "rev-parse", "HEAD"], text=True).strip()
    if revision != baseline:
        raise ValueError("The vcpkg checkout must match the manifest registry baseline")
    names = subprocess.check_output(
        ["git", "-C", str(vcpkg), "ls-tree", "-r", "--name-only", "HEAD", "ports/qtbase"],
        text=True).splitlines()
    files = {Path(name).relative_to("ports").as_posix(): subprocess.check_output(
        ["git", "-C", str(vcpkg), "show", "HEAD:" + name]) for name in names}
    helper = "qtbase/cmake/qt_install_submodule.cmake"
    anchor = b"function(qt_cmake_configure)\n"
    if files.get(helper, b"").count(anchor) != 1:
        raise ValueError("The pinned Qt recipe has an unsupported configure entry point")
    files[helper] = files[helper].replace(anchor, anchor +
        b'    list(APPEND VCPKG_CMAKE_CONFIGURE_OPTIONS "-DBUILD_WITH_PCH=OFF")\n')
    # The installed helper applies to Qt submodules as well. All features and
    # both Debug/Release configurations remain enabled by the original recipes.
    key = hashlib.sha256(b"".join(name.encode() + data for name, data in sorted(files.items()))).hexdigest()
    destination = output / key[:16]
    marker = destination / ".sevenzip-overlay.json"
    owner = {"producer": "sevenzip-qt-no-pch", "baseline": baseline, "sha256": key}
    if destination.is_symlink() or (destination.exists() and not marker.is_file()):
        raise ValueError("Refusing an unowned overlay output directory")
    if marker.exists() and json.loads(marker.read_text()) != owner:
        raise ValueError("Unexpected generated overlay owner")
    destination.mkdir(parents=True, exist_ok=True)
    marker.write_text(json.dumps(owner) + "\n")
    for name, data in files.items():
        path = destination / name
        if path.is_symlink():
            raise ValueError("Refusing a linked overlay file")
        if not path.is_file() or path.read_bytes() != data:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
    return destination


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vcpkg", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(prepare(args.vcpkg.resolve(), args.output.resolve()).as_posix())


if __name__ == "__main__":
    main()
