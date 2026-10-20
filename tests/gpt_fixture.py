"""Disposable GPTs with independent primary/backup headers and entry arrays."""
import json
from pathlib import Path
import struct
import sys
import uuid
import zlib

CASES = ("sector-512", "sector-1024", "sector-2048", "sector-4096", "header-full",
         "revision", "backup-crc", "backup-array", "backup-signature", "backup-short",
         "primary-only", "primary-wins", "backup-address", "entry-outside", "entry-256")
BAD = ("short-header", "long-header", "usable-first", "usable-last", "usable-reversed",
       "empty-table", "table-outside", "table-truncated", "table-overflow", "entry-reversed",
       "crc", "array-crc", "my-lba", "partial-failure")


def volume(kind):
    sector = int(kind[7:]) if kind.startswith("sector-") else 512
    image = bytearray(128 * sector)
    image[510:512] = b"\x55\xaa"
    image[450] = 0xee
    struct.pack_into("<II", image, 454, 1, 127)
    entry_size = 256 if kind == "entry-256" else 128
    entries = bytearray(entry_size * 4)
    entries[:16] = uuid.UUID("0fc63daf-8483-4772-8e79-3d69d8477de4").bytes_le
    if kind == "windows-probe":
        entries[:16] = uuid.UUID("ebd0a0a2-b9e5-4433-87c0-68b6b72699c7").bytes_le
    entries[16:32] = uuid.UUID("12345678-abcd-4321-9876-0123456789ab").bytes_le
    struct.pack_into("<QQQ", entries, 32, 16, 19, 0)
    name = "данные 空".encode("utf-16le")
    entries[56:56 + len(name)] = name
    payload = bytes(range(256)) * (4 * sector // 256)
    image[16 * sector:20 * sector] = payload
    if kind == "entry-reversed":
        struct.pack_into("<QQ", entries, 32, 19, 16)
    if kind == "entry-outside":
        entries[entry_size:entry_size + 128] = entries[:128]
        struct.pack_into("<QQ", entries, entry_size + 32, 126, 128)
    if kind == "partial-failure":
        entries[entry_size:entry_size + 128] = entries[:128]
        struct.pack_into("<QQ", entries, entry_size + 32, 19, 16)
    count = len(entries) // entry_size
    table_blocks = (len(entries) + sector - 1) // sector
    for lba, table_lba in ((1, 2), (127, 127 - table_blocks)):
        table = bytearray(entries)
        if kind == "primary-wins" and lba == 127:
            struct.pack_into("<QQ", table, 32, 32, 35)
            image[32 * sector:36 * sector] = b"B" * len(payload)
        image[table_lba * sector:table_lba * sector + len(table)] = table
        header = bytearray(sector)
        header[:8] = b"EFI PART"
        struct.pack_into("<IIIIQQQQ", header, 8, 0x10001 if kind == "revision" else 0x10000,
                         sector if kind == "header-full" else 92, 0, 0,
                         lba, 127 if lba == 1 else 1, 8, 119)
        header[56:72] = uuid.UUID("01234567-89ab-cdef-0123-456789abcdef").bytes_le
        struct.pack_into("<QIII", header, 72, table_lba, count, entry_size, zlib.crc32(table))
        if kind == "backup-address" and lba == 1:
            struct.pack_into("<Q", header, 32, 0xffffffffffffffff)
        damaged = kind in BAD or kind.startswith("backup-") and lba == 1 or kind == "primary-only" and lba == 127
        if damaged:
            defect = kind.removeprefix("backup-")
            if defect in ("short", "short-header"):
                struct.pack_into("<I", header, 12, 0)
            elif defect == "long-header":
                struct.pack_into("<I", header, 12, sector + 1)
            elif defect == "usable-first":
                struct.pack_into("<Q", header, 40, 128)
            elif defect == "usable-last":
                struct.pack_into("<Q", header, 48, 128)
            elif defect == "usable-reversed":
                struct.pack_into("<QQ", header, 40, 100, 99)
            elif defect == "empty-table":
                struct.pack_into("<II", header, 80, 0, entry_size)
                struct.pack_into("<I", header, 88, 0)
            elif defect in ("table-outside", "table-truncated", "table-overflow"):
                struct.pack_into("<Q", header, 72, 128 if defect == "table-outside" else
                                 127 if defect == "table-truncated" else 0xffffffffffffffff)
                if defect == "table-truncated":
                    struct.pack_into("<I", header, 80, 8)
            elif defect == "my-lba":
                struct.pack_into("<Q", header, 24, 2)
            elif defect == "signature":
                header[:8] = b"BAD PART"
            elif defect in ("array", "array-crc"):
                header[88] ^= 1
        size = struct.unpack_from("<I", header, 12)[0]
        struct.pack_into("<I", header, 16, zlib.crc32(header[:size]))
        if damaged and (kind in ("crc", "backup-crc", "primary-only")):
            header[16] ^= 1
        image[lba * sector:(lba + 1) * sector] = header
    return bytes(image), payload, sector


if __name__ == "__main__":
    image, payload, sector = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps({"payload": payload.hex(), "sector": sector}))
