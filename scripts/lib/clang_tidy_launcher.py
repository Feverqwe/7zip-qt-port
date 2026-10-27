#!/usr/bin/env python3
"""Expose CMake's Clang module-map arguments to clang-tidy's tooling driver."""

import os
from pathlib import Path
import shlex
import subprocess
import sys


def response_arguments(contents):
    if os.name != "nt":
        return shlex.split(contents)
    import ctypes
    from ctypes import wintypes

    count = ctypes.c_int()
    parse = ctypes.windll.shell32.CommandLineToArgvW
    parse.argtypes = (wintypes.LPCWSTR, ctypes.POINTER(ctypes.c_int))
    parse.restype = ctypes.POINTER(wintypes.LPWSTR)
    values = parse("clang " + contents.replace("\r", " ").replace("\n", " "), ctypes.byref(count))
    if not values:
        raise ctypes.WinError()
    free = ctypes.windll.kernel32.LocalFree
    free.argtypes = (ctypes.c_void_p,)
    free.restype = ctypes.c_void_p
    try:
        return [values[index] for index in range(1, count.value)]
    finally:
        free(ctypes.cast(values, ctypes.c_void_p))


def expand_module_maps(arguments):
    for argument in arguments:
        if argument.startswith("@") and Path(argument[1:].strip('"')).suffix == ".modmap":
            contents = Path(argument[1:].strip('"')).read_text(encoding="utf-8-sig")
            options = response_arguments(contents)
            if any(option.startswith(("-fmodule-file=", "-fmodule-output=")) for option in options):
                yield from options
                continue
        yield argument


def main():
    arguments = sys.argv[1:]
    if not arguments:
        raise SystemExit("Usage: clang_tidy_launcher.py <clang-tidy> [arguments]")
    if "--" in arguments:
        separator = arguments.index("--") + 1
        arguments = arguments[:separator] + list(expand_module_maps(arguments[separator:]))
    return subprocess.call(arguments)


if __name__ == "__main__":
    sys.exit(main())
