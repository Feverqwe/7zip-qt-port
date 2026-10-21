"""NTFS creator image and controlled read-path mutations (no mount/repair writes)."""
import argparse
import base64
import json
from pathlib import Path
import struct
import zlib

FIXTURE = Path(__file__).parent / "fixtures" / "ntfs" / "mkntfs.img.zlib"
ALPHA = bytes(range(256)) * 512
FILES = {"alpha": ALPHA, "данные 空": b"NTFS resident payload", "gamma": b"B" * 65536}


def u16(b, p):
    return struct.unpack_from("<H", b, p)[0]


def u32(b, p):
    return struct.unpack_from("<I", b, p)[0]


def u64(b, p):
    return struct.unpack_from("<Q", b, p)[0]


def put(b, p, fmt, *values):
    struct.pack_into("<" + fmt, b, p, *values)


def record(image, number):
    offset = u64(image, 0x30) * 4096 + number * 1024
    rec = bytearray(image[offset:offset + 1024])
    usa = u16(rec, 4)
    for sector in (1, 2):
        rec[sector * 512 - 2:sector * 512] = rec[usa + sector * 2:usa + sector * 2 + 2]
    return offset, rec


def save_record(image, offset, rec):
    usa = u16(rec, 4)
    for sector in (1, 2):
        rec[usa + sector * 2:usa + sector * 2 + 2] = rec[sector * 512 - 2:sector * 512]
        rec[sector * 512 - 2:sector * 512] = rec[usa:usa + 2]
    image[offset:offset + 1024] = rec


def attributes(rec):
    pos = u16(rec, 20)
    while u32(rec, pos) != 0xffffffff:
        length = u32(rec, pos + 4)
        yield pos, bytearray(rec[pos:pos + length])
        pos += length


def replace_attribute(rec, offset, value):
    length = u32(rec, offset + 4)
    used = u32(rec, 24)
    tail = rec[offset + length:used]
    new_used = offset + len(value) + len(tail)
    assert new_used <= 1024
    rec[offset:1024] = value + tail + bytes(1024 - new_used)
    put(rec, 24, "I", new_used)


def runs(entries):
    result = bytearray()
    prev = 0
    for count, lcn in entries:
        length = count.to_bytes(max(1, (count.bit_length() + 7) // 8), "little")
        delta = b""
        if lcn is not None:
            distance = lcn - prev
            n = next(n for n in range(1, 9) if -(1 << (8 * n - 1)) <= distance < (1 << (8 * n - 1)))
            delta = distance.to_bytes(n, "little", signed=True)
            prev = lcn
        result += bytes([len(length) | len(delta) << 4]) + length + delta
    return result + b"\0"


def nonresident(original, entries, size, valid=None, flags=0, unit=0):
    header_size = 72 if flags & 0x8001 else 64
    mapping = runs(entries)
    value = bytearray((header_size + len(mapping) + 7) & ~7)
    value[:16] = original[:16]
    put(value, 4, "I", len(value))
    put(value, 12, "H", flags)
    count = sum(n for n, _ in entries)
    put(value, 16, "QQHH", 0, count - 1, header_size, unit)
    put(value, 40, "QQQ", count * 4096, size, size if valid is None else valid)
    if header_size == 72:
        put(value, 64, "Q", sum(n for n, lcn in entries if lcn is not None) * 4096)
    value[header_size:header_size + len(mapping)] = mapping
    return value


def volume(kind="base"):
    if kind.startswith("volume-"):
        from ntfs_volume_fixture import volume as volume_state
        return volume_state(kind)
    if kind.startswith("bitmap-"):
        from ntfs_bitmap_fixture import volume as bitmap_volume
        return bitmap_volume(kind)
    if kind.startswith("flags-"):
        from ntfs_flags_fixture import volume as flags_volume
        return flags_volume(kind)
    if kind.startswith("standard-"):
        from ntfs_standard_fixture import volume as standard_volume
        return standard_volume(kind)
    if kind.startswith("link-"):
        from ntfs_link_fixture import volume as link_volume
        return link_volume(kind)
    if kind.startswith("security-"):
        from ntfs_security_fixture import volume as security_volume
        return security_volume(kind)
    if kind.startswith("ea-"):
        from ntfs_ea_fixture import volume as ea_volume
        return ea_volume(kind)
    if kind.startswith("mft-list"):
        from ntfs_mft_fixture import volume as mft_volume
        return mft_volume(kind)
    if any(kind.startswith("wof-" + codec) for codec in ("xpress", "lzx")):
        from ntfs_wof_fixture import volume as wof_volume
        return wof_volume(kind)
    image = bytearray(zlib.decompress(FIXTURE.read_bytes()))
    expected = dict(FILES)
    offset, rec = record(image, 64)
    pos, attr = next((p, a) for p, a in attributes(rec) if u32(a, 0) == 0x80)
    # The official creator allocated one contiguous 32-cluster alpha extent.
    mapping = u16(attr, 32)
    lcn = int.from_bytes(attr[mapping + 2:mapping + 4], "little")
    if kind == "sparse":
        attr = nonresident(attr, [(1, lcn), (30, None), (1, lcn + 31)], len(ALPHA), flags=0x8000)
        expected["alpha"] = ALPHA[:4096] + bytes(30 * 4096) + ALPHA[-4096:]
    elif kind == "hole-first":
        attr = nonresident(attr, [(1, None), (31, lcn + 1)], len(ALPHA), flags=0x8000)
        expected["alpha"] = bytes(4096) + ALPHA[4096:]
    elif kind == "initialized":
        put(attr, 56, "Q", 12345)
        expected["alpha"] = ALPHA[:12345] + bytes(len(ALPHA) - 12345)
    elif kind in ("compressed", "short-compressed", "bad-compressed"):
        attr = nonresident(attr, [(1, lcn), (15, None)], 65536, flags=1, unit=4)
        if kind == "short-compressed":
            payload, expected["alpha"] = b"\x01\xb0\x00A", b"A" + bytes(65535)
        elif kind == "bad-compressed":
            payload = b"\x02\xb0\x01\x00\x00"  # Back reference before output start.
            expected.pop("alpha")
        else:
            payload, expected["alpha"] = b"\x03\xb0\x02A\xfc\x0f" * 16, b"A" * 65536
        image[lcn * 4096:(lcn + 1) * 4096] = payload + bytes(4096 - len(payload))
    elif kind == "cache-collision":
        entries = [(1, lcn), (15, None), (1, lcn + 1), (15, None), (1, lcn + 2), (15, None)]
        attr = nonresident(attr, entries, 3 * 65536, flags=1, unit=4)
        for index, payload in enumerate((b"\x03\xb0\x02A\xfc\x0f" * 16,
                                         b"\x03\xb0\x02B\xfc\x0f" * 16,
                                         b"\x03\xb0\x02Z\x00\x10")):
            image[(lcn + index) * 4096:(lcn + index + 1) * 4096] = payload + bytes(4096 - len(payload))
        expected["alpha"] = b"A" * 65536 + b"B" * 65536
    elif kind == "encrypted":
        put(attr, 12, "H", 0x4000)
        expected.pop("alpha")
    elif kind == "run-over-volume":
        attr = nonresident(attr, [(32, len(image) // 4096 - 16)], len(ALPHA))
        expected.pop("alpha")
    elif kind == "run-underflow":
        attr = nonresident(attr, [(1, 1), (31, -2)], len(ALPHA))
        expected.pop("alpha")
    elif kind == "normal-virtual-over-volume":
        attr = nonresident(attr, [(64, lcn + (i % 2) * 64) for i in range(64)], 4096 * 4096)
        expected.pop("alpha")
    elif kind == "invalid-initialized":
        put(attr, 56, "Q", len(ALPHA) + 1)
    replace_attribute(rec, pos, attr)
    save_record(image, offset, rec)
    if kind == "sector4k":
        for boot in (0, len(image) - 4096, len(image) - 512):
            image[boot:boot + 512] = image[:512]
            put(image, boot + 11, "H", 4096)
            image[boot + 13] = 1
            put(image, boot + 40, "Q", len(image) // 4096 - 1)
    elif kind == "boot-reserved":
        image[0:3] = b"\0\0\0"
        image[14:21] = b"ignored"
        image[510:512] = b"\0\0"
        image[65:68] = b"abc"
        image[69:72] = b"def"
    elif kind == "backup-boot":
        image[11:13] = b"\0\0"
    elif kind in ("reparse", "wof", "wof-empty"):
        offset, rec = record(image, 66)
        attrs = list(attributes(rec))
        pos, data_attr = next((p, a) for p, a in attrs if u32(a, 0) == 0x80)
        data_lcn = int.from_bytes(data_attr[u16(data_attr, 32) + 2:u16(data_attr, 32) + 4], "little")
        target = "alpha".encode("utf-16-le")
        payload = struct.pack("<IHHHHHHI", 0xa000000c, 12 + len(target), 0, 0, len(target), 0, len(target), 1) + target
        if kind.startswith("wof"):
            payload = struct.pack("<IHHIIII", 0x80000017, 16, 0, 1, 2, 1, 0)
        image[(data_lcn + 1) * 4096:(data_lcn + 1) * 4096 + len(payload)] = payload
        data_attr = nonresident(data_attr, [(1, data_lcn)], 4096)
        replace_attribute(rec, pos, data_attr)
        reparse = nonresident(data_attr, [(1, data_lcn + 1)], len(payload))
        put(reparse, 0, "I", 0xc0)
        end = u32(rec, 24) - 8
        rec[end:] = reparse + b"\xff\xff\xff\xff\0\0\0\0" + bytes(1024 - end - len(reparse) - 8)
        put(rec, 24, "I", end + len(reparse) + 8)
        std = next(a for _, a in attributes(rec) if u32(a, 0) == 0x10)
        std_pos = next(p for p, a in attributes(rec) if u32(a, 0) == 0x10)
        put(rec, std_pos + u16(std, 20) + 32, "I", 0x420)
        if kind == "wof-empty":
            pos, attr = next((p, a) for p, a in attributes(rec) if u32(a, 0) == 0x80)
            replace_attribute(rec, pos, b"")
        save_record(image, offset, rec)
        if kind.startswith("wof"):
            expected.pop("gamma")
        else:
            expected["gamma"] = b"alpha"
    return bytes(image), expected


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--kind", default="base")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    image, files = volume(args.kind)
    if args.output:
        args.output.write_bytes(image)
    else:
        result = {"size": len(image), "image": base64.b64encode(zlib.compress(image, 9)).decode(),
                  "files": {name: base64.b64encode(data).decode() for name, data in files.items()}}
        if args.kind.startswith("ea-"):
            from ntfs_ea_fixture import properties
            result["wsl"] = properties(args.kind)
        if args.kind.startswith("security-"):
            from ntfs_security_fixture import expected
            result["security"] = expected(args.kind).hex()
        if args.kind.startswith("standard-"):
            from ntfs_standard_fixture import expected
            result["security"] = expected(args.kind).hex()
        if args.kind.startswith("flags-"):
            from ntfs_flags_fixture import properties
            result["attributes"] = properties(args.kind)
        if args.kind.startswith("volume-"):
            from ntfs_volume_fixture import limitation
            result["volume_limitation"] = limitation(args.kind)
            if args.kind.startswith("volume-list-"):
                from ntfs_volume_list_fixture import label
                result["volume_label"] = label(args.kind)
        if args.kind.startswith("bitmap-"):
            from ntfs_bitmap_fixture import expected
            result["free_space"] = expected(args.kind, image)
        print(json.dumps(result))
