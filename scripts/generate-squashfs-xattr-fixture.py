#!/usr/bin/env python3
"""Create and natively verify a compact mksquashfs xattr fixture on Linux."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import tempfile
import zlib


def generate():
    with tempfile.TemporaryDirectory(prefix="squashfs-xattr-", dir=os.environ.get("TMPDIR")) as temporary:
        root = Path(temporary).resolve()
        source = root / "source"
        source.mkdir()
        (source / "nested").mkdir()
        (source / "data.bin").write_bytes(bytes(range(100)))
        os.link(source / "data.bin", source / "alias.bin")
        (source / "[ROOT]").write_bytes(b"ordinary root name")
        (source / "data.bin:$XATTR.user.comment").write_bytes(b"ordinary colon file")
        values = {"user.comment": b"binary\0value\xff", "user.empty": b"",
                  "user.CASE": b"upper", "user.case": b"lower",
                  "user.slash/name:evil": b"escaped name", "user.имя": b"Unicode value",
                  "user.large": bytes(i % 251 for i in range(9000)),
                  "user.duplicate": bytes(i % 251 for i in range(9000)),
                  "trusted.fixture": b"trusted metadata", "security.fixture": b"security metadata"}
        attributes = {"": {"user.root": b"root metadata"},
                      "nested": {"user.directory": b"directory metadata"},
                      "data.bin": values, "alias.bin": values}
        expected = {path: {key: value.hex() for key, value in attrs.items()}
                    for path, attrs in attributes.items()}
        # A normal host ext4 volume may lack EA_INODE and reject 9000-byte xattrs.
        backing = root / "source.ext4"
        with backing.open("wb") as output:
            output.truncate(32 * 1024 * 1024)
        subprocess.run(["mke2fs", "-F", "-q", "-t", "ext4", "-O", "ea_inode",
                        "-d", str(source), str(backing)], capture_output=True, check=True)
        mounted_source = root / "source-mount"
        mounted_source.mkdir()
        subprocess.run(["sudo", "-n", "mount", "-t", "ext4", "-o",
                        "loop,nosuid,nodev,noexec", str(backing), str(mounted_source)], check=True)
        writer = """import json,os,sys
from pathlib import Path
root=Path(sys.argv[1])
for path,attrs in json.load(sys.stdin).items():
 for name,value in attrs.items():os.setxattr(root/path,name,bytes.fromhex(value))
"""
        image = root / "image.squashfs"
        options = ["-noappend", "-processors", "1", "-comp", "gzip", "-b", "8192",
                   "-all-time", "1700000000", "-mkfs-time", "1700000000", "-all-root", "-no-progress"]
        try:
            result = subprocess.run(["sudo", "-n", "python3", "-c", writer, str(mounted_source)],
                                    input=json.dumps(expected), text=True, capture_output=True)
            if result.returncode: raise RuntimeError(result.stderr)
            subprocess.run(["sudo", "-n", "mksquashfs", str(mounted_source), str(image), *options],
                           capture_output=True, check=True)
        finally:
            subprocess.run(["sudo", "-n", "umount", str(mounted_source)], check=True)
        data = image.read_bytes()
        before = hashlib.sha256(data).hexdigest()
        target = root / "mount"
        target.mkdir()
        subprocess.run(["sudo", "-n", "mount", "-t", "squashfs", "-o",
                        "ro,loop,nosuid,nodev,noexec", str(image), str(target)], check=True)
        try:
            reader = """import json,os,sys
from pathlib import Path
root=Path(sys.argv[1]);expected=json.load(sys.stdin)
for path,attrs in expected.items():
 for name,value in attrs.items():
  assert os.getxattr(root/path,name)==bytes.fromhex(value),(path,name)
assert os.stat(root/'data.bin').st_ino==os.stat(root/'alias.bin').st_ino
print(json.dumps({'values':sum(map(len,expected.values())),'hardlink':True}))
"""
            result = subprocess.run(["sudo", "-n", "python3", "-c", reader, str(target)],
                                    input=json.dumps(expected), text=True, capture_output=True, check=True)
            native = json.loads(result.stdout)
        finally:
            subprocess.run(["sudo", "-n", "umount", str(target)], check=True)
        assert hashlib.sha256(image.read_bytes()).hexdigest() == before
        record = {"size": len(data), "sha256": before, "options": options,
                  "creator": subprocess.run(["mksquashfs", "-version"], capture_output=True,
                                             text=True).stdout.splitlines()[0],
                  "kernel": platform.release(), "native": native, "attributes": expected}
        return {"record": record, "compressed": base64.b64encode(zlib.compress(data, 9)).decode()}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", type=Path, help="Write the fixture bundle for transport")
    args = parser.parse_args()
    bundle = generate()
    if args.json:
        args.json.write_text(json.dumps(bundle))
        print("Native xattr verification:", bundle["record"]["native"])
    else:
        fixtures = Path(__file__).resolve().parents[1] / "tests/fixtures/squashfs"
        (fixtures / "xattr-values.squashfs.zlib").write_bytes(base64.b64decode(bundle["compressed"]))
        path = fixtures / "manifest.json"
        manifest = json.loads(path.read_text())
        manifest["images"]["xattr-values"] = bundle["record"]
        path.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")
