"""XZ streams, block/index boundaries and independent encoder fixtures."""
from pathlib import Path
import json
import lzma
import struct
import subprocess
import sys
import tempfile
import zlib

CASES = ("crc32", "none", "crc64", "sha256", "empty", "multiple-blocks", "zero-blocks",
         "padding", "long-padding", "concatenated", "empty-first", "uncompressed",
         "x86", "powerpc", "arm", "thumb", "sparc", "arm64", "riscv", "delta")
BAD = ("magic", "header-crc", "block-crc", "index-crc", "footer-crc", "footer-magic",
       "block-padding", "padding-short", "tail", "body-short", "index-size",
       "index-unpack", "filter", "dictionary", "header-padding", "flags")


def crc(data):
    return struct.pack("<I", zlib.crc32(data))


def vli(value):
    result = bytearray()
    while value >= 128:
        result.append((value & 127) | 128)
        value >>= 7
    return bytes(result) + bytes((value,))


def read_vli(data, position):
    value, shift = 0, 0
    while True:
        byte = data[position]
        position += 1
        value |= (byte & 127) << shift
        if byte < 128:
            return value, position
        shift += 7


def index_position(image):
    return len(image) - 12 - (struct.unpack_from("<I", image, len(image) - 8)[0] + 1) * 4


def index_records(image):
    position = index_position(image) + 1
    count, position = read_vli(image, position)
    records = []
    for _ in range(count):
        packed, position = read_vli(image, position)
        unpacked, position = read_vli(image, position)
        records.append((packed, unpacked))
    return records


def finish(header, blocks, records):
    index = b"\0" + vli(len(records))
    index += b"".join(vli(packed) + vli(unpacked) for packed, unpacked in records)
    index += bytes(-len(index) % 4)
    index += crc(index)
    footer = struct.pack("<I", len(index) // 4 - 1) + header[6:8]
    return header + blocks + index + crc(footer) + footer + b"YZ"


def encode(data, *, check=lzma.CHECK_CRC32, filter_id=None):
    filters = [] if filter_id is None else [{"id": filter_id}]
    if filter_id == lzma.FILTER_DELTA:
        filters[0]["dist"] = 3
    filters.append({"id": lzma.FILTER_LZMA2, "dict_size": 4096})
    return lzma.compress(data, format=lzma.FORMAT_XZ, check=check, filters=filters)


def multiple(data, zero_blocks=False):
    stream_header = encode(b"")[:12]
    records, blocks = [], []
    # Empty LZMA2 block: 12-byte header, end marker, zero padding and CRC32.
    raw_header = b"\x02\x00\x21\x01\x00\x00\x00\x00"
    empty = raw_header + crc(raw_header) + bytes(8)
    for position in range(0, len(data), 8192):
        if zero_blocks:
            blocks.append(empty)
            records.append((17, 0))
        image = encode(data[position:position + 8192])
        block = image[12:index_position(image)]
        packed, unpacked = index_records(image)[0]
        old_header_size = (block[0] + 1) * 4
        block_header = bytearray(b"\0\xc0" + vli(packed - old_header_size - 4) + vli(unpacked)
                                 + b"\x21\x01\x00")
        block_header += bytes(-len(block_header) % 4)
        block_header[0] = len(block_header) // 4
        block_header += crc(block_header)
        blocks.append(block_header + block[old_header_size:])
        records.append((packed + len(block_header) - old_header_size, unpacked))
    if zero_blocks:
        blocks.append(empty)
        records.append((17, 0))
    return finish(stream_header, b"".join(blocks), records)


def volume(kind, producer=None):
    payload = (bytes(range(256)) + b"XZ payload\0\xff") * 110
    image = encode(payload)
    if kind == "zip-xz":
        name = "данные 空.txt".encode("utf-8")
        checksum = zlib.crc32(payload)
        local = struct.pack("<IHHHHHIIIHH", 0x04034b50, 63, 0x800, 95, 0, 0,
                            checksum, len(image), len(payload), len(name), 0) + name + image
        central = struct.pack("<IHHHHHHIIIHHHHHII", 0x02014b50, 63, 63, 0x800, 95, 0, 0,
                              checksum, len(image), len(payload), len(name), 0, 0, 0, 0, 0, 0) + name
        return local + central + struct.pack("<IHHHHIIH", 0x06054b50, 0, 0, 1, 1,
                                             len(central), len(local), 0), payload
    if kind in ("none", "crc64", "sha256"):
        image = encode(payload, check={"none": lzma.CHECK_NONE, "crc64": lzma.CHECK_CRC64,
                                       "sha256": lzma.CHECK_SHA256}[kind])
    elif kind == "empty":
        payload = b""
        image = encode(payload)
    elif kind in ("multiple-blocks", "zero-blocks"):
        image = multiple(payload, kind == "zero-blocks")
    elif kind in ("padding", "long-padding"):
        image += bytes(131072 if kind == "long-padding" else 8)
    elif kind in ("concatenated", "empty-first"):
        if kind == "empty-first":
            image = encode(b"") + image
        else:
            image += bytes(12) + encode(b"second member")
            payload += b"second member"
    elif kind == "uncompressed":
        state = 19
        data = bytearray()
        for _ in range(40000):
            state = (state * 1664525 + 1013904223) & 0xffffffff
            data.append(state >> 24)
        payload = bytes(data)
        image = encode(payload)
    elif kind in ("x86", "powerpc", "arm", "thumb", "sparc", "delta"):
        payload = bytes.fromhex("e80100000048000001eb00000000f000f840000001") * 800
        filters = {"x86": lzma.FILTER_X86, "powerpc": lzma.FILTER_POWERPC,
                   "arm": lzma.FILTER_ARM, "thumb": lzma.FILTER_ARMTHUMB,
                   "sparc": lzma.FILTER_SPARC, "delta": lzma.FILTER_DELTA}
        image = encode(payload, filter_id=filters[kind])
    elif kind in ("arm64", "riscv"):
        if not producer:
            raise ValueError("The pinned 7z producer is required for newer BCJ filters")
        payload = bytes.fromhex("000000940000009400000090ef0000006f000000") * 800
        with tempfile.TemporaryDirectory(prefix="sevenzip-xz-producer-") as temporary:
            temporary = Path(temporary)
            source, output = temporary / "data", temporary / "filter.xz"
            source.write_bytes(payload)
            subprocess.run([str(producer), "a", "-txz", "-mf=" + kind.upper(), "-mcrc=4",
                            "-md=4k", "-mmt=1", "--", str(output), str(source)],
                           check=True, capture_output=True, timeout=10)
            image = output.read_bytes()
    elif kind in ("magic", "header-crc", "footer-crc", "footer-magic", "index-crc"):
        offset = {"magic": 0, "header-crc": 8, "footer-crc": len(image) - 12,
                  "footer-magic": len(image) - 1, "index-crc": len(image) - 16}[kind]
        image = image[:offset] + bytes((image[offset] ^ 1,)) + image[offset + 1:]
    elif kind == "block-crc":
        offset = index_position(image) - 4
        image = image[:offset] + bytes((image[offset] ^ 1,)) + image[offset + 1:]
    elif kind == "block-padding":
        payload = b"abc"
        image = encode(payload)
        packed = index_records(image)[0][0]
        padding = -packed % 4
        assert padding
        offset = 12 + packed - 4
        image = image[:offset] + b"\x01" + image[offset + 1:]
    elif kind in ("index-size", "index-unpack"):
        records = index_records(image)
        packed, unpacked = records[0]
        records[0] = (packed + (kind == "index-size"), unpacked + (kind == "index-unpack"))
        image = finish(image[:12], image[12:index_position(image)], records)
    elif kind in ("filter", "dictionary", "header-padding"):
        header_size = (image[12] + 1) * 4
        header = bytearray(image[12:12 + header_size - 4])
        header[{"filter": 2, "dictionary": 4, "header-padding": 7}[kind]] = (
            0x22 if kind == "filter" else 41 if kind == "dictionary" else 1)
        image = image[:12] + header + crc(header) + image[12 + header_size:]
    elif kind == "flags":
        header = image[:6] + b"\x01\x01"
        image = header + crc(header[6:]) + image[12:]
    elif kind == "body-short":
        image = image[:30]
    elif kind == "padding-short":
        image += bytes(1)
    elif kind == "tail":
        image += b"unexpected tail"
    return image, payload


if __name__ == "__main__":
    image, payload = volume(sys.argv[1], Path(sys.argv[3]) if len(sys.argv) > 3 else None)
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps({"payload": payload.hex()}))
