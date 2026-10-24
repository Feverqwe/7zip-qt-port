#!/usr/bin/env python3
"""Install Qt 6.12 with aqt's verified downloads and kit relocation.

Repository-layout adapter for the compiler-specific Windows Qt kit.
"""
import argparse
from importlib.metadata import version
from pathlib import Path
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True)
    parser.add_argument("--outputdir", type=Path, required=True)
    args = parser.parse_args()
    if version("aqtinstall") != "3.3.0":
        parser.error("Repository-layout adapter requires aqtinstall==3.3.0")
    from aqt import installer
    from aqt.archives import QtArchives

    class WindowsArchives(QtArchives):
        def _arch_ext(self):
            if (self.os_name == "windows" and str(self.version) == "6.12.0"
                    and self.target == "desktop" and self.arch == "win64_msvc2022_64"):
                return "_msvc2022_64"
            return super()._arch_ext()

    installer.QtArchives = WindowsArchives
    return installer.Cli().run([
        "install-qt", "windows", "desktop", args.version, "win64_msvc2022_64",
        "--outputdir", str(args.outputdir), "--archives", "qtbase", "qttools", "qttranslations",
        "qtdeclarative"])


if __name__ == "__main__":
    sys.exit(main())
