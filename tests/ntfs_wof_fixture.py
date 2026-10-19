"""WOF images with independent official wimlib-compressed frames."""
import base64
import hashlib
import json
import struct
from pathlib import Path


def payloads(bits):
    size = 1 << bits
    random = b"".join(hashlib.sha256(str(i).encode()).digest() for i in range(size // 32))
    calls = b"prefix\xe8\x03\0\0\0abcdefghijklmno"
    return [b"A" * size, random, (calls * ((size + len(calls) - 1) // len(calls)))[:size],
            ((b"last frame unicode " + "данные 空".encode()) * 20)[:513]]


def volume(kind):
    from ntfs_fixture import volume as base_volume, record, save_record, attributes
    from ntfs_fixture import u16, u32, nonresident, put, replace_attribute
    image, expected = base_volume()
    image = bytearray(image)
    large = kind.endswith("-large")
    if large:
        image.extend(bytes(len(image)))
        put(image, 40, "Q", len(image) // 512 - 1)
        image[-512:] = image[:512]
    codec = next(name for name in ("xpress4k", "xpress8k", "xpress16k", "lzx32k") if name in kind)
    bits, method = {"xpress4k": (12, 0), "xpress8k": (13, 2), "xpress16k": (14, 3), "lzx32k": (15, 1)}[codec]
    data = payloads(bits)
    records = json.loads((Path(__file__).parent / "fixtures/ntfs/wof-frames.json").read_text())["frames"][codec]
    frames = [base64.b64decode(frame["packed"]) for frame in records]
    for raw, frame in zip(data, records):
        assert hashlib.sha256(raw).hexdigest() == frame["sha256"]
    if "resident" in kind:
        frames, data = frames[:1], data[:1]
    if large:
        frames = [frames[0]] * 131072 + frames[-1:]
        data = [data[0]] * 131072 + data[-1:]
    offsets, end = [], 0
    for frame in frames[:-1]:
        end += len(frame)
        offsets.append(end)
    ads = bytearray(struct.pack("<" + ("Q" if large else "I") * len(offsets), *offsets) + b"".join(frames))
    if "bad-table" in kind:
        put(ads, 4, "I", offsets[0] - 1)
    elif "bad-frame" in kind:
        # A valid literal followed by a bad block/match overwrites output before failing.
        start = 4 * len(offsets) + offsets[1]
        if codec == "lzx32k":
            header = (3 << 17) | 1  # Uncompressed LZX block, explicit size 1.
            header <<= 12
            bad = struct.pack("<HHIII", header >> 16, header & 0xffff, 1, 1, 1) + b"Z\0\0\0"
        else:
            bits_value = ((90 << 24) | (511 << 15) | 0x7fff) << 15
            bad = b"\x99" * 256 + b"".join(struct.pack("<H", (bits_value >> shift) & 0xffff) for shift in (32, 16, 0))
        ads[start:start + len(frames[2])] = bad + bytes(len(frames[2]) - len(bad))
    elif "short-ads" in kind:
        ads = ads[:4]
    offset, rec = record(image, 66)
    original = [attr for _, attr in attributes(rec)]
    data_attr = next(a for a in original if u32(a, 0) == 0x80)
    mapping = u16(data_attr, 32)
    lcn = int.from_bytes(data_attr[mapping + 2:mapping + 4], "little")
    if large:
        lcn = 4096
    size = sum(map(len, data))
    valid = (1 << bits) + 37 if "initialized" in kind else size
    if "uninitialized" in kind:
        valid = 0
    unnamed = nonresident(data_attr, [((size + 4095) // 4096, None)], size, valid, flags=0x8000)
    name = "WofCompressedData".encode("utf-16-le")
    if "resident" in kind:
        start = (24 + len(name) + 7) & ~7
        named = bytearray((start + len(ads) + 7) & ~7)
        put(named, 0, "II", 0x80, len(named))
        put(named, 16, "IH", len(ads), start)
        named[start:start + len(ads)] = ads
    else:
        named = nonresident(data_attr, [((len(ads) + 4095) // 4096, lcn)], len(ads))
        start = (64 + len(name) + 7) & ~7
        mapping = named[64:]
        named = named[:64] + bytearray(start - 64) + mapping
        put(named, 4, "I", len(named))
        put(named, 32, "H", start)
        image[lcn * 4096:lcn * 4096 + len(ads)] = ads
    named[9] = len(name) // 2
    put(named, 10, "H", 24 if "resident" in kind else 64)
    put(named, 14, "H", 3)
    named[u16(named, 10):u16(named, 10) + len(name)] = name
    reparse_data = struct.pack("<IHHIIII", 0x80000017, 16, 0, 1, 2, 1, method)
    reparse = bytearray(48)
    put(reparse, 0, "II", 0xc0, len(reparse))
    put(reparse, 14, "HIH", 4, len(reparse_data), 24)
    reparse[24:] = reparse_data
    attrs = []
    for attr in original:
        if u32(attr, 0) == 0x80:
            attrs += [unnamed, reparse] if "uninitialized" in kind else [unnamed, named, reparse]
        else:
            if u32(attr, 0) == 0x10:
                put(attr, u16(attr, 20) + 32, "I", 0x620)
            attrs.append(attr)
    start = u16(rec, 20)
    contents = b"".join(attrs) + b"\xff\xff\xff\xff\0\0\0\0"
    rec[start:] = contents + bytes(1024 - start - len(contents))
    put(rec, 24, "I", start + len(contents))
    save_record(image, offset, rec)
    if large:
        expected.pop("gamma")
        expected["large-first"] = data[0]
        expected["large-last"] = data[-1]
        bitmap_offset, bitmap_record = record(image, 6)
        pos, bitmap_attr = next((p, a) for p, a in attributes(bitmap_record) if u32(a, 0) == 0x80)
        put(bitmap_attr, 48, "QQ", 1024, 1024)
        replace_attribute(bitmap_record, pos, bitmap_attr)
        save_record(image, bitmap_offset, bitmap_record)
        run = u16(bitmap_attr, 32)
        n, m = bitmap_attr[run] & 15, bitmap_attr[run] >> 4
        bitmap_lcn = int.from_bytes(bitmap_attr[run + 1 + n:run + 1 + n + m], "little")
        for cluster in range(lcn, lcn + (len(ads) + 4095) // 4096):
            image[bitmap_lcn * 4096 + cluster // 8] |= 1 << (cluster % 8)
    else:
        expected["gamma"] = b"".join(data)[:valid] + bytes(size - valid)
        if "uninitialized" not in kind:
            expected["gamma:WofCompressedData"] = bytes(ads)
    return bytes(image), expected
