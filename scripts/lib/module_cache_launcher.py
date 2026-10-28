#!/usr/bin/env python3
"""Cache MSVC importers safely and keep interface/object outputs paired."""

import os
from pathlib import Path
import subprocess
import sys


def produces_interface(arguments):
    for argument in arguments:
        if argument.startswith(("-", "/")) and argument.lstrip("-/").lower() in (
                "interface", "ifcoutput", "exportheader"):
            return True
        if argument.startswith("@"):
            response = Path(argument[1:].strip('"'))
            if response.suffix == ".modmap":
                for line in response.read_text(encoding="utf-8-sig").splitlines():
                    parts = line.strip().split(maxsplit=1)
                    if parts and parts[0].lstrip("-/").lower() in (
                            "interface", "ifcoutput", "exportheader"):
                        return True
    return False


def imported_modules(arguments):
    for argument in arguments:
        if not argument.startswith("@"):
            continue
        response = Path(argument[1:].strip('"'))
        if response.suffix != ".modmap":
            continue
        # CMake writes one -reference "module=path" entry per line. Preserve
        # backslashes and spaces rather than applying POSIX shell unescaping.
        for line in response.read_text(encoding="utf-8-sig").splitlines():
            parts = line.strip().split(maxsplit=1)
            if len(parts) != 2 or parts[0] not in ("-reference", "/reference"):
                continue
            _, separator, name = parts[1].strip('"').partition("=")
            if separator:
                module = Path(name.strip('"'))
                if not module.is_file():
                    raise FileNotFoundError(f"Imported module is missing: {module}")
                yield str(module.resolve())


def main():
    arguments = sys.argv[1:]
    # ccache restores the object alone, not MSVC's additional IFC output.
    # Ninja still reuses completed interface builds; only a dirty producer
    # needs the compiler. Keep ordinary sources and importers in ccache.
    if produces_interface(arguments):
        return subprocess.call(arguments[1:])
    environment = os.environ.copy()
    modules = list(dict.fromkeys(imported_modules(arguments)))
    if modules:
        existing = environment.get("CCACHE_EXTRAFILES")
        environment["CCACHE_EXTRAFILES"] = os.pathsep.join(
            ([existing] if existing else []) + modules)
    return subprocess.call(arguments, env=environment)


if __name__ == "__main__":
    sys.exit(main())
