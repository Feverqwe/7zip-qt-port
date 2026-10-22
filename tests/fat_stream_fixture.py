"""Two independent fragmented FAT streams sharing one archive source."""
from fat_fixture import Volume, PAYLOAD, LONG_NAME, entry
import struct


def stream_volume(bits, clusters=None):
    if bits not in (12, 16, 32):
        raise ValueError(bits)
    v = Volume(bits, clusters=clusters)
    second = bytes((i * 17 + 23) & 255 for i in range(1100)) + b"second tail\n"
    chain = [10, 15, 20]
    for i, c in enumerate(chain):
        v.set_fat(c, chain[i + 1] if i + 1 < len(chain) else v.eof)
        data = second[i * v.cluster_size:(i + 1) * v.cluster_size]
        v.data[v.offset(c):v.offset(c) + len(data)] = data
    v.records.append(entry(b"SECOND  BIN", chain[0], len(second)))
    v.write_directory()
    v.expected_files = {LONG_NAME: PAYLOAD, "SECOND.BIN": second, "EMPTY.TXT": b""}
    v.stream_offsets = [v.offset(c) for c in v.chain]
    return v


def chain_volume(kind):
    mode, bits = kind.rsplit("-", 1)
    v = stream_volume(int(bits))
    prefix = PAYLOAD[:1024]
    if mode in ("free", "eof", "bad", "range", "self", "cycle", "metadata", "metadata-start"):
        if mode in ("metadata", "metadata-start"):
            v.set_fat(30, v.eof)
            v.records.append(entry(b"FOLDER     ", 30, attr=0x10))
            directory = entry(b"INSIDE  TXT", 0, 0)
            v.data[v.offset(30):v.offset(30) + len(directory)] = directory
        if mode == "metadata-start":
            v.set_fat(30, 31)
            v.set_fat(31, v.eof)
            v.data[v.offset(31):v.offset(31) + v.cluster_size] = b"directory tail\n".ljust(v.cluster_size, b"\0")
            record = next(r for r in v.records if r[:11] == v.alias)
            struct.pack_into("<H", record, 26, 30)
            struct.pack_into("<I", record, 28, 2 * v.cluster_size)
            prefix = bytes(v.data[v.offset(30):v.offset(30) + v.cluster_size])
            prefix += bytes(v.data[v.offset(31):v.offset(31) + v.cluster_size])
            v.legacy_file_prefix = b""
        elif mode == "metadata":
            v.legacy_file_prefix = prefix
            prefix += bytes(v.data[v.offset(30):v.offset(30) + len(PAYLOAD) - len(prefix)])
        value = {"free": 0, "eof": v.eof, "bad": v.eof - 8,
                 "range": v.clusters + 2, "self": v.chain[1],
                 "cycle": v.chain[0], "metadata": 30, "metadata-start": v.eof}[mode]
        if mode != "metadata-start":
            v.set_fat(v.chain[1], value)
    elif mode == "start":
        record = next(r for r in v.records if r[:11] == v.alias)
        struct.pack_into("<H", record, 26, 0)
        prefix = b""
    elif mode == "oversized":
        record = next(r for r in v.records if r[:11] == v.alias)
        struct.pack_into("<I", record, 28, 0xffffffff)
        prefix = PAYLOAD.ljust(1536, b"\0")
    elif mode == "tail":
        v.set_fat(v.chain[-1], v.chain[-1])
        prefix = PAYLOAD
    elif mode == "shared":
        record = next(r for r in v.records if r[:11] == b"SECOND  BIN")
        struct.pack_into("<H", record, 26, v.chain[0])
        struct.pack_into("<I", record, 28, len(PAYLOAD))
        v.expected_files["SECOND.BIN"] = PAYLOAD
        prefix = PAYLOAD
    else:
        raise ValueError(kind)
    v.file_limited = mode not in ("tail", "shared")
    v.declared_file_size = (0xffffffff if mode == "oversized" else
                            2 * v.cluster_size if mode == "metadata-start" else len(PAYLOAD))
    v.expected_files[LONG_NAME] = prefix
    v.write_directory()
    return v
