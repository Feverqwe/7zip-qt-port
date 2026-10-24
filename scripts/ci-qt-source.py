#!/usr/bin/env python3
"""Fetch private license inputs; source archives are never packaged or uploaded.

Qt/ICU source inputs supply the corresponding runtime license notices.
"""
import hashlib
from pathlib import Path
import subprocess

VERSION = "6.12.0"
SHA256 = "a951bd163c7b80fc6b8c88d7668fb56abf91c152373e13c10666763238131307"
ICU_SHA256 = "f3005e195ff74d8812cc1f182a1c446fab678d70a10e3dada497585befee5416"


def fetch(url, path, expected):
    subprocess.run(["curl", "--fail", "--location", "--retry", "3",
                    "--output", str(path), url], check=True)
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    if digest.hexdigest() != expected:
        path.unlink()
        raise ValueError(f"Official input checksum mismatch: {path.name}")


def main():
    destination = Path(__file__).resolve().parents[1] / "build/qt-source"
    destination.mkdir(parents=True, exist_ok=True)
    archive = destination / f"qtbase-everywhere-src-{VERSION}.tar.xz"
    fetch(f"https://download.qt.io/official_releases/qt/6.12/{VERSION}/submodules/{archive.name}",
          archive, SHA256)
    archive.with_suffix(".xz.sha256").write_text(f"{SHA256}  {archive.name}\n")
    fetch("https://raw.githubusercontent.com/unicode-org/icu/release-73-2/icu4c/LICENSE",
          destination / "icu-73.2-LICENSE", ICU_SHA256)


if __name__ == "__main__":
    main()
