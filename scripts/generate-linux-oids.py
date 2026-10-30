#!/usr/bin/env python3
"""Regenerate the Linux OID table using the retained original Perl generator.

Ordinary builds consume the generated C data and do not require Perl/bc.
"""
import os
from pathlib import Path
import re
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.dependencies.source_archives import SourceArchives


def generate():
    root = Path(__file__).resolve().parent.parent
    header = root / "src/compat/linux/include/linux/oid_registry.h"
    names = re.findall(r"\b(OID_\w+),\s*/\*\s+[012][.0-9]*\s+\*/", header.read_text())
    with tempfile.TemporaryDirectory(prefix="linux-oids-") as temporary:
        output = Path(temporary) / "registry.c"
        generator = Path(temporary) / "build_OID_registry"
        generator.write_bytes(SourceArchives(root).read_bytes("third_party/linux/lib/build_OID_registry"))
        env = dict(os.environ, srctree=str(root))
        subprocess.run(["perl", str(generator),
                        "src/compat/linux/include/linux/oid_registry.h", str(output)],
                       cwd=root, env=env, check=True)
        data = output.read_text()
    # Preserve ordinal positions and bytes; array designators are unsupported
    # by the project's MSVC C11 compiler.
    indices = re.findall(r"^\s*\[(OID_\w+)\]\s*=", data, re.M)
    if indices != names + ["OID__NR"]:
        raise RuntimeError("Linux OID index order changed")
    data = re.sub(r"^(\s*)\[(OID_\w+)\]\s*=\s*", r"\1/* \2 */ ", data, flags=re.M)
    positions = [int(value) for value in re.findall(r"^\s*\[\s*(\d+)\]\s*=", data, re.M)]
    if positions != list(range(len(names))):
        raise RuntimeError("Linux OID search positions changed")
    data = re.sub(r"^(\s*)\[\s*(\d+)\]\s*=\s*", r"\1/* \2 */ ", data, flags=re.M)
    # Linux's nonnegative enum bitfield is unsigned; MSVC otherwise signs an
    # eight-bit enum slot and corrupts identifiers >= 128.
    if len(re.findall(r"enum OID oid : (?:8|16);", data)) != 1:
        raise RuntimeError("Linux OID search field changed")
    data = re.sub(r"enum OID oid : (8|16);", r"unsigned int oid : \1;", data)
    destination = root / "src/compat/linux/core/oid_registry_data.c"
    destination.write_text("/* SPDX-License-Identifier: GPL-2.0-or-later */\n" + data)
    print(f"Generated {len(names)} Linux OIDs")


if __name__ == "__main__":
    generate()
