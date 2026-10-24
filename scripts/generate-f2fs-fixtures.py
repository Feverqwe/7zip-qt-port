#!/usr/bin/env python3
"""Generate synthetic F2FS fixtures on Linux with official mkfs/sload tools."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import zlib


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    mkfs = shutil.which("mkfs.f2fs") or "/sbin/mkfs.f2fs"
    sload = shutil.which("sload.f2fs") or "/sbin/sload.f2fs"
    record = {"producer": subprocess.check_output([mkfs, "-V"], stderr=subprocess.STDOUT).decode().strip(), "images": {}}
    with tempfile.TemporaryDirectory(prefix="sevenzip-f2fs-fixtures-") as temporary:
        work = Path(temporary)
        source = work / "source"
        source.mkdir()
        (source / "папка 空").mkdir()
        (source / "папка 空" / "tiny.txt").write_bytes(b"inline F2FS\n")
        (source / "empty").touch()
        # Reaches inode, direct and indirect node mapping with deterministic data.
        data = bytes(range(256)) * (12 * 1024 * 1024 // 256) + b"tail"
        (source / "large.bin").write_bytes(data)
        (source / "link").symlink_to("папка 空/tiny.txt")
        for variant, features in (("basic", ""), ("modern", "extra_attr,inode_checksum,sb_checksum")):
            image = work / "image.f2fs"
            with image.open("wb") as output:
                output.truncate(128 * 1024 * 1024)
            options = [mkfs, "-f", "-t", "0", "-r", "-T", "1700000000", "-U", "12345678-1234-5678-90ab-123456789abc"]
            if features:
                options += ["-O", features]
            subprocess.run([*options, str(image)], check=True)
            subprocess.run([sload, "-T", "1700000000", "-f", str(source), str(image)], check=True)
            raw = image.read_bytes()
            (args.output / (variant + ".f2fs.zlib")).write_bytes(zlib.compress(raw, 9))
            record["images"][variant] = {"size": len(raw), "sha256": hashlib.sha256(raw).hexdigest(), "features": features}
            image.unlink()
        record["large_sha256"] = hashlib.sha256(data).hexdigest()
    (args.output / "manifest.json").write_text(json.dumps(record, indent=2) + "\n")


if __name__ == "__main__":
    main()
