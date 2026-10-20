#!/usr/bin/env python3
"""Create public synthetic fscrypt data with Linux ext4 as the byte oracle.

Requires Linux, e2fsprogs and scoped sudo for a disposable loop mount. No keys
or data from the host are used. Unsupported native modes are recorded explicitly.
"""
import argparse
import base64
import errno
import fcntl
import hashlib
import json
import os
from pathlib import Path
import platform
import struct
import subprocess
import sys
import tempfile
import zlib


UUID = "24681357-1234-5678-9abc-0123456789ab"
BLOCK = 4096
# name, policy version, contents mode, filename mode, flags, data-unit bits
CASES = (("v1-aes", 0, 1, 4, 3, 0), ("v1-essiv", 0, 5, 6, 1, 0),
         ("v1-adiantum", 0, 9, 9, 0, 0), ("v1-direct", 0, 9, 9, 4, 0),
         ("v2-aes", 2, 1, 4, 2, 0), ("v2-essiv", 2, 5, 6, 0, 0),
         ("v2-adiantum", 2, 9, 9, 3, 0), ("v2-direct", 2, 9, 9, 4, 0),
         ("v2-ino64", 2, 1, 4, 8, 0), ("v2-ino32", 2, 1, 4, 16, 0),
         ("v2-hctr2", 2, 1, 10, 0, 0), ("v2-sm4", 2, 7, 8, 0, 0),
         ("v2-subblock", 2, 1, 4, 0, 9), ("v2-casefold", 2, 1, 4, 0, 0),
         ("v2-verity", 2, 1, 4, 0, 0))


def run(args):
    return subprocess.run(args, check=True, capture_output=True).stdout


def populate(target):
    """Run in the mounted synthetic filesystem as root, using native ioctls."""
    result = {"cases": [], "unsupported": []}
    rootfd = os.open(target, os.O_RDONLY | os.O_DIRECTORY)
    try:
        for name, version, contents, names, flags, du_bits in CASES:
            directory = target / name
            directory.mkdir()
            if version == 0:
                key_size = 64 if contents == 1 else 32 if contents == 9 else 16
            else:
                key_size = 64 if name == "v2-subblock" else 16 if contents in (5, 7) else 32
            raw = bytes((i * 13 + len(name)) % 256 for i in range(key_size))
            descriptor = hashlib.sha256(name.encode()).digest()[:8]
            keytype = 1 if version == 0 else 2
            arg = bytearray(80 + len(raw))
            struct.pack_into("<I", arg, 0, keytype)
            if keytype == 1:
                arg[8:16] = descriptor
            struct.pack_into("<I", arg, 40, len(raw))
            arg[80:] = raw
            fcntl.ioctl(rootfd, 0xc0506617, arg, True)  # FS_IOC_ADD_ENCRYPTION_KEY
            identity = arg[8:16 if keytype == 1 else 24]
            policy = bytes((version, contents, names, flags))
            policy += identity if version == 0 else bytes((du_bits, 0, 0, 0)) + identity
            fd = os.open(directory, os.O_RDONLY | os.O_DIRECTORY)
            try:
                if name == "v2-casefold":
                    fcntl.ioctl(fd, 0x40086602, struct.pack("<Q", 0x40000000))  # FS_IOC_SETFLAGS
                fcntl.ioctl(fd, 0x800c6613, policy)  # FS_IOC_SET_ENCRYPTION_POLICY
                filename = "данные spaces: " + "a" * 160
                file = directory / filename
                plain = bytes((i * 17 + 31) % 251 for i in range(BLOCK))
                with file.open("wb") as stream:
                    stream.write(plain)
                    stream.seek(2 * BLOCK)
                    stream.write(b"end" + bytes(BLOCK - 3))
                    stream.truncate(2 * BLOCK + 3)
                    stream.flush()
                    os.fsync(stream.fileno())
                    mapping = bytearray(32 + 8 * 56)
                    struct.pack_into("<QQIIII", mapping, 0, 0, (1 << 64) - 1, 1, 0, 8, 0)
                    fcntl.ioctl(stream.fileno(), 0xc020660b, mapping, True)  # FS_IOC_FIEMAP
                if name == "v2-verity":
                    verity = struct.pack("<IIII", 1, 1, BLOCK, 0) + bytes(112)
                    verifyfd = os.open(file, os.O_RDONLY)
                    try:
                        fcntl.ioctl(verifyfd, 0x40806685, verity)  # FS_IOC_ENABLE_VERITY
                    finally:
                        os.close(verifyfd)
                expected = plain + bytes(BLOCK) + b"end"
                assert file.read_bytes() == expected
                mapped = struct.unpack_from("<I", mapping, 20)[0]
                extents = []
                for i in range(mapped):
                    start = 32 + i * 56
                    logical, physical, length = struct.unpack_from("<QQQ", mapping, start)
                    extent_flags = struct.unpack_from("<I", mapping, start + 40)[0]
                    assert not extent_flags & (2 | 4 | 0x800), extent_flags
                    extents.append([logical, physical, length])
                link_target = "relative/цель spaces:" + "b" * 100
                os.symlink(link_target, directory / "link")
                result["cases"].append({"name": name, "raw_key": raw.hex(),
                    "key_type": keytype, "descriptor": descriptor.hex(),
                    "identity": identity.hex(), "policy": policy.hex(),
                    "casefolded": name == "v2-casefold", "verity": name == "v2-verity",
                    "directory_inode": os.stat(directory).st_ino,
                    "file_inode": os.stat(file).st_ino,
                    "symlink_inode": os.lstat(directory / "link").st_ino,
                    "filename": filename, "symlink_target": link_target,
                    "extents": extents, "plain_first": plain.hex(),
                    "native_sha256": hashlib.sha256(expected).hexdigest()})
            except OSError as error:
                if error.errno not in (errno.EINVAL, errno.ENOPKG, errno.EOPNOTSUPP):
                    raise
                result["unsupported"].append({"name": name, "errno": error.errno,
                                               "error": str(error)})
            finally:
                os.close(fd)
    finally:
        os.close(rootfd)
    return result


def generate(output):
    with tempfile.TemporaryDirectory(prefix="sevenzip-fscrypt-") as temp:
        root = Path(temp)
        image, mount = root / "fscrypt.ext4", root / "mount"
        mount.mkdir()
        with image.open("wb") as stream:
            stream.truncate(32 * 1024 * 1024)
        options = ["-q", "-F", "-t", "ext4", "-b", str(BLOCK), "-I", "256",
                   "-O", "encrypt,stable_inodes,casefold,verity,^huge_file", "-E", "encoding=utf8",
                   "-U", UUID]
        run(["mke2fs", *options, str(image)])
        run(["sudo", "-n", "mount", "-t", "ext4", "-o", "loop,nosuid,nodev,noexec",
             str(image), str(mount)])
        try:
            result = json.loads(run(["sudo", "-n", sys.executable, str(Path(__file__).resolve()),
                                     "--populate", str(mount)]))
        finally:
            run(["sudo", "-n", "umount", str(mount)])
        data = image.read_bytes()
        # Re-mount without adding keys: Linux itself supplies opaque names and
        # symlink presentation, independently of the transferred implementation.
        run(["sudo", "-n", "mount", "-t", "ext4", "-o", "ro,loop,nosuid,nodev,noexec",
             str(image), str(mount)])
        try:
            for case in result["cases"]:
                directory = mount / case["name"]
                for entry in directory.iterdir():
                    ino = entry.lstat().st_ino
                    if ino == case["file_inode"]:
                        case["nokey_name"] = entry.name
                    elif ino == case["symlink_inode"]:
                        case["nokey_symlink_target"] = os.readlink(entry)
                assert "nokey_name" in case and "nokey_symlink_target" in case
        finally:
            run(["sudo", "-n", "umount", str(mount)])
        assert image.read_bytes() == data, "read-only native inspection changed the image"

        def debug_dump(ino, context=False):
            destination = root / "dump"
            destination.unlink(missing_ok=True)
            # e2fsprogs exposes the private encryption namespace as bare 'c'.
            command = f"ea_get -f {destination} <{ino}> c" if context else \
                      f"dump <{ino}> {destination}"
            run(["debugfs", "-R", command, str(image)])
            value = destination.read_bytes()
            if context and (len(value) not in (28, 40) or value[0] not in (1, 2)):
                raise RuntimeError(f"debugfs did not retrieve the encryption context for inode {ino}")
            return value

        for case in result["cases"]:
            for role in ("directory", "file", "symlink"):
                case[role + "_context"] = debug_dump(case[role + "_inode"], True).hex()
            case["symlink_payload"] = debug_dump(case["symlink_inode"]).hex()
            directory = debug_dump(case["directory_inode"])
            offset = 0
            while offset + 8 <= len(directory):
                ino, length, namelen, _ = struct.unpack_from("<IHBB", directory, offset)
                if length < 8 or offset + length > len(directory):
                    raise RuntimeError("invalid native directory record")
                if ino == case["file_inode"]:
                    case["cipher_name"] = directory[offset + 8:offset + 8 + namelen].hex()
                offset += length
            assert "cipher_name" in case
            units = []
            unit_size = 1 << (bytes.fromhex(case["policy"])[4] or 12) if case["key_type"] == 2 else BLOCK
            for logical in (0, 2 * BLOCK):
                physical = next(p + logical - l for l, p, n in case["extents"] if l <= logical < l + n)
                for pos in range(0, BLOCK, unit_size):
                    # The truncated tail only has its first data unit allocated as ciphertext.
                    if logical and pos:
                        break
                    plain = bytes.fromhex(case["plain_first"])[pos:pos + unit_size] if not logical else \
                            b"end" + bytes(unit_size - 3)
                    units.append({"index": (logical + pos) // unit_size, "plain": plain.hex(),
                                  "cipher": data[physical + pos:physical + pos + unit_size].hex()})
            case["units"] = units
        result.update({"kernel": platform.release(), "uuid": UUID, "block_bits": 12,
                       "max_file_size": ((1 << 32) - 1) // 8 * BLOCK,
                       "options": options, "image_size": len(data), "image_sha256": hashlib.sha256(data).hexdigest()})
        output.mkdir(parents=True, exist_ok=True)
        (output / "native.ext4.zlib").write_bytes(zlib.compress(data, 9))
        (output / "native.json").write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n")
        print(json.dumps({"cases": [c["name"] for c in result["cases"]],
                          "unsupported": result["unsupported"], "output": str(output)}))


def emit_header(manifest, destination):
    """Make the standalone C checks consume the same native archive oracle."""
    record = json.loads(manifest.read_text())
    lines = ["/* Generated by scripts/generate-fscrypt-fixture.py --emit-header.",
             " * Public synthetic Linux ext4 inputs; not user keys or data. */"]

    def literal(value):
        return '"' + ''.join(f"\\x{b:02x}" for b in value) + '"'

    def byte_array(name, value):
        lines.append(f"static const unsigned char {name}[] =")
        lines.extend(literal(value[pos:pos + 24]) for pos in range(0, len(value), 24))
        lines.append(";")

    for i, case in enumerate(record["cases"]):
        for j, unit in enumerate(case["units"]):
            byte_array(f"native_cipher_{i}_{j}", bytes.fromhex(unit["cipher"]))
        lines.append(f"static const struct fscrypt_native_unit native_units_{i}[] = {{")
        for j, unit in enumerate(case["units"]):
            digest = hashlib.sha256(bytes.fromhex(unit["plain"])).digest()
            lines.append(f"{{{unit['index']}, {len(bytes.fromhex(unit['cipher']))}, "
                         f"native_cipher_{i}_{j}, {literal(digest)}}},")
        lines.append("};")
    lines.append("static const struct fscrypt_native_vector native_vectors[] = {")
    for i, case in enumerate(record["cases"]):
        lines.append("{")
        lines.append(f".name = {json.dumps(case['name'])}, .key_type = {case['key_type']},")
        lines.append(f".casefolded = {int(case.get('casefolded', False))},")
        # The native htree reader supplies these dirhash values even for a
        # single-block directory; they are input metadata for the name reader.
        decoded = base64.urlsafe_b64decode(case["nokey_name"] + "===")
        major, minor = struct.unpack_from("<II", decoded)
        lines.append(f".hash = UINT32_C({major}), .minor_hash = UINT32_C({minor}),")
        for field in ("raw_key", "descriptor", "file_context", "directory_context", "symlink_context",
                      "cipher_name", "symlink_payload"):
            value = bytes.fromhex(case[field])
            lines.append(f".{field} = {literal(value)}, .{field}_size = {len(value)},")
        for field in ("file_inode", "directory_inode", "symlink_inode"):
            lines.append(f".{field} = {case[field]},")
        for field in ("filename", "symlink_target", "nokey_name", "nokey_symlink_target"):
            lines.append(f".{field} = {literal(case[field].encode())},")
        lines.append(f".units = native_units_{i}, .unit_count = {len(case['units'])},")
        lines.append("},")
    lines.append("};")
    lines.append(f"static const uint64_t native_max_file_size = UINT64_C({record['max_file_size']});")
    import uuid
    byte_array("native_uuid", uuid.UUID(record["uuid"]).bytes)
    destination.write_text('\n'.join(lines) + '\n')


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--populate", type=Path, help=argparse.SUPPRESS)
    parser.add_argument("--output", type=Path, default=Path("tests/fixtures/fscrypt"))
    parser.add_argument("--emit-header", type=Path, metavar="MANIFEST")
    args = parser.parse_args()
    if args.emit_header:
        emit_header(args.emit_header, args.output)
    elif args.populate:
        print(json.dumps(populate(args.populate), ensure_ascii=False))
    else:
        generate(args.output)
