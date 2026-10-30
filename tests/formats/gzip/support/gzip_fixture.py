"""Gzip framing fixtures with independent zlib payload generation."""
from pathlib import Path
import json
import struct
import sys
import zlib

CASES = ("normal", "xfl-1", "xfl-255", "stored-padding", "name-4095", "name-4096",
         "name-4097", "name-long", "comment-long", "optional-fields", "text",
         "fixed", "dynamic", "empty", "concatenated")
BAD = ("crc", "size", "footer-short", "header-short", "name-unterminated",
       "stored-length", "deflate-type", "old-magic", "method", "reserved", "tail",
       "second-crc")


def member(data, *, name=b"payload.bin", comment=None, extra=None, header_crc=False,
           xfl=0, flags=0, stored=False, fixed=False):
    flags |= (8 if name is not None else 0) | (16 if comment is not None else 0)
    flags |= (4 if extra is not None else 0) | (2 if header_crc else 0)
    header = b"\x1f\x8b\x08" + bytes((flags,)) + struct.pack("<IBB", 123, xfl, 3)
    if extra is not None:
        header += struct.pack("<H", len(extra)) + extra
    if name is not None:
        header += name + b"\0"
    if comment is not None:
        header += comment + b"\0"
    if header_crc:
        header += struct.pack("<H", zlib.crc32(header) & 65535)
    if stored:
        assert len(data) < 65536
        body = b"\xf9" + struct.pack("<HH", len(data), len(data) ^ 65535) + data
    else:
        coder = zlib.compressobj(6, zlib.DEFLATED, -15,
                                strategy=zlib.Z_FIXED if fixed else zlib.Z_DEFAULT_STRATEGY)
        body = coder.compress(data) + coder.flush()
    return header + body + struct.pack("<II", zlib.crc32(data), len(data) & 0xffffffff)


def volume(kind):
    data = b"gzip payload\x00\xff\n"
    name = b"payload.bin"
    options = {}
    if kind.startswith("xfl-"):
        options["xfl"] = int(kind.split("-")[1])
    elif kind.startswith("name-") and kind != "name-unterminated":
        count = 131073 if kind == "name-long" else int(kind.split("-")[1])
        name = b"n" * count
    elif kind == "comment-long":
        options["comment"] = b"c" * 131073
    elif kind == "optional-fields":
        options.update(comment=b"archive comment", extra=b"SZ\x03\x00abc", header_crc=True)
    elif kind == "text":
        options["flags"] = 1
    elif kind == "fixed":
        options["fixed"] = True
    elif kind == "dynamic":
        data = bytes(range(256)) * 1024 + b"other data" * 1000
    elif kind == "empty":
        data = b""
    image = member(data, name=name, stored=kind in ("stored-padding", "stored-length"), **options)
    if kind in ("concatenated", "second-crc"):
        other = member(b"second member", name=None)
        if kind == "second-crc":
            other = other[:-8] + bytes((other[-8] ^ 1,)) + other[-7:]
        image += other
        data += b"second member"
    elif kind == "crc":
        image = image[:-8] + bytes((image[-8] ^ 1,)) + image[-7:]
    elif kind == "size":
        image = image[:-4] + bytes((image[-4] ^ 1,)) + image[-3:]
    elif kind == "footer-short":
        image = image[:-1]
    elif kind == "header-short":
        image = image[:7]
    elif kind == "name-unterminated":
        image = image[:10] + b"n" * 131073
    elif kind == "stored-length":
        offset = 10 + len(name) + 1 + 3
        image = image[:offset] + bytes((image[offset] ^ 1,)) + image[offset + 1:]
    elif kind == "deflate-type":
        offset = 10 + len(name) + 1
        image = image[:offset] + b"\x07" + image[offset + 1:]
    elif kind == "old-magic":
        image = b"\x1f\x9e" + image[2:]
    elif kind == "method":
        image = image[:2] + b"\x09" + image[3:]
    elif kind == "reserved":
        image = image[:3] + bytes((image[3] | 0x20,)) + image[4:]
    elif kind == "tail":
        image += b"unexpected tail"
    return image, data, name


if __name__ == "__main__":
    image, payload, name = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps({"payload": payload.hex(), "name": name.decode()}))
