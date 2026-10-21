"""Two independent fragmented FAT streams sharing one archive source."""
from fat_fixture import Volume, PAYLOAD, LONG_NAME, entry


def stream_volume(bits):
    if bits not in (12, 16, 32):
        raise ValueError(bits)
    v = Volume(bits)
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
