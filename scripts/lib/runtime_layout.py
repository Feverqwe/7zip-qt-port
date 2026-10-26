"""Locate preset-owned native deployments and select sanitizer startup linkage."""

import argparse
import json
from pathlib import Path
import re


def runtime_root(root, preset="dev"):
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]*", preset):
        raise ValueError("Invalid runtime preset name")
    build = Path(root) / "build"
    return build if preset == "dev" else build / preset


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

    return build_type(preset) in ("Debug", "RelWithDebInfo")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--preset", default="dev")
    parser.add_argument("--component", choices=["codec", "sfx"], default="codec")
    args = parser.parse_args()
    directory = codec_runtime if args.component == "codec" else sfx_runtime
    print(directory(args.root, args.preset).as_posix())
