#!/usr/bin/env python3
"""Archive a fresh CMake runtime install, with notices and without sources."""
import argparse
import hashlib
import json
from pathlib import Path
import platform
import re
import shutil
import subprocess
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[1]
DEVELOPMENT_SUFFIXES = {".c", ".cpp", ".cc", ".h", ".hpp", ".mm", ".m",
                        ".py", ".cmake", ".a", ".lib", ".pdb", ".prl"}
DEVELOPMENT_DIRS = {"Source", "Sources", "src", "include", "Headers",
                    "PrivateHeaders", ".git", "tests", "cmake", "__pycache__"}
MACHO_MAGIC = {b"\xfe\xed\xfa\xce", b"\xce\xfa\xed\xfe",
               b"\xfe\xed\xfa\xcf", b"\xcf\xfa\xed\xfe",
               b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca",
               b"\xca\xfe\xba\xbf", b"\xbf\xba\xfe\xca"}


def qt_notices(archive, checksum, version, output):
    prefix = f"qtbase-everywhere-src-{version}"
    if archive.name != prefix + ".tar.xz":
        raise ValueError("Qt notice input must match the linked Qt version")
    digest = hashlib.sha256()
    with archive.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    if digest.hexdigest() != checksum.read_text().split()[0]:
        raise ValueError("Qt notice input checksum mismatch")
    count = 0
    with tarfile.open(archive) as source:
        for member in source.getmembers():
            if not member.isfile() or not member.name.startswith(prefix + "/LICENSES/"):
                continue
            relative = Path(member.name[len(prefix) + 1:])
            if relative.is_absolute() or ".." in relative.parts:
                raise ValueError("Unsafe Qt notice path")
            target = output / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            with source.extractfile(member) as stream:
                target.write_bytes(stream.read())
            count += 1
    if not count:
        raise ValueError("Qt notice input has no licenses")


def prune_development(root):
    for path in sorted(root.rglob("*"), key=lambda item: len(item.parts), reverse=True):
        if (path.name in DEVELOPMENT_DIRS or path.suffix in DEVELOPMENT_SUFFIXES
                or path.suffix == ".dSYM"):
            if path.is_symlink() or path.is_file():
                path.unlink()
            elif path.is_dir():
                shutil.rmtree(path)
        elif path.name.startswith(("libqoffscreen.", "qoffscreen.", "libqminimal.", "qminimal.")):
            path.unlink()
        elif (path.parent.name == "imageformats" and path.is_file()
              and not any(name in path.name for name in ("qico", "qsvg"))):
            # UI resources are BMP/ICO; theme icons may use SVG. No image viewer.
            path.unlink()


def validate_payload(root):
    for path in root.rglob("*"):
        relative = path.relative_to(root)
        if (any(part in DEVELOPMENT_DIRS for part in relative.parts)
                or path.suffix in DEVELOPMENT_SUFFIXES
                or path.name in {"CMakeCache.txt", "compile_commands.json", "CMakeLists.txt"}
                or path.name.endswith((".tar.xz", ".tar.gz", ".zip"))):
            raise ValueError(f"Development payload in runtime package: {relative}")
        if path.is_symlink() and (not path.exists() or not path.resolve().is_relative_to(root.resolve())):
            raise ValueError(f"Broken or escaping runtime symlink: {relative}")


def imports(path):
    output = subprocess.check_output(["dumpbin.exe", "/dependents", str(path)], text=True)
    return re.findall(r"^\s+([^\s]+\.dll)\s*$", output, re.MULTILINE | re.IGNORECASE)


def deploy_crt(root, runtime):
    # Follow htopQt's imported CRT closure instead of shipping a CRT installer.
    for path in root.rglob("*.exe"):
        if path.name.lower().startswith("vc_redist."):
            path.unlink()
    candidates = {path.name.lower(): path for path in runtime.glob("*.dll")}
    queue = [path for path in root.rglob("*") if path.suffix.lower() in {".exe", ".dll"}]
    copied = set()
    while queue:
        for name in imports(queue.pop()):
            key = name.lower()
            if key in candidates and key not in copied:
                original = candidates[key]
                target = root / "bin" / original.name
                shutil.copy2(original, target)
                copied.add(key)
                queue.append(target)
    if "vcruntime140.dll" not in copied:
        raise ValueError("Missing imported VC runtime dependency")


def compact_and_sign(bundle):
    architecture = platform.machine()
    for path in bundle.rglob("*"):
        if path.is_symlink() or not path.is_file():
            continue
        with path.open("rb") as stream:
            if stream.read(4) not in MACHO_MAGIC:
                continue
        arches = subprocess.check_output(["lipo", "-archs", str(path)], text=True).split()
        if architecture not in arches:
            raise ValueError(f"Missing native runtime slice: {path}")
        if len(arches) > 1:
            thin = path.with_name(path.name + ".thin")
            subprocess.run(["lipo", str(path), "-thin", architecture, "-output", str(thin)], check=True)
            thin.replace(path)
        subprocess.run(["strip", "-x", str(path)], check=True)
    sign_bundle(bundle)


def sign_bundle(bundle):
    # Same inside-out ad-hoc signing order as htopQt/scripts/package.py.
    # Include the console engine/module in Resources as nested runtime code.
    containers = []
    for path in bundle.rglob("*"):
        if path.is_symlink():
            continue
        if path.is_dir() and path.suffix in {".framework", ".bundle", ".app"}:
            containers.append(path)
        if path.is_file():
            with path.open("rb") as stream:
                if stream.read(4) not in MACHO_MAGIC:
                    continue
            subprocess.run(["codesign", "--force", "--sign", "-",
                            "--timestamp=none", str(path)], check=True)
    # Signing/stripping changes the SFX bytes; discovery verifies this sidecar.
    sfx = bundle / "Contents/Resources/7zip/7zCon.sfx"
    record_path = sfx.with_suffix(".sfx.json")
    record = json.loads(record_path.read_text())
    record["sha256"] = hashlib.sha256(sfx.read_bytes()).hexdigest()
    record_path.write_text(json.dumps(record, indent=2) + "\n")
    for path in sorted(containers, key=lambda item: len(item.parts), reverse=True):
        subprocess.run(["codesign", "--force", "--sign", "-",
                        "--timestamp=none", str(path)], check=True)
    subprocess.run(["codesign", "--force", "--sign", "-",
                    "--timestamp=none", str(bundle)], check=True)
    subprocess.run(["codesign", "--verify", "--deep", "--strict", str(bundle)], check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build/release")
    parser.add_argument("--qt-source", type=Path, required=True)
    parser.add_argument("--qt-sha256", type=Path, required=True)
    parser.add_argument("--icu-license", type=Path)
    parser.add_argument("--vc-runtime", type=Path)
    parser.add_argument("--vc-notices", type=Path)
    args = parser.parse_args()
    info = json.loads((args.build_dir / "PackageInfo.json").read_text())
    destination = ROOT / "build/packages"
    destination.mkdir(parents=True, exist_ok=True)
    system = platform.system()
    if system == "Windows" and (args.vc_runtime is None or args.vc_notices is None):
        parser.error("Windows packages require --vc-runtime and --vc-notices")
    arch = {"AMD64": "x86_64", "aarch64": "arm64"}.get(platform.machine(), platform.machine())
    name = f"sevenzip-desktop-{info['version']}-{system.lower()}-{arch}"
    with tempfile.TemporaryDirectory(prefix="stage-", dir=destination) as temporary:
        stage = Path(temporary)
        install = stage / name
        subprocess.run(["cmake", "--install", str(args.build_dir), "--prefix", str(install)], check=True)
        if system == "Darwin":
            bundle = install / "sevenzip_desktop.app"
            # Build aliases are copies; installed aliases share the actual application.
            for alias in ("7zFM", "7zG"):
                path = bundle / "Contents/MacOS" / alias
                path.unlink(missing_ok=True)
                path.symlink_to("sevenzip_desktop")
            notices = bundle / "Contents/Resources/Notices"
        else:
            notices = install / "share/licenses/sevenzip-desktop"
            if system == "Linux":
                for alias in ("7zFM", "7zG"):
                    path = install / "bin" / alias
                    path.unlink()
                    path.symlink_to("sevenzip_desktop")
        qt_notices(args.qt_source, args.qt_sha256, info["qt_version"], notices / "Qt")
        if system == "Windows":
            deploy_crt(install, args.vc_runtime)
            shutil.copy2(args.vc_notices, notices / "Microsoft-VC-Redist.txt")
        if any("icu" in path.name.lower() for path in install.rglob("*")):
            if args.icu_license is None:
                raise ValueError("Deployed ICU needs its license input")
            shutil.copy2(args.icu_license, notices / "ICU-LICENSE")
        prune_development(install)
        validate_payload(install)
        if system == "Darwin":
            compact_and_sign(bundle)
            archive = stage / (name + ".zip")
            subprocess.run(["ditto", "--norsrc", "-c", "-k", "--keepParent",
                            str(bundle), str(archive)], check=True)
        elif system == "Windows":
            archive = Path(shutil.make_archive(str(stage / name), "zip", stage, name))
        else:
            archive = stage / (name + ".tar.gz")
            with tarfile.open(archive, "w:gz") as stream:
                stream.add(install, arcname=name)
        archive.replace(destination / archive.name)
        print(destination / archive.name)


if __name__ == "__main__":
    main()
