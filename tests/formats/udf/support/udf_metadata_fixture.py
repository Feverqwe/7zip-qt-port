"""Fragmented metadata files and mirrors over a disposable native UDF image."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import binascii
import struct
from tests.formats.udf.support.udf_fixture import volume as base_volume

CASES = ("md-fragmented", "md-mirror-bootstrap", "md-mirror-lazy", "md-primary-only",
         "md-bitmap-missing", "md-bitmap-present", "md-short-size", "md-ad-chain",
         "md-offset", "md-sparable", "md-long-primary", "md-efe", "md-attributes",
         "metadata-hole", "metadata-outside")
BAD = ("md-both-invalid", "md-both-long", "md-inline-primary", "md-unmapped",
       "md-no-allocation", "md-primary-corrupt", "md-mirror-corrupt", "md-ad-cycle",
       "md-outside-target", "md-wrapped-target", "md-ad-zero-cycle")


def volume(kind):
    if kind.startswith("metadata-"):
        return base_volume(kind)
    block = 2048
    raw, files = base_volume("native")
    image = bytearray(raw)
    image.extend(bytes(800 * block - len(image)))
    template = bytes(image[263 * block:264 * block])

    def tag(pos, size=None):
        if size is None:
            size = 16 + struct.unpack_from("<H", image, pos + 10)[0]
        struct.pack_into("<HH", image, pos + 8,
                         binascii.crc_hqx(image[pos + 16:pos + size], 0), size - 16)
        image[pos + 4] = 0
        image[pos + 4] = sum(image[pos:pos + 16]) & 255

    struct.pack_into("<H", image, 257 * block + 408, 1)
    tag(257 * block)
    for sector, size in ((260, 180), (262, 88)):
        pos = sector * block
        while pos < sector * block + size:
            struct.pack_into("<H", image, pos + 28, 1)
            tag(pos)
            pos += (38 + image[pos + 19] + struct.unpack_from("<H", image, pos + 36)[0] + 3) & ~3
    original = bytes(image[257 * block:278 * block])
    split = 16 if kind == "md-offset" else 8
    image[300 * block:(300 + split) * block] = original[:split * block]
    image[340 * block:(361 - split) * block] = original[split * block:]
    image[500 * block:521 * block] = original
    image[257 * block:278 * block] = b"!" * (21 * block)
    if kind == "md-offset":
        image[316 * block:319 * block] = original[16 * block:19 * block]
        image[340 * block:343 * block] = b"X" * (3 * block)

    def inode(logical, extents, descriptor=0, file_type=250, size=21 * block):
        pos = (257 + logical) * block
        image[pos:pos + block] = template
        image[pos + 27] = file_type
        struct.pack_into("<I", image, pos + 12, logical)
        struct.pack_into("<H", image, pos + 34, descriptor)
        struct.pack_into("<Q", image, pos + 56, size)
        struct.pack_into("<Q", image, pos + 64, (size + block - 1) // block)
        data = b"".join(struct.pack("<IIH6x", length, location, 0) if descriptor == 1
                         else struct.pack("<II", length, location) for length, location in extents)
        struct.pack_into("<II", image, pos + 168, 0, len(data))
        image[pos + 176:pos + block] = data + bytes(block - 176 - len(data))
        tag(pos, 176 + len(data))
        return pos

    first = [(split * block, 43), ((21 - split) * block, 83)]
    if kind in ("md-mirror-lazy", "md-unmapped"):
        first[1] = ((1 << 30) | (21 - split) * block, 0xffffffff)
    elif kind == "md-mirror-corrupt":
        first = [((1 << 30) | 21 * block, 0xffffffff)]
    elif kind == "md-no-allocation":
        first = []
    elif kind in ("md-outside-target", "md-wrapped-target"):
        first = [(21 * block, 600 if kind == "md-outside-target" else 0xffffffff)]
    elif kind in ("md-ad-chain", "md-ad-cycle", "md-ad-zero-cycle"):
        first = [(8 * block, 43), ((3 << 30) | block, 133)] if kind == "md-ad-chain" else [((3 << 30) | block, 133)]
        pos = 390 * block
        image[pos:pos + block] = bytes(block)
        struct.pack_into("<HH", image, pos, 258, 2)
        struct.pack_into("<I", image, pos + 12, 133)
        struct.pack_into("<I", image, pos + 20, 16 if kind == "md-ad-zero-cycle" else 8)
        struct.pack_into("<II", image, pos + 24, 13 * block if kind == "md-ad-chain" else (3 << 30) | block,
                         83 if kind == "md-ad-chain" else 133)
        tag(pos, 32)
        if kind == "md-ad-zero-cycle":
            struct.pack_into("<IIII", image, pos + 24, 1 << 30, 0, (3 << 30) | block, 133)
            tag(pos, 40)
    primary = inode(130, first, descriptor=1 if kind in ("md-long-primary", "md-both-long") else 3 if kind == "md-inline-primary" else 0,
                    size=0 if kind in ("md-short-size", "md-inline-primary") else 21 * block)
    mirror = inode(131, [(21 * block, 0xffffffff if kind == "md-wrapped-target" else 243)],
                   descriptor=1 if kind == "md-both-long" else 0, file_type=251)
    if kind == "md-efe":
        old = bytes(image[primary:primary + block])
        image[primary:primary + block] = bytes(block)
        for dst, src, size in ((0, 0, 64), (64, 56, 8), (72, 64, 8), (80, 72, 24),
                               (104, 84, 12), (116, 96, 12), (128, 108, 4),
                               (136, 112, 16), (168, 128, 48)):
            image[primary + dst:primary + dst + size] = old[src:src + size]
        image[primary + 216:primary + block] = old[176:block - 40]
        struct.pack_into("<H", image, primary, 266)
        tag(primary, 216 + struct.unpack_from("<I", image, primary + 212)[0])
    elif kind == "md-attributes":
        length = struct.unpack_from("<I", image, primary + 172)[0]
        allocation = bytes(image[primary + 176:primary + 176 + length])
        image[primary + 176:primary + 200] = bytes(24)
        image[primary + 200:primary + 200 + length] = allocation
        struct.pack_into("<I", image, primary + 168, 24)
        struct.pack_into("<HH", image, primary + 176, 262, 2)
        struct.pack_into("<I", image, primary + 188, 130)
        struct.pack_into("<II", image, primary + 192, 0xffffffff, 0xffffffff)
        tag(primary + 176, 24)
        tag(primary, 200 + length)
    if kind in ("md-mirror-bootstrap", "md-both-invalid"):
        image[primary + 4] ^= 1
    if kind in ("md-primary-only", "md-both-invalid", "md-inline-primary", "md-unmapped",
                "md-no-allocation", "md-ad-cycle", "md-ad-zero-cycle"):
        image[mirror + 4] ^= 1
    if kind == "md-primary-corrupt":
        image[300 * block + 16] ^= 1
    if kind == "md-mirror-corrupt":
        image[500 * block + 16] ^= 1
    if kind == "md-bitmap-present":
        inode(132, [], descriptor=3, file_type=252, size=0)
    for sector in (34, 50):
        struct.pack_into("<I", image, sector * block + 192, 543)
        tag(sector * block)
    pm = bytearray(64)
    pm[:2] = b"\x02\x40"
    pm[5:28] = b"*UDF Metadata Partition"
    struct.pack_into("<H", pm, 28, 0x250)
    struct.pack_into("<HH", pm, 36, 1, 0)
    struct.pack_into("<III", pm, 40, 130, 131, 132 if kind.startswith("md-bitmap-") else 0xffffffff)
    struct.pack_into("<IH", pm, 52, 1, 1)
    pm[58] = 1
    for sector in (35, 51):
        pos = sector * block
        image[pos + 446:pos + 510] = pm
        struct.pack_into("<H", image, pos + 256, 1)
        struct.pack_into("<II", image, pos + 264, 70, 2)
        struct.pack_into("<H", image, pos + 240, 0x250)
        tag(pos, 510)
    if kind == "md-sparable":
        image[700 * block:702 * block] = image[347 * block:349 * block]
        image[704 * block:706 * block] = image[349 * block:351 * block]
        image[347 * block:351 * block] = b"!" * (4 * block)
        pos = 64 * block
        image[pos:pos + block] = bytes(block)
        struct.pack_into("<HH", image, pos, 0, 2)
        struct.pack_into("<I", image, pos + 12, 64)
        image[pos + 17:pos + 35] = b"*UDF Sparing Table"
        struct.pack_into("<H", image, pos + 48, 2)
        struct.pack_into("<I", image, pos + 52, 1)
        struct.pack_into("<IIII", image, pos + 56, 90, 700, 92, 704)
        tag(pos, 72)
        sparing = bytearray(64)
        sparing[:2] = b"\x02\x40"
        sparing[5:28] = b"*UDF Sparable Partition"
        struct.pack_into("<H", sparing, 28, 0x150)
        struct.pack_into("<HHH", sparing, 36, 1, 0, 2)
        sparing[42] = 1
        struct.pack_into("<II", sparing, 44, 72, 64)
        for sector in (35, 51):
            pos = sector * block
            image[pos + 440:pos + 568] = sparing + pm
            struct.pack_into("<II", image, pos + 264, 128, 2)
            tag(pos, 568)
    return bytes(image), files
