"""VAT revisions over a disposable native UDF volume; original data is poisoned."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import binascii
import struct
from tests.formats.udf.support.udf_fixture import volume as base_volume

CASES = ("vat15-inline", "vat20-inline", "vat15-external", "vat20-external",
         "vat20-extended-header", "vat20-previous", "vat15-offset",
         "vat20-sparse", "vat20-unknown-newest", "vat20-cross-partition", "vat20-sparable")
BAD = ("vat-missing", "vat15-short", "vat20-short", "vat20-header-outside",
       "vat20-header-unaligned", "vat20-entry-outside", "vat20-entry-sentinel",
       "vat20-truncated-entries", "vat20-newest-damaged", "vat20-data-hole")


def volume(kind):
    block = 2048
    raw, files = base_volume("sparse-long" if kind == "vat20-sparse" else "native")
    image = bytearray(raw)
    image.extend(bytes(1400 * block - len(image)))
    last = len(image) // block - 1
    version15 = kind.startswith("vat15")
    external = kind in ("vat15-external", "vat20-external", "vat20-extended-header",
                        "vat20-data-hole")
    entries = 1024 if external else 64
    header = 0 if version15 else 4096 if kind == "vat20-extended-header" else 152

    def tag(pos, size=None):
        if size is None:
            size = 16 + struct.unpack_from("<H", image, pos + 10)[0]
        struct.pack_into("<HH", image, pos + 8,
                         binascii.crc_hqx(image[pos + 16:pos + size], 0), size - 16)
        image[pos + 4] = 0
        image[pos + 4] = sum(image[pos:pos + 16]) & 255

    # All directory identifiers and the fileset now refer to the virtual map.
    struct.pack_into("<H", image, 257 * block + 408, 1)
    tag(257 * block)
    for sector, size in ((260, 180), (262, 88)):
        pos = sector * block
        while pos < sector * block + size:
            struct.pack_into("<H", image, pos + 28, 1)
            tag(pos)
            pos += (38 + image[pos + 19] + struct.unpack_from("<H", image, pos + 36)[0] + 3) & ~3
    if kind == "vat20-sparse":
        pos = 263 * block
        for i in range(4):
            struct.pack_into("<H", image, pos + 176 + i * 16 + 8, 1)
        tag(pos)
    if external and kind != "vat20-data-hole":
        # Exercise lookup in a later VAT block while preserving extent offset semantics.
        struct.pack_into("<I", image, 263 * block + 180, 600)
        tag(263 * block)
    image[300 * block:321 * block] = image[257 * block:278 * block]
    image[257 * block:278 * block] = b"!" * (21 * block)

    cells = [(20 if kind == "vat20-cross-partition" else 43) + i for i in range(entries)]
    if external:
        cells[600] = 58
    if kind in ("vat15-offset", "vat20-sparable"):
        cells[16:19] = [0xffffffff] * 3
    if kind == "vat20-entry-outside":
        cells[15] = 2000
    if kind == "vat20-entry-sentinel":
        cells[15] = 0xffffffff
    if kind == "vat20-truncated-entries":
        cells = cells[:15]
    data = bytearray(header) + bytearray(struct.pack("<" + "I" * len(cells), *cells))
    if version15:
        data += b"\0*UDF Virtual Alloc Tbl".ljust(32, b"\0") + struct.pack("<I", 0xffffffff)
    else:
        struct.pack_into("<HH", data, 0, header, max(0, header - 152))
    if kind in ("vat15-short", "vat20-short"):
        data = data[:35 if version15 else 1]
    if kind == "vat20-header-outside":
        struct.pack_into("<H", data, 0, len(data) + 4)
    if kind == "vat20-header-unaligned":
        struct.pack_into("<H", data, 0, header + 1)

    def inode(sector, payload):
        pos = sector * block
        image[pos:pos + block] = image[306 * block:307 * block]
        image[pos + 27] = 0 if version15 else 248
        struct.pack_into("<I", image, pos + 12, sector - 257)
        struct.pack_into("<H", image, pos + 34, 0 if external else 3)
        struct.pack_into("<Q", image, pos + 56, len(payload))
        struct.pack_into("<Q", image, pos + 64, (len(payload) + block - 1) // block if external else 0)
        descriptor = struct.pack("<II", len(payload) | (1 << 30 if kind == "vat20-data-hole" else 0), 133) if external else payload
        struct.pack_into("<I", image, pos + 168, 0)
        struct.pack_into("<I", image, pos + 172, len(descriptor))
        image[pos + 176:pos + block] = descriptor + bytes(block - 176 - len(descriptor))
        tag(pos, 176 + len(descriptor))
        if external:
            image[390 * block:390 * block + len(payload)] = payload

    if kind != "vat-missing":
        inode(last - 3 if kind == "vat20-previous" else last, data)
    if kind == "vat20-newest-damaged":
        inode(last - 1, data)
        inode(last, b"\xff\xff")
    if kind == "vat20-unknown-newest":
        inode(last - 1, data)
        damaged = bytearray(data)
        struct.pack_into("<I", damaged, header + 15 * 4, 0xffffffff)
        inode(last, damaged)
        image[last * block + 27] = 255
        tag(last * block)
    for sector in (34, 50):
        struct.pack_into("<I", image, sector * block + 192, 1400 - 257)
        tag(sector * block)
    if kind == "vat20-cross-partition":
        # The newest usable VAT follows a strategy-4096 IE into a later map.
        inode(503, data)
        struct.pack_into("<I", image, 503 * block + 12, 223)
        tag(503 * block)
        inode(last - 1, data)
        struct.pack_into("<H", image, (last - 1) * block + 20, 4096)
        tag((last - 1) * block)
        pos = last * block
        image[pos:pos + block] = bytes(block)
        struct.pack_into("<HH", image, pos, 259, 2)
        struct.pack_into("<I", image, pos + 12, last - 257)
        struct.pack_into("<IIH", image, pos + 36, block, 223, 2)
        tag(pos, 52)
        for first, second in ((34, 36), (50, 52)):
            a, b = first * block, second * block
            original = bytes(image[a:a + block])
            image[b:b + block] = original
            struct.pack_into("<H", image, a + 22, 1)
            struct.pack_into("<II", image, a + 188, 280, 1120)
            struct.pack_into("<I", image, b + 12, second)
            tag(a)
            tag(b)
    pm = bytearray(64)
    pm[:2] = b"\x02\x40"
    pm[5:27] = b"*UDF Virtual Partition"
    struct.pack_into("<H", pm, 28, 0x150 if version15 else 0x200)
    struct.pack_into("<HH", pm, 36, 1, 0)
    for sector in (35, 51):
        pos = sector * block
        image[pos + 446:pos + 510] = pm
        if kind == "vat20-cross-partition":
            image[pos + 510:pos + 516] = image[pos + 440:pos + 446]
            struct.pack_into("<H", image, pos + 514, 1)
        struct.pack_into("<II", image, pos + 264, 70, 2)
        if kind == "vat20-cross-partition":
            struct.pack_into("<II", image, pos + 264, 76, 3)
        struct.pack_into("<H", image, pos + 256, 1)
        struct.pack_into("<H", image, pos + 240, 0x150 if version15 else 0x200)
        tag(pos, 516 if kind == "vat20-cross-partition" else 510)
    if kind == "vat20-sparable":
        # Packet splits must keep the same VAT base and advance only its offset.
        image[700 * block:704 * block] = image[315 * block:319 * block]
        image[315 * block:319 * block] = b"!" * (4 * block)
        pos = 64 * block
        image[pos:pos + block] = bytes(block)
        struct.pack_into("<HH", image, pos, 0, 2)
        struct.pack_into("<I", image, pos + 12, 64)
        image[pos + 17:pos + 35] = b"*UDF Sparing Table"
        struct.pack_into("<H", image, pos + 48, 2)
        struct.pack_into("<I", image, pos + 52, 1)
        struct.pack_into("<IIII", image, pos + 56, 58, 700, 60, 702)
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
