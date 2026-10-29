"""Locate preset-owned native deployments and select sanitizer startup linkage."""

import argparse
import json
import os
from pathlib import Path
import re

if __package__:
    from .build_paths import directory
else:
    from build_paths import directory


def runtime_root(root, preset="dev"):
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]*", preset):
        raise ValueError("Invalid runtime preset name")
    return directory(root, "artifacts") / preset


def codec_runtime(root, preset="dev"):
    return runtime_root(root, preset) / "codec-runtime"


def sfx_runtime(root, preset="dev"):
    return runtime_root(root, preset) / "sfx"


def sanitizer_startup(root, preset):
    presets = {item["name"]: item for item in json.loads(
        (Path(root) / "CMakePresets.json").read_text())["configurePresets"]}

    def build_type(name):
        item = presets[name]
        variables = item.get("cacheVariables", {})
        if "CMAKE_BUILD_TYPE" in variables:
            value = variables["CMAKE_BUILD_TYPE"]
            return value.get("value") if isinstance(value, dict) else value
        parents = item.get("inherits", [])
        for parent in [parents] if isinstance(parents, str) else parents:
            value = build_type(parent)
            if value is not None:
                return value
        return None

    mode = os.environ.get("SEVENZIP_BUILD_MODE", "lite")
    if mode not in ("lite", "debug", "sanitize"):
        raise ValueError("SEVENZIP_BUILD_MODE must be lite, debug or sanitize")
    selected = os.environ.get("SEVENZIP_ENABLE_SANITIZERS", "ON" if mode == "sanitize" else "OFF").upper()
    if selected not in ("ON", "OFF"):
        raise ValueError("SEVENZIP_ENABLE_SANITIZERS must be ON or OFF")
    return selected == "ON" and build_type(preset) in ("Debug", "RelWithDebInfo")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--preset", default="dev")
    parser.add_argument("--component", choices=["codec", "sfx"], default="codec")
    args = parser.parse_args()
    component = codec_runtime if args.component == "codec" else sfx_runtime
    print(component(args.root, args.preset).as_posix())
