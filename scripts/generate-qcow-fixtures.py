#!/usr/bin/env python3
"""Encode disposable QCOW2 v3 test images over the existing generated MBR/ext4.

No device is opened, attached or mounted. This is a fixture encoder, not a
production decoder. Layout: https://www.qemu.org/docs/master/interop/qcow2.html
"""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
CLUSTER = 4096
VIRTUAL_SIZE = 1 << 30
COPIED = 1 << 63
COMPRESSED = 1 << 62


def encode(raw, compress):
    chunks = [raw[pos:pos + CLUSTER].ljust(CLUSTER, b"\0")
              for pos in range(0, len(raw), CLUSTER)]
    groups = sorted({index // 512 for index, block in enumerate(chunks) if any(block)})
    image = bytearray((4 + len(groups)) * CLUSTER)
    l2 = {group: (4 + index) * CLUSTER for index, group in enumerate(groups)}
    struct.pack_into(">4sIQIIQIIQQIIQ", image, 0, b"QFI\xfb", 3, 0, 0, 12,
                     VIRTUAL_SIZE, 0, 512, CLUSTER, 2 * CLUSTER, 1, 0, 0)
    struct.pack_into(">QQQII", image, 72, 0, 0, 0, 4, 104)
    struct.pack_into(">Q", image, 2 * CLUSTER, 3 * CLUSTER)
    for group, table in l2.items():
        struct.pack_into(">Q", image, CLUSTER + group * 8, COPIED | table)
    compressed_count = 0
    used_end = len(image)
    for index, block in enumerate(chunks):
        table = l2.get(index // 512)
        if table is None:
            continue
        if not any(block):
            # Alternate explicit v3 zero entries with unallocated clusters.
            entry = 1 if index & 1 else 0
        else:
            offset = len(image)
            encoder = zlib.compressobj(level=9, wbits=-12)
            packed = encoder.compress(block) + encoder.flush()
            sectors = (len(packed) + 511) // 512
            if compress and sectors * 512 < CLUSTER:
                entry = COMPRESSED | ((sectors - 1) << 58) | offset
                image.extend(packed.ljust(CLUSTER, b"\0"))
                used_end = offset + sectors * 512
                compressed_count += 1
            else:
                entry = COPIED | offset
                image.extend(block)
                used_end = len(image)
        struct.pack_into(">Q", image, table + (index % 512) * 8, entry)
    # One 16-bit refcount block covers every allocated host cluster here.
    allocated = len(image) // CLUSTER
    if allocated > CLUSTER // 2:
        raise ValueError("fixture exceeds its single refcount block")
    for index in range(allocated):
        struct.pack_into(">H", image, 3 * CLUSTER + index * 2, 1)
    return bytes(image[:used_end]), compressed_count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    raw = gzip.decompress((ROOT / "tests/fixtures/devices/mbr.img.gz").read_bytes())
    args.destination.mkdir(parents=True, exist_ok=True)
    records = []
    for compressed in (False, True):
        image, count = encode(raw, compressed)
        name = "deflate.qcow2.gz" if compressed else "stored.qcow2.gz"
        destination = args.destination / name
        if not destination.is_file() or gzip.decompress(destination.read_bytes()) != image:
            destination.write_bytes(gzip.compress(image, mtime=0))
        records.append({"file": name, "size": len(image),
                        "sha256": hashlib.sha256(image).hexdigest(),
                        "compressed_clusters": count})
    # Use QEMU itself for the new compression variant instead of extending
    # the local fixture encoder. Its check/compare provide an independent oracle.
    with tempfile.TemporaryDirectory(prefix="sevenzip-qcow-fixture-") as temporary:
        source = Path(temporary) / "disk.raw"
        source.write_bytes(raw)
        with source.open("r+b") as stream:
            stream.truncate(VIRTUAL_SIZE)
        target = Path(temporary) / "disk.qcow2"
        subprocess.run(["qemu-img", "convert", "-f", "raw", "-O", "qcow2", "-c",
                        "-o", "cluster_size=4096,compression_type=zstd",
                        str(source), str(target)], check=True)
        subprocess.run(["qemu-img", "check", "-q", str(target)], check=True)
        subprocess.run(["qemu-img", "compare", "-q", "-f", "qcow2", "-F", "raw",
                        str(target), str(source)], check=True)
        image = target.read_bytes()
    name = "zstd.qcow2.gz"
    (args.destination / name).write_bytes(gzip.compress(image, mtime=0))
    records.append({"file": name, "size": len(image),
                    "sha256": hashlib.sha256(image).hexdigest(),
                    "compression": "ZSTD", "producer": "qemu-img"})
    manifest = {"virtual_size": VIRTUAL_SIZE, "source_size": len(raw),
                "source_sha256": hashlib.sha256(raw).hexdigest(),
                "path": "папка 空/данные с пробелом.txt",
                "payload": "device ext4 payload\n", "images": records}
    (args.destination / "manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")


if __name__ == "__main__":
    main()
