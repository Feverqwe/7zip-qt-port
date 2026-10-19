"""Finite and cyclic allocation traversals over disposable native UDF images."""
import binascii
import struct
from udf_fixture import volume as base_volume

CASES = ("walk-zero-short", "walk-zero-long", "walk-zero-many",
         "walk-zero-chain-short", "walk-zero-chain-long", "walk-active-eof",
         "walk-aed-tags", "walk-tail-short", "walk-tail-long", "walk-cross-partition",
         "walk-metadata-zero", "walk-metadata-zero-chain")
MODULE_ONLY = ("walk-metadata-cycle-mirror",)
BAD = ("walk-zero-cycle-short", "walk-zero-cycle-long", "walk-zero-cycle-two",
       "walk-zero-prefix-cycle", "walk-data-cycle-short", "walk-data-cycle-long",
       "walk-hole-cycle")


def volume(kind):
    block = 2048
    metadata = kind.startswith("walk-metadata-")
    source_kind = "md-ad-chain" if metadata and kind != "walk-metadata-zero" else "md-fragmented" if metadata else "ad-cross-partition" if kind == "walk-cross-partition" else "ad-short"
    raw, files = base_volume(source_kind)
    image = bytearray(raw)

    def tag(pos, size=None):
        if size is None:
            size = 16 + struct.unpack_from("<H", image, pos + 10)[0]
        struct.pack_into("<HH", image, pos + 8,
                         binascii.crc_hqx(image[pos + 16:pos + size], 0), size - 16)
        image[pos + 4] = 0
        image[pos + 4] = sum(image[pos:pos + 16]) & 255

    if metadata:
        if kind == "walk-metadata-zero":
            pos, header, length_at = 387 * block, 176, 172
        else:
            pos, header, length_at = 390 * block, 24, 20
        length = struct.unpack_from("<I", image, pos + length_at)[0]
        old = bytes(image[pos + header:pos + header + length])
        data = struct.pack("<II", 1 << 30, 0xffffffff) + old
        if kind == "walk-metadata-cycle-mirror":
            data = struct.pack("<IIII", 1 << 30, 0, (3 << 30) | block, 133)
        else:
            # These finite zero ADs must work without falling back to the mirror.
            image[388 * block + 4] ^= 1
        struct.pack_into("<I", image, pos + length_at, len(data))
        image[pos + header:pos + header + len(data)] = data
        tag(pos, header + len(data))
        return bytes(image), files

    long = kind.endswith("long") or kind == "walk-cross-partition"

    def allocation(extents):
        return b"".join(struct.pack("<IIH6x", length | (type << 30), logical, partition)
                         if long else struct.pack("<II", length | (type << 30), logical)
                         for type, length, logical, partition in extents)

    def aed(logical, extents, partition=0, tail=b""):
        pos = ((360 if partition else 257) + logical) * block
        data = allocation(extents) + tail
        image[pos:pos + block] = bytes(block)
        struct.pack_into("<HH", image, pos, 258, 2)
        struct.pack_into("<I", image, pos + 12, logical)
        struct.pack_into("<I", image, pos + 20, len(data))
        image[pos + 24:pos + 24 + len(data)] = data
        tag(pos, 24 + len(data))

    pos = 263 * block
    struct.pack_into("<H", image, pos + 34, int(long))
    first = allocation([(3, block, 21, 0)])
    struct.pack_into("<I", image, pos + 172, len(first))
    image[pos + 176:pos + block] = first + bytes(block - 176 - len(first))
    tag(pos, 176 + len(first))
    zero = (1, 0, 0xffffffff, 0)
    if kind in ("walk-zero-short", "walk-zero-long", "walk-zero-many"):
        aed(21, [zero] * (64 if kind == "walk-zero-many" else 2) + [(0, 8192, 15, 0)])
    elif "zero-chain" in kind:
        aed(21, [zero, (0, block, 15, 0), (3, block, 22, 0)])
        aed(22, [(2, 0, 0, 0), (0, 3 * block, 16, 0)])
    elif kind == "walk-active-eof":
        aed(21, [(0, 8192, 15, 0), (3, block, 21, 0)])
    elif kind == "walk-aed-tags":
        aed(21, [(0, 8192, 15, 0)])
        pos = 278 * block
        struct.pack_into("<HH", image, pos, 0xffff, 0xffff)
        struct.pack_into("<HHI", image, pos + 8, 0x1234, 0xffff, 0xffffffff)
        image[pos + 4] ^= 1
    elif "tail-" in kind:
        aed(21, [(0, block, 15, 0)], tail=b"partial" if long else b"pad")
        files["alpha.bin"] = files["alpha.bin"][:block] + bytes(3 * block)
    elif kind == "walk-cross-partition":
        aed(21, [(0, block, 15, 1), (3, block, 21, 1)])
        aed(21, [(0, 3 * block, 16, 1)], partition=1)
    elif kind in ("walk-zero-cycle-short", "walk-zero-cycle-long"):
        aed(21, [zero, (3, block, 21, 0)])
    elif kind == "walk-zero-cycle-two":
        aed(21, [zero, (3, block, 22, 0)])
        aed(22, [zero, (3, block, 21, 0)])
    elif kind == "walk-zero-prefix-cycle":
        for logical in range(21, 31):
            aed(logical, [zero, (3, block, logical + 1 if logical < 30 else 26, 0)])
    elif kind in ("walk-data-cycle-short", "walk-data-cycle-long"):
        aed(21, [(0, 1, 15, 0), (3, block, 21, 0)])
    elif kind == "walk-hole-cycle":
        aed(21, [(1, block, 0, 0), (3, block, 21, 0)])
    else:
        raise ValueError(kind)
    return bytes(image), files
