"""Partition-map selection and default mapping over disposable native UDF data."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import binascii
import struct
from tests.formats.udf.support.udf_fixture import volume as base_volume

CASES = ("pm-unknown-type", "pm-unknown-ident", "pm-unknown-before", "pm-short-unknown",
         "pm-duplicate", "pm-many", "pm-zero-length", "pm-padded", "pm-long-span",
         "pm-unvisited", "pm-unknown-data", "pm-duplicate-data", "pm-unknown-fsd",
         "pm-unused-virtual", "pm-unused-metadata", "pm-virtual-revision",
         "pm-metadata-length", "pm-unmatched", "pm-duplicate-virtual",
         "pm-duplicate-metadata", "pm-duplicate-sparable", "pm-many-partitions")
BAD = ("pm-unknown-inode", "pm-duplicate-inode", "pm-too-many", "pm-table-overrun")
MODULE_BAD = ("pm-truncated-type1", "pm-truncated-type2", "pm-unbound-virtual")


def volume(kind):
    source = ("vat15-inline" if kind in ("pm-virtual-revision", "pm-duplicate-virtual")
              else "metadata-bootstrap" if kind in ("pm-metadata-length", "pm-duplicate-metadata")
              else "sp-no-table" if kind == "pm-duplicate-sparable" else "native")
    original, files = base_volume(source)
    image = bytearray(original)
    block = 2048

    def tag(pos, size=None):
        if size is None:
            size = 16 + struct.unpack_from("<H", image, pos + 10)[0]
        struct.pack_into("<HH", image, pos + 8,
                         binascii.crc_hqx(image[pos + 16:pos + size], 0), size - 16)
        image[pos + 4] = 0
        image[pos + 4] = sum(image[pos:pos + 16]) & 255

    physical = bytes(image[35 * block + 440:35 * block + 446])
    unknown = bytearray(64)
    unknown[:2] = b"\x02\x40"
    ident = b"*Unknown partition map!"
    unknown[5:5 + len(ident)] = ident
    struct.pack_into("<HH", unknown, 36, 99, 99)
    first = kind in ("pm-unknown-before", "pm-short-unknown")
    duplicate = kind in ("pm-duplicate", "pm-many", "pm-duplicate-data", "pm-duplicate-inode")
    extra = physical if duplicate else unknown
    if kind == "pm-unmatched":
        extra = struct.pack("<BBHH", 1, 6, 1, 99)
    if kind == "pm-unknown-type":
        extra = bytes((42, 6, 0, 0, 0, 0))
    elif kind == "pm-short-unknown":
        extra = b"\x02\x02"
    elif kind in ("pm-unused-virtual", "pm-unused-metadata", "pm-unbound-virtual"):
        ident = b"*UDF Virtual Partition" if "virtual" in kind else b"*UDF Metadata Partition"
        extra = bytearray(64)
        extra[:2] = b"\x02\x40"
        extra[5:5 + len(ident)] = ident
        struct.pack_into("<H", extra, 28, 0x250)
        struct.pack_into("<HH", extra, 36, 1, 99)

    for sector in (35, 51):
        pos = sector * block
        if kind in ("pm-duplicate-virtual", "pm-duplicate-metadata", "pm-duplicate-sparable"):
            length = struct.unpack_from("<I", image, pos + 264)[0]
            duplicate_map = bytearray(image[pos + 440 + length - 64:pos + 440 + length])
            if kind == "pm-duplicate-virtual":
                struct.pack_into("<H", duplicate_map, 28, 0x0200)
            elif kind == "pm-duplicate-metadata":
                struct.pack_into("<II", duplicate_map, 40, 0xffffffff, 0xffffffff)
            image[pos + 440 + length:pos + 440 + length + 64] = duplicate_map
            struct.pack_into("<II", image, pos + 264, length + 64,
                             struct.unpack_from("<I", image, pos + 268)[0] + 1)
            tag(pos, 440 + length + 64)
            continue
        if kind == "pm-virtual-revision":
            # Linux selects VAT 1.5 for every revision below 0x0200.
            struct.pack_into("<H", image, pos + 446 + 28, 0x0100)
            tag(pos)
            continue
        if kind == "pm-metadata-length":
            image[pos + 446 + 1] = 59
            tag(pos)
            continue
        maps = extra + physical if first else physical + extra
        count = 2
        if kind == "pm-short-unknown":
            maps += bytes(4)
        if kind in ("pm-many", "pm-many-partitions"):
            maps, count = physical * 80, 80
            if kind == "pm-many-partitions":
                maps = b"".join(struct.pack("<BBHH", 1, 6, 1, number) for number in range(count))
        elif kind == "pm-zero-length":
            maps = bytearray(physical * 2)
            maps[1] = 0
        elif kind == "pm-padded":
            maps = bytearray(physical + b"xx" + extra)
            maps[1] = 8
        elif kind in ("pm-long-span", "pm-unvisited"):
            maps = bytearray(physical * (1 if kind == "pm-long-span" else 2))
            maps[1] = 255
            count = len(maps) // 6
        elif kind in ("pm-truncated-type1", "pm-truncated-type2"):
            # Walk past padding to place the final entry one byte from block EOF.
            maps = bytearray(8 * 255)
            maps[:6] = physical
            for offset in range(0, 7 * 255, 255):
                maps[offset:offset + 2] = b"\x2a\xff"
            maps[0] = 1
            # Use a smaller final stride to land at 2047 - 440.
            maps[6 * 255 + 1] = 77
            maps[1607] = 1 if kind.endswith("type1") else 2
            maps = maps[:1608]
            count = 8
        table_len = len(maps)
        image[pos + 440:pos + block] = bytes(block - 440)
        image[pos + 440:pos + 440 + len(maps)] = maps
        struct.pack_into("<II", image, pos + 264,
                         block if kind == "pm-table-overrun" else table_len,
                         table_len // 6 + 1 if kind == "pm-too-many" else count)
        if first:
            struct.pack_into("<H", image, pos + 256, 1)
        tag(pos, 440 + len(maps))

    if kind == "pm-many-partitions":
        descriptor = bytes(image[34 * block:35 * block])
        terminator = bytes(image[37 * block:38 * block])
        for number in range(1, 80):
            pos = (36 + number) * block
            image[pos:pos + block] = descriptor
            struct.pack_into("<I", image, pos + 12, 36 + number)
            struct.pack_into("<H", image, pos + 22, number)
            tag(pos)
        image[116 * block:117 * block] = terminator
        struct.pack_into("<I", image, 116 * block + 12, 116)
        tag(116 * block)
        for sector in range(len(image) // block):
            pos = sector * block
            if (struct.unpack_from("<H", image, pos)[0] == 2 and
                    struct.unpack_from("<I", image, pos + 12)[0] == sector):
                struct.pack_into("<I", image, pos + 16, 128 * block)
                tag(pos)

    if first:
        struct.pack_into("<H", image, 257 * block + 408, 1)
        tag(257 * block)
        for sector, size in ((260, 180), (262, 88)):
            offset = sector * block
            while offset < sector * block + size:
                struct.pack_into("<H", image, offset + 28, 1)
                tag(offset)
                offset += (38 + image[offset + 19] + struct.unpack_from("<H", image, offset + 36)[0] + 3) & ~3
    if kind in ("pm-unknown-data", "pm-duplicate-data", "pm-duplicate-sparable",
                "pm-unbound-virtual"):
        image[100 * block:104 * block] = files["alpha.bin"]
        pos = 263 * block
        struct.pack_into("<H", image, pos + 34, 1)
        struct.pack_into("<I", image, pos + 172, 16)
        image[pos + 176:pos + block] = struct.pack("<IIH6x", 8192, 100, 1) + bytes(block - 192)
        tag(pos, 192)
    elif kind == "pm-unknown-fsd":
        image[240 * block:241 * block] = image[257 * block:258 * block]
        struct.pack_into("<I", image, 240 * block + 12, 240)
        tag(240 * block)
        for sector in (35, 51):
            struct.pack_into("<IH", image, sector * block + 252, 240, 1)
            tag(sector * block)
    elif kind in ("pm-unknown-inode", "pm-duplicate-inode"):
        offset = 260 * block
        while offset < 260 * block + 180:
            if image[offset + 39:offset + 48] == b"alpha.bin":
                struct.pack_into("<H", image, offset + 28, 1)
                tag(offset)
                break
            offset += (38 + image[offset + 19] + struct.unpack_from("<H", image, offset + 36)[0] + 3) & ~3
    return bytes(image), files
