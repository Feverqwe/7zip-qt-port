"""Independent file chains around unavailable FAT sectors and partial entries."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import struct
from tests.formats.fat.support.fat_fixture import PAYLOAD, LONG_NAME, entry
from tests.formats.fat.support.fat_stream_fixture import stream_volume


def table_volume(kind):
    mode, bits = kind.rsplit("-", 1)
    bits = int(bits)
    v = stream_volume(bits, clusters=10000 if mode == "large" else None)
    chain = ([3, 9000, 8] if mode == "large" else
             {12: [341, 600, 24], 16: [3, 300, 8], 32: [3, 150, 8]}[bits])
    healthy = bytes((i * 17 + 23) & 255 for i in range(1100)) + b"second tail\n"
    for clusters, payload in ((chain, PAYLOAD), ([700, 710, 720], healthy)):
        for i, cluster in enumerate(clusters):
            v.set_fat(cluster, clusters[i + 1] if i + 1 < len(clusters) else v.eof)
            data = payload[i * 512:(i + 1) * 512]
            v.data[v.offset(cluster):v.offset(cluster) + len(data)] = data
    for record in v.records:
        if record[:11] == v.alias:
            struct.pack_into("<H", record, 26, chain[0])
        if record[:11] == b"SECOND  BIN":
            struct.pack_into("<H", record, 26, 700)
    sector, partial, prefix = 0, 0, 512
    if mode == "large":
        assert bits == 32
        sector, prefix = 9000 * 4 // 512, 1024
    elif mode == "middle":
        sector, prefix = 1, 512 if bits == 12 else 1024
    elif mode == "partial":
        partial = {12: 511, 16: 8, 32: 16}[bits]
        prefix = 512 if bits == 12 else len(PAYLOAD)
    elif mode == "entry-prefix":
        sector = 1 if bits == 12 else 0
        partial = {12: 1, 16: 7, 32: 15}[bits]
        prefix = 1024 if bits == 12 else 512
    elif mode == "padding":
        table_bytes = ((v.clusters + 2) * bits + 7) // 8
        sector, partial = divmod(table_bytes, 512)
        assert partial
        prefix = len(PAYLOAD)
    elif mode == "directory":
        v.set_fat(30, v.eof)
        v.records.append(entry(b"FOLDER     ", 30, attr=0x10))
        deleted = bytearray(32)
        deleted[0] = 0xe5
        v.data[v.offset(30):v.offset(30) + 512] = (
            entry(b"KNOWN   TXT", 700, len(healthy)) + deleted * 15)
    elif mode != "first":
        raise ValueError(kind)
    v.write_directory()
    v.expected_files = {LONG_NAME: PAYLOAD[:prefix], "SECOND.BIN": healthy, "EMPTY.TXT": b""}
    if mode == "directory":
        v.expected_files["FOLDER/KNOWN.TXT"] = healthy
    v.table_fault_offset = (v.reserved + sector) * 512
    v.table_partial_bytes = partial
    v.table_limited = mode != "padding"
    v.directory_limited = mode == "directory"
    return v
