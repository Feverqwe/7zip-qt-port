"""Small Apple partition maps, including old installation-image geometry."""
import json
from pathlib import Path
import struct
import sys

CASES = tuple("block-" + str(1 << n) for n in range(9, 16)) + (
    "ddm-reserved", "pm-reserved", "first-count", "map-end", "header-count", "legacy-512",
    "partition-outside", "partition-overflow")
BAD = ("signature", "block-zero", "block-small", "block-nonpower", "map-count-zero",
       "map-count-large", "first-signature", "truncated-map")


def volume(kind):
    block = int(kind[6:]) if kind.startswith("block-") and kind[6:].isdigit() else 512
    header_block = 2048 if kind == "legacy-512" else block
    image = bytearray(64 * header_block)
    struct.pack_into(">HHI", image, 0, 0x4552, header_block, 64)
    payload = bytes(range(256)) * (4 * block // 256)
    image[16 * block:20 * block] = payload
    for i, (start, count, name, typ) in enumerate((
            (1, 3, b"map", b"Apple_partition_map"),
            (16, 4, b"data", b"Apple_HFS")), 1):
        offset = i * block
        struct.pack_into(">HHIII", image, offset, 0x504d,
                         0x1234 if kind == "pm-reserved" else 0,
                         3 if kind == "map-end" else 2, start, count)
        image[offset + 16:offset + 16 + len(name)] = name
        image[offset + 48:offset + 48 + len(typ)] = typ
        struct.pack_into(">I", image, offset + 88, 0x33)
    if kind == "ddm-reserved":
        struct.pack_into(">I", image, 12, 0x12345678)
    elif kind == "first-count":
        struct.pack_into(">I", image, 2 * block + 4, 0xffffffff)
    elif kind == "header-count":
        struct.pack_into(">I", image, 4, 0xffffffff)
    elif kind == "signature":
        image[:2] = b"XX"
    elif kind.startswith("block-") and not kind[6:].isdigit():
        struct.pack_into(">H", image, 2, {
            "block-zero": 0, "block-small": 256, "block-nonpower": 1536}[kind])
    elif kind in ("map-count-zero", "map-count-large"):
        struct.pack_into(">I", image, block + 4, 0 if kind.endswith("zero") else 257)
    elif kind == "first-signature":
        image[block:block + 2] = b"XX"
    elif kind in ("partition-outside", "partition-overflow"):
        struct.pack_into(">II", image, 2 * block + 8,
                         63 if kind == "partition-outside" else 0xffffffff, 4)
        start = 63 if kind == "partition-outside" else 0xffffffff
        payload = bytes(image[start * block:min(len(image), (start + 4) * block)])
    elif kind == "truncated-map":
        struct.pack_into(">I", image, block + 12, 1)
        image = image[:2 * block + 100]
    return bytes(image), payload, block


if __name__ == "__main__":
    image, payload, block = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps({"payload": payload.hex(), "block": block}))
