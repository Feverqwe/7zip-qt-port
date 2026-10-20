#!/usr/bin/env python3
"""Generate deterministic content with e2fsprogs/Linux; validate read-only ext4."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import struct
import subprocess
import tempfile
import zlib


def run(args):
    result = subprocess.run(args, capture_output=True, check=True)
    return result.stdout.decode(errors="replace")


def generate():
    with tempfile.TemporaryDirectory(prefix="ext-xattr-", dir=os.environ.get("TMPDIR")) as temporary:
        root = Path(temporary).resolve()
        source = root / "source"
        source.mkdir()
        (source / "nested").mkdir()
        (source / "data.bin").write_bytes(bytes(range(100)))
        os.link(source / "data.bin", source / "alias.bin")
        (source / "data.bin:$XATTR.user.comment").write_bytes(b"ordinary colon file")
        image = root / "image.ext4"
        with image.open("wb") as output:
            output.truncate(32 * 1024 * 1024)
        options = ["-F", "-q", "-t", "ext4", "-b", "1024", "-I", "256", "-O", "ea_inode",
                   "-U", "12345678-1234-5678-1234-567812345678"]
        run(["mke2fs", *options, "-d", str(source), str(image)])
        def debug(command):
            return run(["debugfs", "-R", command, str(image)])
        values = {"user.comment": b"binary\0xattr\xff", "user.empty": b"",
                  "user.case": b"lower", "user.CASE": b"upper",
                  "user.slash/name:evil": b"escaped name", "user.имя": b"Unicode name",
                  "user.large": bytes(i % 251 for i in range(9000)),
                  "user.данные": bytes(i % 251 for i in range(9000)),
                  "trusted.fixture": b"trusted metadata", "security.fixture": b"security metadata",
                  "gnu.fixture": b"Hurd metadata"}
        def acl(entries):
            return struct.pack("<I", 2) + b"".join(struct.pack("<HHI", *entry) for entry in entries)
        access = acl(((1, 7, 0xffffffff), (2, 5, 2067), (4, 5, 0xffffffff),
                      (16, 5, 0xffffffff), (32, 1, 0xffffffff)))
        default = acl(((1, 7, 0xffffffff), (4, 5, 0xffffffff), (32, 0, 0xffffffff)))
        attributes = {"data.bin": dict(values, **{"system.posix_acl_access": access}),
                      "nested": {"user.directory": b"directory", "system.posix_acl_default": default},
                      "": {"user.root": b"root metadata"}}
        attributes["alias.bin"] = attributes["data.bin"]
        expected = {path: {name: value.hex()
                           for name, value in attrs.items()} for path, attrs in attributes.items()}
        target = root / "mount"
        target.mkdir()
        run(["sudo", "-n", "mount", "-t", "ext4", "-o", "loop,nosuid,nodev,noexec", str(image), str(target)])
        try:
            writer = """import json,os,sys
from pathlib import Path
root=Path(sys.argv[1])
for path,attrs in json.load(sys.stdin).items():
 for name,value in attrs.items(): os.setxattr(root/path,name,bytes.fromhex(value))
"""
            result = subprocess.run(["sudo", "-n", "python3", "-c", writer, str(target)],
                                    input=json.dumps(expected), text=True, capture_output=True)
            if result.returncode: raise RuntimeError(result.stderr)
        finally:
            run(["sudo", "-n", "umount", str(target)])
        data = image.read_bytes()
        before = hashlib.sha256(data).hexdigest()
        run(["sudo", "-n", "mount", "-t", "ext4", "-o", "ro,loop,nosuid,nodev,noexec", str(image), str(target)])
        try:
            child = """import hashlib,json,os,sys
from pathlib import Path
root=Path(sys.argv[1]);expected=json.load(sys.stdin);results={}
for path,attrs in expected.items():
 p=root/path;actual={}
 for name in attrs:
  try: actual[name]=os.getxattr(p,name).hex()
  except OSError as error: raise RuntimeError((path,name,str(error))) from error
 results[path]={'matches':actual==attrs,'names':os.listxattr(p),'inode':os.stat(p).st_ino,
                'different':{name:len(value)//2 for name,value in actual.items() if value!=attrs[name]}}
print(json.dumps(results,ensure_ascii=False))
assert all(v['matches'] for v in results.values())
"""
            result = subprocess.run(["sudo", "-n", "python3", "-c", child, str(target)],
                                    input=json.dumps(expected), text=True, capture_output=True)
            if result.returncode:
                raise RuntimeError(result.stdout + result.stderr)
            native = json.loads(result.stdout)
        finally:
            run(["sudo", "-n", "umount", str(target)])
        assert hashlib.sha256(image.read_bytes()).hexdigest() == before
        assert native["data.bin"]["inode"] == native["alias.bin"]["inode"]
        def inode_offset(ino):
            location = debug(f"imap <{ino}>")
            match = re.search(r"located at block (\d+), offset (0x[0-9a-fA-F]+)", location)
            assert match, location
            return int(match[1]) * 1024 + int(match[2], 16)
        parent = native["data.bin"]["inode"]
        offset = inode_offset(parent)
        entries = {}
        def scan(start, end, block):
            while start + 16 < end and struct.unpack_from("<I", data, start)[0]:
                length, index, value_offset, ino, size, ehash = struct.unpack_from("<BBHIII", data, start)
                name = data[start + 16:start + 16 + length].decode()
                entries[f"{index}.{name}"] = {"offset": start, "inode": ino, "size": size, "block": block}
                start += (16 + length + 3) & ~3
        extra = struct.unpack_from("<H", data, offset + 128)[0]
        header = offset + 128 + extra
        if struct.unpack_from("<I", data, header)[0] == 0xea020000:
            scan(header + 4, offset + 256, 0)
        block = struct.unpack_from("<I", data, offset + 104)[0]
        if block:
            scan(block * 1024 + 32, (block + 1) * 1024, block)
        ea = entries["1.large"]["inode"]
        assert ea and entries["1.данные"]["inode"]
        for entry in entries.values():
            if entry["inode"]: entry["inode_offset"] = inode_offset(entry["inode"])
        ea_offset = inode_offset(ea)
        record = {"creator": subprocess.run(["mke2fs", "-V"], capture_output=True, text=True).stderr.strip(),
                  "kernel": platform.release(), "options": options, "size": len(data), "sha256": before,
                  "native": native, "attributes": expected, "entries": entries,
                  "parent_inode": parent, "parent_offset": offset, "ea_inode": ea,
                  "ea_offset": ea_offset}
        return {"compressed": base64.b64encode(zlib.compress(data, 9)).decode(), "record": record}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--output", type=Path, default=Path("tests/fixtures/ext4"))
    args = parser.parse_args()
    result = generate()
    if args.json:
        print(json.dumps(result, ensure_ascii=False))
    else:
        args.output.mkdir(parents=True, exist_ok=True)
        (args.output / "xattrs.ext4.zlib").write_bytes(base64.b64decode(result["compressed"]))
        manifest = args.output / "manifest.json"
        record = json.loads(manifest.read_text()) if manifest.exists() else {"images": {}}
        record["images"]["xattrs"] = result["record"]
        manifest.write_text(json.dumps(record, ensure_ascii=False, indent=2) + "\n")
