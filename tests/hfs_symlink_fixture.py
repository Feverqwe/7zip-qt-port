"""Page-backed HFS+ symlink targets and independent raw fork bytes."""
import struct

PAYLOADS = {"empty": b"", "nul": b"alpha\0hidden", "first-nul": b"\0hidden",
            "limit": b"T" * 4095, "page": b"T" * 4096, "large": b"T" * 65537}


def payload(kind):
    return PAYLOADS[kind.removeprefix("modern-symlink-")]


def target(kind):
    return payload(kind)[:4095].split(b"\0", 1)[0]


def apply(image, records, block, kind, files):
    data = payload(kind)
    record = records["symlink"]
    allocation = struct.unpack_from(">I", image, 1024 + 0x70 + 16)[0] * block
    blocks = struct.unpack_from(">I", image, 1024 + 44)[0]
    old = struct.unpack_from(">I", image, record + 104)[0]
    count = (len(data) + block - 1) // block
    start = 0
    run = 0
    for index in range(blocks):
        if image[allocation + index // 8] & (0x80 >> (index % 8)):
            run = 0
        else:
            run += 1
            if run == count and count:
                start = index + 1 - count
                break
    if count and not start:
        raise ValueError("no free contiguous extent for symlink fixture")
    image[allocation + old // 8] &= ~(0x80 >> (old % 8))
    for index in range(start, start + count):
        image[allocation + index // 8] |= 0x80 >> (index % 8)
    for header in (1024, len(image) - 1024):
        free = struct.unpack_from(">I", image, header + 48)[0]
        struct.pack_into(">I", image, header + 48, free + 1 - count)
    struct.pack_into(">Q", image, record + 88, len(data))
    struct.pack_into(">I", image, record + 100, count)
    image[record + 104:record + 168] = bytes(64)
    if count:
        struct.pack_into(">II", image, record + 104, start, count)
        image[start * block:(start + count) * block] = data.ljust(count * block, b"\0")
    files["HfsFixture/symlink"] = data
