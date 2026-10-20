"""Deep directories and cyclic namespace references in a disposable UDF tree."""
import binascii
import struct
from udf_fixture import volume as base_volume

CASES = ("ut-depth-256", "ut-depth-512", "ut-depth-1022")
BAD = ("ut-depth-1023", "ut-cycle-root", "ut-cycle-parent")


def volume(kind):
    original, _ = base_volume("native")
    block = 2048
    depth = int(kind.rsplit("-", 1)[1]) if kind.startswith("ut-depth-") else 8
    leaf = 100 + depth * 2
    image = bytearray(original)
    image.extend(bytes(max(0, (257 + leaf + 2) * block - len(image))))
    payload = b"deep directory payload"

    def tag(pos, size=None):
        if size is None:
            size = 16 + struct.unpack_from("<H", image, pos + 10)[0]
        struct.pack_into("<HH", image, pos + 8,
                         binascii.crc_hqx(image[pos + 16:pos + size], 0), size - 16)
        image[pos + 4] = 0
        image[pos + 4] = sum(image[pos:pos + 16]) & 255

    for sector in (34, 50):
        struct.pack_into("<I", image, sector * block + 192, leaf + 1)
        tag(sector * block)
    for index in range(depth + 1):
        logical = 2 if not index else 100 + (index - 1) * 2
        data_block = 3 if not index else logical + 1
        name = b"d" if index < depth else b"tail.bin"
        target = 100 + index * 2
        if index == depth and kind.startswith("ut-cycle-"):
            target = 2 if kind == "ut-cycle-root" else logical
        entry = bytearray(38) + b"\x08" + name
        entry.extend(bytes((-len(entry)) % 4))
        struct.pack_into("<HH", entry, 0, 257, 2)
        struct.pack_into("<I", entry, 12, data_block)
        struct.pack_into("<H", entry, 16, 1)
        entry[18] = 2 if index < depth or kind.startswith("ut-cycle-") else 0
        entry[19] = len(name) + 1
        struct.pack_into("<IIH", entry, 20, block, target, 0)
        parent = bytearray(original[260 * block:260 * block + 40])
        struct.pack_into("<I", parent, 12, data_block)
        struct.pack_into("<I", parent, 24, 2 if index < 2 else logical - 2)
        directory = parent + entry
        pos = (257 + data_block) * block
        image[pos:pos + block] = directory + bytes(block - len(directory))
        tag(pos, len(parent))
        tag(pos + len(parent), len(entry))
        pos = (257 + logical) * block
        image[pos:pos + block] = original[259 * block:260 * block]
        struct.pack_into("<I", image, pos + 12, logical)
        struct.pack_into("<H", image, pos + 48, 2 if index < depth else 1)
        struct.pack_into("<Q", image, pos + 56, len(directory))
        struct.pack_into("<II", image, pos + 176, len(directory), data_block)
        tag(pos)
    pos = (257 + leaf) * block
    image[pos:pos + block] = original[263 * block:264 * block]
    struct.pack_into("<I", image, pos + 12, leaf)
    struct.pack_into("<H", image, pos + 34, 3)
    struct.pack_into("<QQ", image, pos + 56, len(payload), 0)
    struct.pack_into("<II", image, pos + 168, 0, len(payload))
    image[pos + 176:pos + block] = payload + bytes(block - 176 - len(payload))
    tag(pos, 176 + len(payload))
    # Keep an end anchor outside the enlarged partition for the standalone
    # reader's archive-integrity check as well as Linux's primary-anchor lookup.
    end = len(image) // block - 1
    pos = end * block
    image[pos:pos + block] = original[427 * block:428 * block]
    struct.pack_into("<I", image, pos + 12, end)
    tag(pos)
    return bytes(image), {"d/" * depth + "tail.bin": payload}
