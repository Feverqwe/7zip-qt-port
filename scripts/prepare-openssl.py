#!/usr/bin/env python3
"""Build the pinned optional static libcrypto in the existing dependency tree."""

import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def run(command, cwd, **kwargs):
    subprocess.run(command, cwd=cwd, check=True, **kwargs)


def prepare(args):
    baseline = json.loads((ROOT / "docs/openssl-reference-baseline.json").read_text())
    directory = ROOT / "build/dependencies" / baseline["release"]
    source = directory / "source"
    directory.mkdir(parents=True, exist_ok=True)
    if not shutil.which("ccache"):
        raise RuntimeError("ccache is required; keep the existing compiler cache")
    if not source.exists():
        run(["git", "clone", "--depth", "1", "--branch", baseline["release"],
             baseline["repository"], str(source)], ROOT)
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=source, text=True).strip()
    if revision != baseline["revision"]:
        raise RuntimeError("Existing OpenSSL source is not the pinned revision; preserve it for review")
    run([os.sys.executable, "scripts/upstream_baseline.py", "reference-verify",
         "--manifest", "docs/openssl-reference-baseline.json", "--upstream", str(source)], ROOT)
    system = platform.system()
    machine = platform.machine().lower()
    windows = system == "Windows"
    if windows:
        perl_os = subprocess.check_output([args.perl, "-e", "print $^O"], text=True).strip()
        if perl_os != "MSWin32":
            raise RuntimeError("MSVC requires native Windows Perl (for example Strawberry Perl); select it with --perl")
    options = ["no-shared", "no-tests", "no-apps", "no-docs"]
    environment = dict(os.environ)
    if windows:
        if machine not in ("amd64", "x86_64"):
            raise RuntimeError("This dependency helper currently supports Windows x64")
        target = "VC-WIN64A"
        # Embedded debug information permits ccache; OpenSSL otherwise uses a
        # shared /Zi database even for its optimized static library.
        options.append("-Z7")
        if not shutil.which("nasm"):
            options.append("no-asm")
        compiler = "ccache cl"
        library = source / "libcrypto.lib"
    else:
        compiler = "ccache " + ("clang" if system == "Darwin" else "cc")
        library = source / "libcrypto.a"
        if system == "Darwin":
            target = "darwin64-arm64-cc" if machine == "arm64" else "darwin64-x86_64-cc"
            options.append("-mmacosx-version-min=14.4")
            environment.setdefault("CCACHE_DIR", "/private/tmp/sevenzip-desktop-ccache")
        elif system == "Linux":
            targets = {"x86_64": "linux-x86_64", "aarch64": "linux-aarch64"}
            target = targets[machine]
            options.append("-fPIC")
        else:
            raise RuntimeError("Unsupported dependency platform: " + system)
    options.append("--prefix=" + str(directory / "install"))
    environment["CC"] = compiler
    inputs = {"revision": revision, "target": target, "options": options, "compiler": compiler}
    stamp = directory / "configuration.json"
    # Never reconfigure or discard a working build just to repeat validation.
    if args.reconfigure or (not (source / "Makefile").exists() and not (source / "makefile").exists()):
        run([args.perl, "Configure", target, *options], source, env=environment)
        stamp.write_text(json.dumps(inputs, indent=2) + "\n")
    elif stamp.exists() and json.loads(stamp.read_text()) != inputs:
        raise RuntimeError("Existing dependency configuration differs; review it and use --reconfigure if intended")
    if windows:
        run(["nmake", "/nologo", "CC=" + compiler, "build_generated"], source, env=environment)
        run(["nmake", "/nologo", "CC=" + compiler, "libcrypto.lib"], source, env=environment)
    else:
        run(["make", "-j" + str(args.jobs), "build_generated", "libcrypto.a"], source, env=environment)
    print("Static libcrypto: " + str(library), flush=True)
    if args.configure_app:
        run(["cmake", "--preset", args.preset, "-DSEVENZIP_CRYPTO_PROVIDER=OPENSSL",
             "-DOPENSSL_USE_STATIC_LIBS=TRUE", "-DOPENSSL_ROOT_DIR=" + str(source),
             "-DOPENSSL_INCLUDE_DIR=" + str(source / "include"),
             "-DOPENSSL_CRYPTO_LIBRARY=" + str(library)], ROOT)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configure-app", action="store_true")
    parser.add_argument("--reconfigure", action="store_true", help="apply reviewed configuration changes, retaining objects and cache")
    parser.add_argument("--perl", default="perl", help="Perl executable; native Windows Perl for MSVC")
    parser.add_argument("--preset", choices=("dev", "release"), default="dev")
    parser.add_argument("--jobs", type=int, default=int(os.environ.get("CMAKE_BUILD_PARALLEL_LEVEL", "4")))
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("jobs must be positive")
    prepare(args)


if __name__ == "__main__":
    main()
