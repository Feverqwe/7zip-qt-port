"""Create Linux namespace/ACL fixtures through official mkfs.erofs PAX input."""
import base64
import hashlib
import io
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tarfile
import tempfile
import urllib.parse
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from erofs_fixture import FILES, PAYLOAD_NAME


def generate(tool, output, manifest):
    def acl(entries):
        return struct.pack("<I", 2) + b"".join(struct.pack("<HHI", *entry) for entry in entries)
    values = {"user.binary": b"binary\0value\xff", "user.empty": b"",
              "user.CASE": b"upper", "user.case": b"lower",
              "user.slash/name:evil": b"escaped name", "user.имя": b"Unicode value",
              "user.fixture.long.attribute": b"long prefix value",
              "user.large": bytes(i % 251 for i in range(9000)),
              "trusted.fixture": b"trusted metadata", "security.fixture": b"security metadata",
              "system.posix_acl_access": acl(((1, 7, 0xffffffff), (2, 5, 2067),
                                               (4, 5, 0xffffffff), (8, 4, 2099),
                                               (16, 5, 0xffffffff), (32, 1, 0xffffffff)))}
    attributes = {"": {"user.root": b"root metadata"}, PAYLOAD_NAME: values,
                  "duplicate.bin": values,
                  "папка": {"user.directory": b"directory metadata",
                            "system.posix_acl_default": acl(((1, 7, 0xffffffff),
                                                              (4, 5, 0xffffffff),
                                                              (32, 0, 0xffffffff)))}}
    with tempfile.TemporaryDirectory(prefix="erofs-xattr-creator-") as temporary:
        root = Path(temporary)
        archive = root / "source.tar"
        with tarfile.open(archive, "w", format=tarfile.PAX_FORMAT) as tar:
            def add(name, kind, data=b"", link=""):
                item = tarfile.TarInfo(name or ".")
                item.type, item.linkname = kind, link
                item.uid, item.gid, item.mtime = 1234, 5678, 1700000000
                item.mode = 0o755 if kind == tarfile.DIRTYPE else 0o644
                item.size = len(data)
                item.pax_headers = {"LIBARCHIVE.xattr." + urllib.parse.quote(key, safe="._-"):
                                    base64.b64encode(value).decode()
                                    for key, value in attributes.get(name, {}).items()}
                tar.addfile(item, io.BytesIO(data))
            add("", tarfile.DIRTYPE)
            add("папка", tarfile.DIRTYPE)
            add(PAYLOAD_NAME, tarfile.REGTYPE, FILES[PAYLOAD_NAME])
            for name, data in FILES.items():
                if name not in (PAYLOAD_NAME, "duplicate.bin"):
                    add(name, tarfile.REGTYPE, data)
            add("duplicate.bin", tarfile.LNKTYPE, link=PAYLOAD_NAME)
            add("link", tarfile.SYMTYPE, link=PAYLOAD_NAME)
            add("[ROOT]", tarfile.REGTYPE, b"ordinary root name")
        variants = {"xattr-values-inline": ["-x99999"],
                    "xattr-values-plain-prefix": ["-x0", "--xattr-prefix=user.fixture.long.",
                                                   "-Eplain-xattr-prefixes"],
                    "xattr-values-prefix": ["-x0", "-zlz4", "-Efragments",
                                              "--xattr-prefix=user.fixture.long."],
                    "xattr-values-metabox": ["-x0", "-zlz4", "-m4096",
                                               "--xattr-prefix=user.fixture.long."]}
        shared_source = root / "source"
        shared_source.mkdir()
        (shared_source / "папка").mkdir()
        for name, data in FILES.items():
            path = shared_source / name
            if name == "duplicate.bin":
                os.link(shared_source / PAYLOAD_NAME, path)
            else:
                path.write_bytes(data)
        (shared_source / "link").symlink_to(PAYLOAD_NAME)
        (shared_source / "[ROOT]").write_bytes(b"ordinary root name")
        shared_attributes = {path: {key: value for key, value in attrs.items() if key.startswith("user.")}
                             for path, attrs in attributes.items()}
        for path, attrs in shared_attributes.items():
            for key, value in attrs.items():
                if hasattr(os, "setxattr"):
                    os.setxattr(shared_source / path, key, value)
                else:
                    subprocess.run(["xattr", "-wx", key, value.hex(), str(shared_source / path)], check=True)
        variants["xattr-values-shared"] = ["-x0"]
        variants["xattr-values-shared-metabox"] = ["-x0", "-zlz4", "-m4096",
                                                   "--xattr-prefix=user.fixture.long."]
        for name, options in variants.items():
            image = root / (name + ".erofs")
            shared = name in ("xattr-values-shared", "xattr-values-shared-metabox")
            command = [str(tool), *([] if shared else ["--tar=f"]), "-b4096", "-T1700000000",
                       "-U00000000-0000-0000-0000-000000000001", "-LEROFS fixture",
                       "--force-uid=1234", "--force-gid=5678",
                       "--quiet", *options, str(image), str(shared_source if shared else archive)]
            result = subprocess.run(command, capture_output=True)
            if result.returncode: raise RuntimeError(result.stderr.decode(errors="replace"))
            data = image.read_bytes()
            (output / (name + ".erofs.zlib")).write_bytes(zlib.compress(data, 9))
            manifest["images"][name] = {
                "options": command[1:-2], "size": len(data),
                "sha256": hashlib.sha256(data).hexdigest(),
                "attributes": {path: {key: value.hex() for key, value in attrs.items()}
                               for path, attrs in (shared_attributes if shared else attributes).items()}}


if __name__ == "__main__":
    tool, output = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()
    manifest = json.loads((output / "manifest.json").read_text())
    generate(tool, output, manifest)
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")
