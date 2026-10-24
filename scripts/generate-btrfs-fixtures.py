#!/usr/bin/env python3
"""Generate private Btrfs samples and verify their contents with the Linux reader."""

import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import struct
import subprocess
import sys
import tempfile
import uuid
import zlib

ROOT = Path(__file__).resolve().parents[1]
VARIANTS = {"plain": ("crc32c", None), "xxhash": ("xxhash", None),
            "sha256": ("sha256", None), "blake2": ("blake2", None),
            "zlib": ("crc32c", "zlib"), "lzo": ("crc32c", "lzo"),
            "zstd": ("crc32c", "zstd")}
MULTI_PROFILES = {"raid0": 2, "raid1": 2, "raid1c3": 3, "raid1c4": 4,
                  "raid10": 4, "raid5": 3, "raid6": 4}
PAYLOAD = bytes((i * 17 + i // 251) % 251 for i in range(400000))
DATA_NAME = "данные 空.bin"


def run(argv, **kwargs):
    return subprocess.run(argv, check=True, text=True, capture_output=True, **kwargs).stdout


def inspect_tree(root):
    files, links, attrs = {}, {}, {}
    for parent, directories, names in os.walk(root, followlinks=False):
        for name in sorted(directories + names):
            path = Path(parent) / name
            relative = str(path.relative_to(root))
            if path.is_symlink():
                links[relative] = os.readlink(path)
            elif path.is_file():
                data = path.read_bytes()
                files[relative] = {"size": len(data), "sha256": hashlib.sha256(data).hexdigest()}
            values = {key: os.getxattr(path, key, follow_symlinks=False).hex()
                      for key in os.listxattr(path, follow_symlinks=False)
                      if key.startswith("user.")}
            if values:
                attrs[relative] = values
    attrs[""] = {key: os.getxattr(root, key).hex() for key in os.listxattr(root)
                 if key.startswith("user.")}
    return {"files": files, "symlinks": links, "attributes": attrs}


def populate(root):
    (root / "nested").mkdir()
    (root / DATA_NAME).write_bytes(PAYLOAD)
    (root / "tiny.txt").write_bytes(b"inline Btrfs file\n")
    (root / "compressed-inline.txt").write_bytes(b"inline compression " * 150)
    (root / "many entries").mkdir()
    for number in range(180):
        (root / "many entries" / ("entry-%03d.txt" % number)).write_bytes(
            ("Btrfs leaf traversal %03d\n" % number).encode())
    (root / "empty").touch()
    os.link(root / DATA_NAME, root / "hardlink.bin")
    os.symlink(DATA_NAME, root / "symlink.txt")
    run(["cp", "--reflink=always", str(root / DATA_NAME), str(root / "shared.bin")])
    run(["fallocate", "--punch-hole", "--keep-size", "-o", "0", "-l", "4096", str(root / "shared.bin")])
    with (root / "nested/sparse.bin").open("wb") as file:
        file.write(b"head")
        file.seek(2 * 1024 * 1024)
        file.write(b"tail")
    run(["fallocate", "-l", "65536", str(root / "preallocated.bin")])
    with (root / "preallocated.bin").open("r+b") as file:
        file.seek(4096)
        file.write(b"written inside preallocation")
    os.setxattr(root, "user.root", b"root metadata")
    os.setxattr(root / DATA_NAME, "user.comment", b"binary\0value\xff")
    os.setxattr(root / DATA_NAME, "user.empty", b"")
    os.setxattr(root / "nested", "user.directory", b"directory metadata")
    run(["btrfs", "subvolume", "create", str(root / "subvolume")])
    (root / "subvolume/same-inode.txt").write_bytes(b"snapshot data\n")
    run(["btrfs", "subvolume", "snapshot", "-r", str(root / "subvolume"), str(root / "snapshot")])
    (root / "subvolume/same-inode.txt").write_bytes(b"live data changed\n")
    os.utime(root / DATA_NAME, ns=(1700000000123456700, 1700000000123456700))


def generate(bundle_path):
    if platform.system() != "Linux":
        raise RuntimeError("Generation requires Linux; import the resulting bundle on other hosts")
    area = ROOT / "build/btrfs-fixture-generation"
    area.mkdir(parents=True, exist_ok=True)
    result = {"creator": run(["mkfs.btrfs", "--version"]).strip(),
              "kernel": platform.release(), "images": {}}
    with tempfile.TemporaryDirectory(prefix="samples-", dir=area) as temporary:
        task = Path(temporary)
        for name, (checksum, codec) in VARIANTS.items():
            image, mountpoint = task / (name + ".img"), task / "mount"
            mountpoint.mkdir(exist_ok=True)
            with image.open("wb") as output:
                output.truncate(128 * 1024 * 1024)
            identifier = str(uuid.uuid5(uuid.NAMESPACE_URL, "sevenzip-btrfs-fixture/" + name))
            options = ["-f", "-s", "4096", "-n", "16384", "-d", "single", "-m", "dup",
                       "--csum", checksum, "-U", identifier, "-L", "Sevenzip " + name]
            run(["mkfs.btrfs", *options, str(image)])
            mount_options = "loop,nosuid,nodev,noexec" + (",compress-force=" + codec if codec else "")
            run(["sudo", "-n", "mount", "-t", "btrfs", "-o", mount_options, str(image), str(mountpoint)])
            try:
                run(["sudo", "-n", sys.executable, str(Path(__file__).resolve()), "--populate", str(mountpoint)])
                expected = json.loads(run(["sudo", "-n", sys.executable, str(Path(__file__).resolve()),
                                           "--inspect", str(mountpoint)]))
                run(["sudo", "-n", "btrfs", "filesystem", "sync", str(mountpoint)])
            finally:
                run(["sudo", "-n", "umount", str(mountpoint)])
            # Independently reopen the finished image through Linux, without replay.
            before = hashlib.sha256(image.read_bytes()).hexdigest()
            run(["sudo", "-n", "mount", "-t", "btrfs", "-o", "loop,ro,nologreplay,nosuid,nodev,noexec",
                 str(image), str(mountpoint)])
            try:
                observed = json.loads(run(["sudo", "-n", sys.executable, str(Path(__file__).resolve()),
                                           "--inspect", str(mountpoint)]))
                if observed != expected:
                    raise RuntimeError("Linux reopened contents differ: " + name)
                ids = run(["sudo", "-n", "stat", "-c", "%i", str(mountpoint / DATA_NAME),
                           str(mountpoint / "hardlink.bin"), str(mountpoint / "subvolume/same-inode.txt"),
                           str(mountpoint / "snapshot/same-inode.txt")]).splitlines()
                if ids[0] != ids[1] or ids[2] != ids[3]:
                    raise RuntimeError("Fixture does not exercise inode identity: " + name)
            finally:
                run(["sudo", "-n", "umount", str(mountpoint)])
            data = image.read_bytes()
            if hashlib.sha256(data).hexdigest() != before:
                raise RuntimeError("Read-only verification changed the source image")
            tree = run(["sudo", "-n", "btrfs", "inspect-internal", "dump-tree", "-t", "fs", str(image)])
            result["images"][name] = {"size": len(data), "sha256": before, "options": options,
                                       "expected": expected, "native_readonly": True,
                                       "tree": tree, "qcompressed": base64.b64encode(
                                           struct.pack(">I", len(data)) + zlib.compress(data, 9)).decode()}
            image.unlink()
            print("Generated and Linux-verified:", name, flush=True)
    bundle_path.parent.mkdir(parents=True, exist_ok=True)
    bundle_path.write_text(json.dumps(result, ensure_ascii=False))


def generate_multi(bundle_path):
    if platform.system() != "Linux":
        raise RuntimeError("Generation requires Linux")
    area = ROOT / "build/btrfs-fixture-generation"
    area.mkdir(parents=True, exist_ok=True)
    result = {"creator": run(["mkfs.btrfs", "--version"]).strip(),
              "kernel": platform.release(), "profiles": {}}
    with tempfile.TemporaryDirectory(prefix="multi-", dir=area) as temporary:
        task = Path(temporary)
        mountpoint = task / "mount"
        mountpoint.mkdir()
        for profile, count in MULTI_PROFILES.items():
            images = [task / ("devid-%d.img" % (i + 1)) for i in range(count)]
            loops = []
            try:
                for image in images:
                    with image.open("wb") as output:
                        output.truncate(128 * 1024 * 1024)
                    loops.append(run(["sudo", "-n", "losetup", "--find", "--show", str(image)]).strip())
                identifier = str(uuid.uuid5(uuid.NAMESPACE_URL, "sevenzip-btrfs-multi/" + profile))
                options = ["-f", "-s", "4096", "-n", "16384", "-d", profile, "-m", profile,
                           "-U", identifier, "-L", "Sevenzip " + profile]
                run(["sudo", "-n", "mkfs.btrfs", *options, *loops])
                run(["sudo", "-n", "btrfs", "device", "scan", *loops])
                run(["sudo", "-n", "mount", "-t", "btrfs", "-o", "nosuid,nodev,noexec",
                     loops[0], str(mountpoint)])
                try:
                    run(["sudo", "-n", sys.executable, str(Path(__file__).resolve()), "--populate", str(mountpoint)])
                    expected = json.loads(run(["sudo", "-n", sys.executable, str(Path(__file__).resolve()),
                                               "--inspect", str(mountpoint)]))
                    run(["sudo", "-n", "btrfs", "filesystem", "sync", str(mountpoint)])
                finally:
                    run(["sudo", "-n", "umount", str(mountpoint)])
                before = [hashlib.sha256(image.read_bytes()).hexdigest() for image in images]
                run(["sudo", "-n", "mount", "-t", "btrfs", "-o", "ro,nologreplay,nosuid,nodev,noexec",
                     loops[0], str(mountpoint)])
                try:
                    observed = json.loads(run(["sudo", "-n", sys.executable, str(Path(__file__).resolve()),
                                               "--inspect", str(mountpoint)]))
                    if observed != expected:
                        raise RuntimeError("Linux multi-device contents differ: " + profile)
                finally:
                    run(["sudo", "-n", "umount", str(mountpoint)])
                devices = {}
                for index, image in enumerate(images):
                    data = image.read_bytes()
                    if hashlib.sha256(data).hexdigest() != before[index]:
                        raise RuntimeError("Read-only verification changed a source device")
                    devices[str(index + 1)] = {"size": len(data), "sha256": before[index],
                                               "qcompressed": base64.b64encode(struct.pack(">I", len(data)) +
                                                                               zlib.compress(data, 9)).decode()}
                tree = run(["sudo", "-n", "btrfs", "inspect-internal", "dump-tree", "-t", "chunk", loops[0]])
                result["profiles"][profile] = {"devices": devices, "options": options,
                                                "expected": expected, "native_readonly": True, "tree": tree}
                print("Generated and Linux-verified multi-device:", profile, flush=True)
            finally:
                for loop in reversed(loops):
                    run(["sudo", "-n", "btrfs", "device", "scan", "--forget", loop])
                    run(["sudo", "-n", "losetup", "--detach", loop])
                for image in images:
                    image.unlink(missing_ok=True)
    bundle_path.parent.mkdir(parents=True, exist_ok=True)
    bundle_path.write_text(json.dumps(result, ensure_ascii=False))


def import_multi(path, output):
    bundle = json.loads(path.read_text())
    output = output / "multi"
    output.mkdir(parents=True, exist_ok=True)
    for profile, record in bundle["profiles"].items():
        if profile not in MULTI_PROFILES or len(record["devices"]) != MULTI_PROFILES[profile]:
            raise RuntimeError("Unknown multi-device sample")
        directory = output / profile
        directory.mkdir(exist_ok=True)
        for devid, device in record["devices"].items():
            if devid not in {str(i + 1) for i in range(MULTI_PROFILES[profile])}:
                raise RuntimeError("Unknown fixture device")
            compressed = base64.b64decode(device.pop("qcompressed"))
            data = zlib.decompress(compressed[4:])
            if len(data) != device["size"] or hashlib.sha256(data).hexdigest() != device["sha256"]:
                raise RuntimeError("Corrupted multi-device fixture bundle")
            (directory / ("devid-" + devid + ".img.qz")).write_bytes(compressed)
        review = ROOT / "build/review/btrfs"
        review.mkdir(parents=True, exist_ok=True)
        (review / (profile + "-linux-chunks.txt")).write_text(record.pop("tree"))
    (output / "manifest.json").write_text(json.dumps(bundle, ensure_ascii=False, indent=2) + "\n")


def import_bundle(path, output):
    bundle = json.loads(path.read_text())
    output.mkdir(parents=True, exist_ok=True)
    for name, image in bundle["images"].items():
        if name not in VARIANTS:
            raise RuntimeError("Unknown sample variant")
        compressed = base64.b64decode(image.pop("qcompressed"))
        data = zlib.decompress(compressed[4:])
        if len(data) != image["size"] or hashlib.sha256(data).hexdigest() != image["sha256"]:
            raise RuntimeError("Corrupted fixture bundle")
        (output / (name + ".img.qz")).write_bytes(compressed)
        # Keep the original tool output as review evidence, outside the test data.
        review = ROOT / "build/review/btrfs"
        review.mkdir(parents=True, exist_ok=True)
        (review / (name + "-linux-tree.txt")).write_text(image.pop("tree"))
    (output / "manifest.json").write_text(json.dumps(bundle, ensure_ascii=False, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", action="store_true")
    parser.add_argument("--populate", type=Path, help=argparse.SUPPRESS)
    parser.add_argument("--inspect", type=Path, help=argparse.SUPPRESS)
    parser.add_argument("--bundle", type=Path)
    parser.add_argument("--multi-bundle", type=Path)
    parser.add_argument("--import-bundle", type=Path)
    parser.add_argument("--import-multi", type=Path)
    parser.add_argument("--output", type=Path, default=ROOT / "tests/fixtures/btrfs")
    args = parser.parse_args()
    if args.probe:
        print(json.dumps({"mkfs": shutil.which("mkfs.btrfs"), "btrfs": shutil.which("btrfs"),
                          "sudo": subprocess.run(["sudo", "-n", "true"]).returncode,
                          "kernel": platform.release()}))
    elif args.populate:
        populate(args.populate)
    elif args.inspect:
        print(json.dumps(inspect_tree(args.inspect), ensure_ascii=False))
    elif args.import_multi:
        import_multi(args.import_multi, args.output)
    elif args.multi_bundle:
        generate_multi(args.multi_bundle)
    elif args.import_bundle:
        import_bundle(args.import_bundle, args.output)
    elif args.bundle:
        generate(args.bundle)
    else:
        parser.error("Select --probe, --bundle or --import-bundle")


if __name__ == "__main__":
    main()
