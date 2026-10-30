"""Small legacy LZ4/lzop streams; no system compressor or user files."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import json
from pathlib import Path
import struct
import sys
import zlib

LZ4_MAGIC = bytes.fromhex("02214c18")
LZO_MAGIC = bytes.fromhex("894c5a4f000d0a1a0a")
PAYLOAD = bytes(range(256)) * 3 + b"frame payload\x00\xff"


def lz4_literal(data):
    if len(data) < 15:
        return bytes([len(data) << 4]) + data
    extra = len(data) - 15
    return b"\xf0" + bytes([255]) * (extra // 255) + bytes([extra % 255]) + data


def lz4_match(size=8192):
    extra = size - 6 - 19
    return b"\x1fA\x01\x00" + bytes([255]) * (extra // 255) + bytes([extra % 255, 0x50]) + b"A" * 5


def lz4_frame(blocks):
    return LZ4_MAGIC + b"".join(struct.pack("<I", len(b)) + b for b in blocks)


def lzo_header(flags=1, old=False, name=b"", method=1, filter_value=0):
    data = struct.pack(">HH", 0x0930 if old else 0x1040, 0x20A0)
    if not old:
        data += struct.pack(">H", 0x0900)
    data += bytes([method])
    if not old:
        data += b"\x05"
    data += struct.pack(">I", flags)
    if flags & 0x800:
        data += struct.pack(">I", filter_value)
    data += struct.pack(">II", 0o100644, 1700000000)
    if not old:
        data += bytes(4)
    data += bytes([len(name)]) + name
    checksum = zlib.crc32(data) if flags & 0x1000 else zlib.adler32(data)
    return LZO_MAGIC + data + struct.pack(">I", checksum)


def lzo_block(data, packed=None, flags=1):
    packed = data if packed is None else packed
    result = struct.pack(">II", len(data), len(packed))
    for mask, value, crc in ((1, data, False), (0x100, data, True),
                             (2, packed, False), (0x200, packed, True)):
        if flags & mask and (mask in (1, 0x100) or len(packed) < len(data)):
            result += struct.pack(">I", zlib.crc32(value) if crc else zlib.adler32(value))
    return result + packed


def lzo_rle():
    packed = bytes([17, 1, 18, ord("X")])
    for count in (2048, 2048, 2048, 2047):
        value = count - 4
        packed += bytes([0x18 | (value & 7), 0xfc, 0xff, value >> 3])
    return b"X" + bytes(8191), packed + b"\x11\x00\x00"


CASES = tuple("lz4-" + s for s in ("literal", "match", "multi", "magic-between", "zero-end",
                                     "empty", "zero-tail", "concat", "max-block")) + tuple(
    "lzo-" + s for s in ("stored", "rle", "multi", "empty", "old", "max-name", "filter-identity",
                         "crc", "both", "no-checksum", "compressed-checksums", "concat", "zero-tail",
                         "metadata-flags"))
BAD = tuple("lz4-" + s for s in ("short-magic", "short-length", "oversize", "short-data",
                                  "bad-match", "overflow-output", "junk")) + tuple(
    "lzo-" + s for s in ("short-header", "header-checksum", "data-checksum", "compressed-checksum",
                         "dst-oversize", "src-oversize", "src-zero", "short-data", "no-end",
                         "filter", "method", "late-checksum", "junk",
                         "crc-header", "crc-data", "crc-compressed"))


def volume(kind):
    payload = PAYLOAD
    if kind.endswith("-cpio"):
        from tests.formats.cpio.support.cpio_fixture import record, trailer
        payload = record("данные 空.txt", b"nested cpio payload") + trailer()
    if kind.startswith("lz4-"):
        image = lz4_frame([lz4_literal(payload)])
        fmt = "Lz4Legacy"
        if kind in ("lz4-match", "lz4-multi", "lz4-magic-between"):
            payload = b"A" * 8192
            image = lz4_frame([lz4_match()])
            if kind != "lz4-match":
                image += (LZ4_MAGIC if kind == "lz4-magic-between" else b"")
                image += struct.pack("<I", len(lz4_literal(PAYLOAD))) + lz4_literal(PAYLOAD)
                payload += PAYLOAD
        elif kind == "lz4-max-block":
            payload = b"A" * (8 << 20)
            image = lz4_frame([lz4_match(len(payload))])
        elif kind == "lz4-empty":
            image, payload = LZ4_MAGIC, b""
        elif kind in ("lz4-zero-end", "lz4-zero-tail"):
            image += bytes(4 if kind == "lz4-zero-end" else 192 << 10)
        elif kind == "lz4-concat":
            image += bytes(7) + lz4_frame([lz4_literal(b"second")])
            payload += b"second"
        elif kind == "lz4-short-magic":
            image = LZ4_MAGIC[:3]
        elif kind == "lz4-short-length":
            image = LZ4_MAGIC + b"\x11\x00"
        elif kind == "lz4-oversize":
            image = LZ4_MAGIC + struct.pack("<I", 0xffffffff)
        elif kind == "lz4-short-data":
            image = image[:-7]
        elif kind == "lz4-bad-match":
            image = lz4_frame([b"\x10A\x02\x00\x50abcde"])
        elif kind == "lz4-overflow-output":
            image = lz4_frame([lz4_match((8 << 20) + 1)])
        elif kind == "lz4-junk":
            image += b"garbage"
    else:
        fmt = "Lzop"
        flags = {"lzo-crc": 0x1100, "lzo-both": 0x1101, "lzo-no-checksum": 0,
                 "lzo-crc-header": 0x1100, "lzo-crc-data": 0x1100,
                 "lzo-crc-compressed": 0x1300,
                 "lzo-compressed-checksums": 0x1303,
                 "lzo-compressed-checksum": 3, "lzo-filter-identity": 0x801,
                 "lzo-metadata-flags": 0x0300249d}.get(kind, 1)
        data, packed = lzo_rle()
        compressed = kind in ("lzo-rle", "lzo-compressed-checksums", "lzo-compressed-checksum",
                              "lzo-crc-compressed")
        if compressed:
            payload = data
        header = lzo_header(flags, old=kind == "lzo-old",
                            name=b"n" * 255 if kind == "lzo-max-name" else b"",
                            method=0 if kind == "lzo-method" else 1,
                            filter_value=1 if kind == "lzo-filter" else 0)
        if kind == "lzo-filter":
            header = lzo_header(0x801, filter_value=1)
        image = header + lzo_block(payload, packed if compressed else None, flags) + bytes(4)
        if kind == "lzo-multi":
            image = header + lzo_block(payload, flags=flags) + lzo_block(data, packed, flags) + bytes(4)
            payload += data
        elif kind == "lzo-empty":
            image, payload = header + bytes(4), b""
        elif kind == "lzo-zero-tail":
            image += bytes(192 << 10)
        elif kind == "lzo-concat":
            image += bytes(7) + header + lzo_block(b"second", flags=flags) + bytes(4)
            payload += b"second"
        elif kind == "lzo-short-header":
            image = header[:25]
        elif kind in ("lzo-header-checksum", "lzo-crc-header"):
            image = image[:len(header) - 1] + bytes([image[len(header) - 1] ^ 1]) + image[len(header):]
        elif kind in ("lzo-data-checksum", "lzo-crc-data"):
            image = image[:len(header) + 8] + bytes([image[len(header) + 8] ^ 1]) + image[len(header) + 9:]
        elif kind in ("lzo-compressed-checksum", "lzo-crc-compressed"):
            image = image[:len(header) + 12] + bytes([image[len(header) + 12] ^ 1]) + image[len(header) + 13:]
        elif kind in ("lzo-dst-oversize", "lzo-src-oversize", "lzo-src-zero"):
            sizes = {"lzo-dst-oversize": ((256 << 10) + 1, 1),
                     "lzo-src-oversize": (10, 11), "lzo-src-zero": (10, 0)}[kind]
            image = header + struct.pack(">II", *sizes) + bytes(8)
        elif kind == "lzo-short-data":
            image = image[:-12]
        elif kind == "lzo-no-end":
            image = image[:-4]
        elif kind == "lzo-late-checksum":
            second = header + lzo_block(b"second", flags=flags) + bytes(4)
            image += second[:len(header) + 8] + b"\0" * 4 + second[len(header) + 12:]
        elif kind == "lzo-junk":
            image += b"garbage"
    return image, dict(format=fmt, payload=payload.hex())


if __name__ == "__main__":
    image, meta = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps(meta))
