"""Bzip2 framing and block fixtures produced by the standard library."""
from pathlib import Path
import bz2
import json
import sys

CASES = tuple("level-" + str(n) for n in range(1, 10)) + (
    "empty", "padding", "padding-chain", "empty-first", "concatenated",
    "multiple-blocks", "long-run", "all-bytes", "extra-selectors")
BAD = ("magic", "level-zero", "level-ten", "block-signature", "block-crc",
       "stream-crc", "header-short", "body-short", "tail", "second-crc",
       "origin", "groups", "selectors-zero")
END = f"{0x177245385090:048b}"


def bits(image):
    return "".join(f"{b:08b}" for b in image)


def packed(value):
    value += "0" * (-len(value) % 8)
    return bytes(int(value[i:i + 8], 2) for i in range(0, len(value), 8))


def padding(image):
    value = bits(image)
    end = value.rindex(END) + 80
    assert 0 < len(value) - end < 8
    return packed(value[:end] + "1" * (len(value) - end))


def table_fields(value):
    # Block magic/CRC/randomised/origin are followed by the two-level byte map.
    position = 32 + 48 + 32 + 1 + 24
    used = value[position:position + 16].count("1")
    return position + 16 + 16 * used


def volume(kind):
    payload = b"bzip2 payload\x00\xff\n."
    level = int(kind[6:]) if kind.startswith("level-") and kind[6:].isdigit() else 1
    image = bz2.compress(payload, compresslevel=level)
    if kind == "empty":
        payload = b""
        image = bz2.compress(payload)
    elif kind == "padding":
        image = padding(image)
    elif kind == "padding-chain":
        image = padding(image) + bz2.compress(b"second member", compresslevel=9)
        payload += b"second member"
    elif kind == "empty-first":
        image = bz2.compress(b"") + image
    elif kind == "concatenated":
        image += bz2.compress(b"second member", compresslevel=9)
        payload += b"second member"
    elif kind == "multiple-blocks":
        # The LCG is deterministic; the data exceeds three 100k BWT blocks.
        state = 13
        data = bytearray()
        for _ in range(310000):
            state = (state * 1664525 + 1013904223) & 0xffffffff
            data.append(state >> 24)
        payload = bytes(data)
        image = bz2.compress(payload, compresslevel=1)
    elif kind == "long-run":
        payload = b"A" * 400000
        image = bz2.compress(payload)
    elif kind == "all-bytes":
        payload = bytes(range(256)) * 40
        image = bz2.compress(payload)
    elif kind == "extra-selectors":
        value = bits(image)
        position = table_fields(value) + 3
        count = int(value[position:position + 15], 2)
        end = position + 15
        for _ in range(count):
            end = value.index("0", end) + 1
        crc_end = value.rindex(END) + 80
        # Extra rank-zero selectors are never used by the unchanged coded block.
        value = (value[:position] + f"{32767:015b}" + value[position + 15:end]
                 + "0" * (32767 - count) + value[end:crc_end])
        image = packed(value)
    elif kind == "magic":
        image = b"XZ" + image[2:]
    elif kind in ("level-zero", "level-ten"):
        image = image[:3] + (b"0" if kind == "level-zero" else b":") + image[4:]
    elif kind == "block-signature":
        image = image[:4] + bytes((image[4] ^ 1,)) + image[5:]
    elif kind == "block-crc":
        image = image[:10] + bytes((image[10] ^ 1,)) + image[11:]
    elif kind == "stream-crc":
        value = bits(image)
        position = value.rindex(END) + 48
        image = packed(value[:position] + str(1 - int(value[position])) + value[position + 1:])
    elif kind == "header-short":
        image = image[:3]
    elif kind == "body-short":
        image = image[:-7]
    elif kind == "tail":
        image += b"unexpected tail"
    elif kind == "second-crc":
        other, _ = volume("block-crc")
        image += other
    elif kind in ("origin", "groups", "selectors-zero"):
        value = bits(image)
        position, count, replacement = 113, 24, (1 << 24) - 1
        if kind != "origin":
            position = table_fields(value)
            count, replacement = 3, 7
            if kind == "selectors-zero":
                position += 3
                count, replacement = 15, 0
        image = packed(value[:position] + f"{replacement:0{count}b}" + value[position + count:])
    return image, payload


if __name__ == "__main__":
    image, payload = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps({"payload": payload.hex()}))
