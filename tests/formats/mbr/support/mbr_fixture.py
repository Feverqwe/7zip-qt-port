"""Primary/EBR images with explicit expected readable partition byte spans."""
import json
from pathlib import Path
import struct
import sys

CASES = ("primary", "unsorted", "overlap", "chs", "type-zero", "empty", "clamp",
         "outside", "lba-end-overflow", "zero-lba", "extended-5", "extended-15",
         "extended-133", "link-first", "os2", "garbage", "backwards", "first-link",
         "cycle", "ebr-signature", "ebr-status", "long-chain", "extended-clamp")
BAD = ("signature", "primary-status", "empty-status", "short")


def volume(kind):
    sector = 512
    if kind.startswith("device-"):
        sector = int(kind[7:])
        kind = "extended-5"
    blocks = 2048 if kind == "long-chain" else 128 if kind in CASES[10:] else 64
    image = bytearray().join(bytes([i % 251]) * 512 for i in range(blocks))
    expected = []

    def table(lba, entries):
        offset = lba * 512
        image[offset:offset + 512] = bytes(512)
        image[offset + 510:offset + 512] = b"\x55\xaa"
        for slot, (typ, start, size) in enumerate(entries):
            p = offset + 446 + slot * 16
            image[p + 4] = typ
            struct.pack_into("<II", image, p + 8, start, size)

    if kind.startswith("extended-") or kind in CASES[13:]:
        typ = int(kind[9:]) if kind.startswith("extended-") and kind[9:].isdigit() else 5
        table(0, [(typ, 8, blocks - 20)])
        table(8, [(0x83, 2, 4), (typ, 32, blocks - 52)])
        table(40, [(0x83, 2, 4)])
        expected = [(10, 4), (42, 4)]
        if kind == "link-first":
            table(8, [(typ, 32, blocks - 52), (0x83, 2, 4), (0, 0, 0), (0x83, 8, 4)])
            expected = [(10, 4), (16, 4), (42, 4)]
        elif kind == "os2":
            table(8, [(0x83, 2, 4), (0x83, 8, 4), (0x83, 16, 4), (typ, 32, blocks - 52)])
            expected = [(10, 4), (16, 4), (24, 4), (42, 4)]
        elif kind == "garbage":
            table(8, [(0x83, 2, 4), (typ, 32, blocks - 52), (0x83, 110, 4)])
        elif kind == "backwards":
            table(40, [(0x83, 2, 4), (typ, 16, blocks - 36)])
            table(24, [(0x83, 2, 4)])
            expected += [(26, 4)]
        elif kind == "first-link":
            table(8, [(0x83, 2, 4), (typ, 32, blocks - 52), (typ, 64, blocks - 84)])
            table(72, [(0x83, 2, 4)])
        elif kind == "cycle":
            table(40, [(0x83, 2, 4), (typ, 0, blocks - 20)])
        elif kind == "ebr-signature":
            image[40 * 512 + 510] = 0
            expected = [(10, 4)]
        elif kind == "ebr-status":
            image[8 * 512 + 446] = 0x7f
        elif kind == "extended-clamp":
            table(0, [(typ, 8, 200)])
            table(8, [(0x83, 2, 4), (0, 0, 0), (0x83, 110, 40)])
            expected = [(10, 4), (118, 10)]
        elif kind == "long-chain":
            expected = []
            for i in range(170):
                lba = 8 + i * 8
                entries = [(0x83, 2, 2)]
                if i + 1 < 170:
                    entries += [(typ, (i + 1) * 8, blocks - 20 - (i + 1) * 8)]
                table(lba, entries)
                expected += [(lba + 2, 2)]
    else:
        entries = [(0x83, 16, 4)]
        if kind == "unsorted":
            entries = [(0x83, 48, 4), (0x83, 16, 4)]
        elif kind == "overlap":
            entries = [(0x83, 16, 8), (0x83, 20, 8)]
        elif kind == "type-zero":
            entries = [(0, 16, 4)]
        elif kind == "empty":
            entries += [(0x83, 0, 0)]
        elif kind == "clamp":
            entries = [(0x83, 60, 16)]
        elif kind == "outside":
            entries = [(0x83, 70, 4), (0x83, 16, 4)]
        elif kind == "lba-end-overflow":
            entries = [(0x83, 16, 0xffffffff)]
        elif kind == "zero-lba":
            entries = [(0x83, 0, 1)]
        table(0, entries)
        expected = [(start, min(size, blocks - start)) for _, start, size in entries
                    if size and start < blocks]
        if kind == "chs":
            image[447:450] = image[451:454] = b"\x01\x00\x00"
        elif kind == "signature":
            image[510] = 0
        elif kind in ("primary-status", "empty-status"):
            image[446 if kind == "primary-status" else 462] = 0x7f
        elif kind == "short":
            image = image[:511]
    if sector != 512:
        image = bytearray().join(image[i:i + 512] * (sector // 512)
                                 for i in range(0, len(image), 512))
    parts = [dict(offset=start * sector, size=size * sector,
                  payload=bytes(image[start * sector:(start + size) * sector]).hex())
             for start, size in expected]
    return bytes(image), parts


if __name__ == "__main__":
    image, parts = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps(parts))
