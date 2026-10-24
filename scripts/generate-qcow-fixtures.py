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


def generate_references(destination):
    """Use QEMU to build dependent images; do not invent reference mappings."""
    with tempfile.TemporaryDirectory(prefix="sevenzip-qcow-references-") as temporary:
        root = Path(temporary)
        nested = root / "слои 空"
        nested.mkdir()
        base = nested / "база.raw"
        base.write_bytes(bytes((index * 17 + 3) % 256 for index in range(65536)))
        def run(args, cwd=root):
            subprocess.run(args, cwd=cwd, check=True, stdout=subprocess.DEVNULL)
        run(["qemu-img", "create", "-f", "qcow2", "-F", "raw", "-b", base.name,
             "-o", "cluster_size=16384,extended_l2=on", "родитель.qcow2", "131072"], nested)
        run(["qemu-io", "-f", "qcow2", "-c", "write -P 0x42 512 512", "-c",
             "write -z 1024 512", "родитель.qcow2"], nested)
        run(["qemu-img", "create", "-f", "qcow2", "-F", "qcow2", "-b",
             "слои 空/родитель.qcow2", "диск.qcow2"])
        run(["qemu-io", "-f", "qcow2", "-c", "write -P 0x5a 4096 512", "-c",
             "write -z 8192 4096", "диск.qcow2"])
        run(["qemu-img", "convert", "-f", "qcow2", "-O", "raw", "диск.qcow2", "expected.raw"])
        sets = [{"name": "backing-chain", "entry": "диск.qcow2", "files": {
            "диск.qcow2": "top", "слои 空/родитель.qcow2": "parent", "слои 空/база.raw": "base"}}]
        contents = {"expected": root / "expected.raw", "base": base,
                    "parent": nested / "родитель.qcow2", "top": root / "диск.qcow2"}
        for variant, options in (
                ("external", "cluster_size=4096,data_file_raw=on"),
                ("external-extended", "cluster_size=16384,extended_l2=on")):
            image = variant + ".qcow2"
            data = variant + " данные 空.raw"
            run(["qemu-img", "convert", "-f", "raw", "-O", "qcow2", "-o",
                 options + ",data_file=" + data, "expected.raw", image])
            run(["qemu-img", "check", "-q", image])
            run(["qemu-img", "compare", "-q", "-f", "qcow2", "-F", "raw", image, "expected.raw"])
            contents[variant] = root / image
            contents[variant + "-data"] = root / data
            sets.append({"name": variant, "entry": image, "files": {
                image: variant, data: variant + "-data"}})
        for image in (root / "диск.qcow2", nested / "родитель.qcow2"):
            run(["qemu-img", "check", "-q", str(image)])
        folder = destination / "references"
        folder.mkdir(exist_ok=True)
        for name, source in contents.items():
            target = folder / (name + ".gz")
            data = source.read_bytes()
            if not target.is_file() or gzip.decompress(target.read_bytes()) != data:
                target.write_bytes(gzip.compress(data, mtime=0))
        for record in sets:
            record["files"] = {local: "references/" + fixture + ".gz"
                               for local, fixture in record["files"].items()}
            record["expected"] = "references/expected.gz"
        return sets


def generate_snapshots(destination):
    """Read-only snapshot oracles come from QEMU's temporary snapshot loader."""
    records = []
    with tempfile.TemporaryDirectory(prefix="sevenzip-qcow-snapshots-") as temporary:
        root = Path(temporary)
        source = root / "source.raw"
        seed = bytes((i * 17 + 3) % 256 for i in range(65536))
        folder = destination / "snapshots"
        folder.mkdir(exist_ok=True)
        def run(args):
            subprocess.run(args, cwd=root, check=True, stdout=subprocess.DEVNULL)
        for variant, options in (("v2", "cluster_size=4096,compat=0.10"),
                                 ("extended", "cluster_size=16384,extended_l2=on,compression_type=zstd"),
                                 ("backed", "cluster_size=16384,extended_l2=on"),
                                 ("filesystem", "cluster_size=4096")):
            source.write_bytes(gzip.decompress((ROOT / "tests/fixtures/devices/mbr.img.gz").read_bytes())
                               if variant == "filesystem" else seed)
            if variant == "v2":
                with source.open("r+b") as stream:
                    stream.truncate(196608)
            image = variant + ".qcow2"
            if variant == "backed":
                run(["qemu-img", "create", "-q", "-f", "qcow2", "-F", "raw",
                     "-b", "source.raw", "-o", options, image])
            else:
                command = ["qemu-img", "convert", "-f", "raw", "-O", "qcow2", "-o", options]
                if variant in ("extended", "filesystem"):
                    command += ["-c"]
                run(command + ["source.raw", image])
            run(["qemu-img", "snapshot", "-c", "../../снимок 空", image])
            run(["qemu-io", "-f", "qcow2", "-c", "write -P 0x42 4096 4096",
                 "-c", "write -z 0 512", image])
            if variant in ("extended", "backed"):
                run(["qemu-img", "resize", image, "131072"])
            run(["qemu-io", "-f", "qcow2", "-c", "write -P 0x73 98304 512", image])
            run(["qemu-img", "snapshot", "-c", "second", image])
            if variant in ("extended", "backed"):
                run(["qemu-img", "resize", image, "196608"])
            run(["qemu-io", "-f", "qcow2", "-c", "write -P 0x5a 0 8192",
                 "-c", "write -z 8192 8192", "-c", "write -P 0xaa 163840 512", image])
            # Fix only fixture timestamps so regeneration is reproducible.
            contents = bytearray((root / image).read_bytes())
            table = struct.unpack_from(">Q", contents, 64)[0]
            for index in range(2):
                table = (table + 7) & ~7
                struct.pack_into(">II", contents, table + 16, 1700000000 + index * 3600, 123456789)
                id_size, name_size = struct.unpack_from(">HH", contents, table + 12)
                extra_size = struct.unpack_from(">I", contents, table + 36)[0]
                table += 40 + extra_size + id_size + name_size
            (root / image).write_bytes(contents)
            run(["qemu-img", "check", "-q", image])
            files = {"image": root / image}
            if variant == "backed":
                files["backing"] = source
            for index in range(3):
                expected = variant + f"-{index}.raw"
                command = ["qemu-img", "convert", "-f", "qcow2", "-O", "raw"]
                if index:
                    command += ["-l", "snapshot.id=" + str(index)]
                run(command + [image, expected])
                if variant in ("extended", "backed") and index:
                    # QEMU load_tmp retains the active geometry. Keep its
                    # decoded bytes within the snapshot's saved disk size.
                    size = 65536 if index == 1 else 131072
                    data = (root / expected).read_bytes()
                    if any(data[size:]):
                        raise ValueError("unexpected snapshot bytes beyond saved geometry")
                    (root / expected).write_bytes(data[:size])
                files[str(index)] = root / expected
            record = {"name": variant, "files": {}}
            for name, path in files.items():
                fixture = "snapshots/" + variant + "-" + name + ".gz"
                target = destination / fixture
                data = path.read_bytes()
                if not target.is_file() or gzip.decompress(target.read_bytes()) != data:
                    target.write_bytes(gzip.compress(data, mtime=0))
                record["files"][name] = fixture
            records.append(record)
    return records


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
        for variant, compression, options in (
                ("qcow1-stored", None, ""),
                ("qcow1-deflate", "zlib", ""),
                ("qcow1-deflate-short", "zlib", ""),
                ("zstd", "zstd", "cluster_size=4096"),
                ("extended-stored", None, "cluster_size=16384,extended_l2=on"),
                ("extended-deflate", "zlib", "cluster_size=16384,extended_l2=on"),
                ("extended-zstd", "zstd", "cluster_size=16384,extended_l2=on")):
            target = Path(temporary) / (variant + ".qcow2")
            image_format = "qcow" if variant.startswith("qcow1-") else "qcow2"
            command = ["qemu-img", "convert", "-f", "raw", "-O", image_format]
            if compression and image_format == "qcow2":
                command += ["-c"]
                options += ",compression_type=" + compression
            if options:
                command += ["-o", options]
            if compression and image_format == "qcow":
                # qcow's compressed writer rejects qemu-img's final zero-byte
                # request. Exercise that writer directly with complete clusters.
                subprocess.run(["qemu-img", "create", "-q", "-f", "qcow",
                                str(target), str(VIRTUAL_SIZE)], check=True)
                block_file = Path(temporary) / "cluster.raw"
                for offset in range(0, len(raw), CLUSTER):
                    block = raw[offset:offset + CLUSTER].ljust(CLUSTER, b"\0")
                    if any(block):
                        block_file.write_bytes(block)
                        subprocess.run(["qemu-io", "-f", "qcow", "-c",
                                        f'write -q -c -s cluster.raw {offset} {CLUSTER}',
                                        str(target)], cwd=temporary, check=True)
            else:
                subprocess.run(command + [str(source), str(target)], check=True)
            if variant == "qcow1-deflate-short":
                # QEMU accepts a full decoded cluster even when the descriptor
                # omits the final Deflate byte (inflate returns Z_BUF_ERROR).
                image = bytearray(target.read_bytes())
                l1 = struct.unpack_from(">Q", image, 40)[0]
                l2 = struct.unpack_from(">Q", image, l1)[0]
                entry = struct.unpack_from(">Q", image, l2)[0]
                struct.pack_into(">Q", image, l2, entry - (1 << (63 - image[32])))
                target.write_bytes(image)
            if image_format == "qcow2":
                subprocess.run(["qemu-img", "check", "-q", "-f", image_format, str(target)], check=True)
            subprocess.run(["qemu-img", "compare", "-q", "-f", image_format, "-F", "raw",
                            str(target), str(source)], check=True)
            image = target.read_bytes()
            name = variant + ".qcow2.gz"
            destination = args.destination / name
            if not destination.is_file() or gzip.decompress(destination.read_bytes()) != image:
                destination.write_bytes(gzip.compress(image, mtime=0))
            records.append({"file": name, "size": len(image),
                            "sha256": hashlib.sha256(image).hexdigest(),
                            "compression": compression or "stored",
                            "producer": "qemu-io" if variant.startswith("qcow1-deflate") else "qemu-img"})
    manifest = {"virtual_size": VIRTUAL_SIZE, "source_size": len(raw),
                "source_sha256": hashlib.sha256(raw).hexdigest(),
                "path": "папка 空/данные с пробелом.txt",
                "payload": "device ext4 payload\n", "images": records,
                "reference_sets": generate_references(args.destination),
                "snapshot_sets": generate_snapshots(args.destination)}
    (args.destination / "manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")


if __name__ == "__main__":
    main()
