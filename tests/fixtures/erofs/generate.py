"""Regenerate small EROFS images using the official mkfs.erofs."""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import struct
import sys
import tempfile
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from erofs_fixture import FILES, PAYLOAD, PAYLOAD_NAME

VARIANTS = {
    "plain": [],
    "plain-512": ["-b512"],
    "plain-64k": ["-b65536"],
    "lz4": ["-zlz4"],
    "lz4-subpage": ["-b512", "-zlz4", "-C16384"],
    "lz4-64k": ["-b65536", "-zlz4"],
    "lz4-full": ["-zlz4", "-Elegacy-compress"],
    "lz4-big": ["-zlz4", "-C8192"],
    "lzma": ["-zlzma"],
    "lzma-full": ["-zlzma", "-Elegacy-compress"],
    "deflate": ["-zdeflate"],
    "deflate-full": ["-zdeflate", "-Elegacy-compress"],
    "zstd": ["-zzstd"],
    "zstd-full": ["-zzstd", "-Elegacy-compress"],
    "fragments": ["-zlz4", "-Efragments"],
    "all-fragments": ["-zlz4", "-Eall-fragments"],
    "ztailpacking": ["-zlz4", "-Eztailpacking"],
    "metabox": ["-zlz4", "-m4096"],
    "chunks": ["--chunksize=4096"],
    "chunk-indexes": ["--chunksize=4096", "-Eforce-chunk-indexes"],
    "48bit": ["-zlz4", "-E48bit"],
    "compressed-directory": ["-zlz4", "-m4096", "--zD=1"],
    "xattrs": ["-zlz4", "-Efragments", "-x0", "--xattr-prefix=user.fixture.long."],
}


def generate(tool, output):
    output.mkdir(parents=True, exist_ok=True)
    version = subprocess.run([str(tool), "--version"], check=True, capture_output=True,
                             text=True).stdout.strip()
    manifest = {"version": version, "repository":
                "https://git.kernel.org/pub/scm/linux/kernel/git/xiang/erofs-utils.git",
                "revision": "f36cadb5c563995ab3aa8572a60ed6b721b9557d",
                "payload_size": len(PAYLOAD), "images": {}}
    (output / "payload.bin.zlib").write_bytes(zlib.compress(PAYLOAD, 9))
    def save(name, data, options, **extra):
        (output / (name + ".erofs.zlib")).write_bytes(zlib.compress(data, 9))
        manifest["images"][name] = {"options": options, "size": len(data),
                                    "sha256": hashlib.sha256(data).hexdigest(), **extra}
    common = ["-b4096", "-T1700000000", "-U00000000-0000-0000-0000-000000000001",
              "--force-uid=1234", "--force-gid=5678", "-LEROFS fixture", "-x-1", "--quiet"]
    with tempfile.TemporaryDirectory(prefix="sevenzip-erofs-creator-") as temporary:
        root = Path(temporary) / "root"
        root.mkdir()
        for name, contents in FILES.items():
            path = root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            if name == "duplicate.bin":
                os.link(root / PAYLOAD_NAME, path)
            else:
                if name == "sparse.bin":
                    with path.open("wb") as sparse:
                        sparse.write(contents[:4096])
                        sparse.seek(len(contents) - 4)
                        sparse.write(contents[-4:])
                else:
                    path.write_bytes(contents)
                path.chmod(0o644)
        (root / "link").symlink_to(PAYLOAD_NAME)
        for path in [root, root / "папка"]:
            path.chmod(0o755)
        for path in [root, root / "папка", *(root / name for name in FILES)]:
            for key, value in (("user.fixture.shared", b"shared metadata"),
                               ("user.fixture.long.attribute", b"long prefix metadata")):
                if hasattr(os, "setxattr"):
                    os.setxattr(path, key, value)
                else:
                    subprocess.run(["xattr", "-w", key, value.decode(), str(path)], check=True)
        for name, options in VARIANTS.items():
            image = Path(temporary) / (name + ".erofs")
            command = [str(tool), *common, *options, str(image), str(root)]
            result = subprocess.run(command, capture_output=True, text=True)
            if result.returncode:
                raise RuntimeError(f"{name}: {result.stderr}")
            data = image.read_bytes()
            save(name, data, options)
        primary, blob = Path(temporary) / "external.erofs", Path(temporary) / "blob.bin"
        options = ["--chunksize=4096", "--blobdev=blob.bin"]
        subprocess.run([str(tool), *common, "--chunksize=4096", "--blobdev=" + str(blob),
                        str(primary), str(root)], check=True, capture_output=True)
        data, extra = bytearray(primary.read_bytes()), blob.read_bytes()
        slot = struct.unpack_from("<H", data, 1024 + 88)[0] * 128
        uniaddr = struct.unpack_from("<I", data, slot + 68)[0] * 4096
        unified = data + bytes(max(0, uniaddr + len(extra) - len(data)))
        unified[uniaddr:uniaddr + len(extra)] = extra
        save("unified-device", unified, options, transformation="append blob at device unified address")
        data[slot:slot + 64] = b"blob.bin" + bytes(56)
        # Linux's checksum contract excludes the preverified magic/checksum words.
        crc = 0x5045B54A
        for byte in data[1032:4096]:
            crc ^= byte
            for _ in range(8):
                crc = (crc >> 1) ^ (0x82F63B78 if crc & 1 else 0)
        struct.pack_into("<I", data, 1028, crc)
        (output / "blob.bin.zlib").write_bytes(zlib.compress(extra, 9))
        save("external-device", data, options, devices={"blob.bin": {"size": len(extra),
             "file": "blob.bin.zlib"}}, transformation="name device blob.bin and update checksum")
    from generate_xattrs import generate as generate_xattrs
    generate_xattrs(tool, output, manifest)
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    generate(Path(sys.argv[1]).resolve(), Path(__file__).resolve().parent)
