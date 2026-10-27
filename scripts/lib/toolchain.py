"""Select a shared toolchain for builds and native runtime preparation."""

import argparse
import json
import os
from pathlib import Path
import platform
import re
import shlex
import shutil
import subprocess


def harness_root(root):
    result = subprocess.run(
        ["git", "-C", str(root), "rev-parse", "--path-format=absolute", "--git-common-dir"],
        capture_output=True, text=True)
    return Path(result.stdout.strip()).parent if result.returncode == 0 else root


def companion(compiler, cxx):
    path = Path(shutil.which(compiler) or compiler)
    source, replacement = ((r"(clang|gcc|cc)(-\d+)?", {"clang": "clang++", "gcc": "g++", "cc": "c++"})
                           if cxx else
                           (r"(clang\+\+|g\+\+|c\+\+)(-\d+)?", {"clang++": "clang", "g++": "gcc", "c++": "cc"}))
    match = re.fullmatch(source, path.name)
    if match:
        candidate = path.with_name(replacement[match[1]] + (match[2] or ""))
        if candidate.is_file():
            return str(candidate)
    raise ValueError("Cannot infer the companion compiler; set both CC and CXX explicitly")


def environment(root, preset="dev", current=None, require_qt=True):
    current = dict(os.environ if current is None else current)
    system = platform.system()
    if system == "Linux" and preset == "modules-linux-dev" and not current.get("CMAKE_TOOLCHAIN_FILE"):
        presets = json.loads((root / "CMakePresets.json").read_text())["configurePresets"]
        configured = next(item for item in presets if item["name"] == preset)["cacheVariables"]
        cc, cxx = current.get("CC"), current.get("CXX")
        if not cc and not cxx:
            cc, cxx = configured["CMAKE_C_COMPILER"], configured["CMAKE_CXX_COMPILER"]
        cc, cxx = cc or companion(cxx, False), cxx or companion(cc, True)
        selected = {"CC": shutil.which(cc), "CXX": shutil.which(cxx)}
        if not all(selected.values()):
            raise ValueError(f"Install the {preset} compilers ({cc}, {cxx}), or set CC and CXX")
        return selected
    if system != "Darwin":
        return {}
    if __package__:
        from .local_ramdisk import environment as scratch_environment
    else:
        from local_ramdisk import environment as scratch_environment
    selected = scratch_environment(root)
    if not current.get("CMAKE_TOOLCHAIN_FILE"):
        cc, cxx = current.get("CC"), current.get("CXX")
        if not cc and not cxx:
            brew = shutil.which("brew") or next(
                (str(path) for path in (Path("/opt/homebrew/bin/brew"), Path("/usr/local/bin/brew"))
                 if path.is_file()), None)
            if not brew:
                raise ValueError("Install Homebrew LLVM 23+ (brew install llvm), or set CC and CXX")
            package = "gcc" if preset == "modules-gcc-dev" else "llvm"
            prefix = Path(subprocess.check_output([brew, "--prefix", package], text=True).strip())
            if package == "gcc":
                candidates = sorted((prefix / "bin").glob("g++-*"),
                                    key=lambda path: int(path.name.rsplit("-", 1)[-1]))
                if not candidates:
                    raise ValueError("Homebrew GCC was not found; install gcc or set CC and CXX")
                cxx = str(candidates[-1])
                cc = companion(cxx, False)
            else:
                cc, cxx = str(prefix / "bin/clang"), str(prefix / "bin/clang++")
                version = subprocess.check_output([cxx, "--version"], text=True)
                match = re.search(r"clang version (\d+)", version)
                if not match or int(match[1]) < 23:
                    raise ValueError("Homebrew LLVM 23+ is required; update llvm or set CC and CXX")
        cc = cc or companion(cxx, False)
        cxx = cxx or companion(cc, True)
        selected.update(CC=shutil.which(cc) or cc, CXX=shutil.which(cxx) or cxx,
                        OBJCXX=current.get("OBJCXX") or shutil.which(cxx) or cxx)
        compiler_bin = str(Path(selected["CXX"]).parent)
        if compiler_bin not in current.get("PATH", "").split(os.pathsep):
            selected["PATH"] = compiler_bin + os.pathsep + current.get("PATH", "")
    if require_qt and not current.get("CMAKE_PREFIX_PATH") and not current.get("CMAKE_TOOLCHAIN_FILE") and \
            not preset.startswith("vcpkg-"):
        qt = harness_root(root) / "build/qt/6.12.0/macos"
        if not (qt / "lib/cmake/Qt6/Qt6Config.cmake").is_file():
            raise ValueError("Install the Qt 6.12.0 macOS kit in the harness build/qt, or set CMAKE_PREFIX_PATH")
        selected["CMAKE_PREFIX_PATH"] = str(qt)
    return selected


def apply_environment(root, preset="dev", require_qt=True):
    os.environ.update(environment(root, preset, require_qt=require_qt))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--preset", default="dev")
    parser.add_argument("--runtime-only", action="store_true")
    parser.add_argument("--upstream", action="store_true")
    args = parser.parse_args()
    if args.upstream:
        print(harness_root(args.root).parent / "7zip-upstream")
        return
    for name, value in environment(args.root, args.preset, require_qt=not args.runtime_only).items():
        print(f"export {name}={shlex.quote(value)}")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        raise SystemExit(f"toolchain: {error}")
