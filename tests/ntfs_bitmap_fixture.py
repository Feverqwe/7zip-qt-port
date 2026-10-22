"""Allocation bitmap windows, partial words and unavailable volume statistics."""
from ntfs_fixture import attributes, nonresident, put, record, u16, u32, u64
from ntfs_ea_fixture import resident, set_attributes
from ntfs_security_fixture import read_stream

VALID = ("bitmap-base", "bitmap-padding", "bitmap-last", "bitmap-full",
         "bitmap-fragmented", "bitmap-resident", "bitmap-uninitialized",
         "bitmap-encrypted", "bitmap-compressed", "bitmap-tail-hole")
BAD = ("bitmap-short", "bitmap-bad-run", "bitmap-hole")


def expected(kind, image):
    if kind in BAD:
        return None
    _, rec = record(image, 6)
    data = next(a for _, a in attributes(rec) if u32(a, 0) == 0x80 and not a[9])
    bits = u64(image, 40) // 8
    occupied = bin(int.from_bytes(read_stream(image, data), "little") & ((1 << bits) - 1)).count("1")
    return (bits - occupied) * 4096


def volume(kind):
    from ntfs_fixture import volume as base
    original, files = base()
    image = bytearray(original)
    offset, rec = record(image, 6)
    attrs = [a for _, a in attributes(rec)]
    data = next(a for a in attrs if u32(a, 0) == 0x80)
    bitmap = bytearray(read_stream(image, data))
    bits = u64(image, 40) // 8
    mapping = u16(data, 32)
    lcn = int.from_bytes(data[mapping + 2:mapping + 4], "little")
    if kind == "bitmap-padding":
        bitmap[bits // 8] ^= 0xff << (bits % 8) & 0xff
    elif kind == "bitmap-last":
        bitmap[(bits - 1) // 8] |= 1 << ((bits - 1) % 8)
    elif kind == "bitmap-full":
        bitmap[:] = b"\xff" * len(bitmap)
    elif kind == "bitmap-fragmented":
        bits = 32771
        image.extend(bytes(bits * 4096 + 512 - len(image)))
        put(image, 40, "Q", bits * 8)
        image[-512:] = image[:512]
        bitmap.extend(b"\xa5" * (4096 - len(bitmap)))
        bitmap += b"\xf8" + b"\xff" * 7
        for cluster in (1100, 1500):
            bitmap[cluster // 8] |= 1 << (cluster % 8)
        attrs[attrs.index(data)] = nonresident(data, [(1, 1100), (1, 1500)], len(bitmap))
        image[1100 * 4096:1101 * 4096] = bitmap[:4096]
        image[1500 * 4096:1500 * 4096 + 8] = bitmap[4096:]
    elif kind == "bitmap-resident":
        attrs[attrs.index(data)] = resident(0x80, bitmap, u16(data, 14))
    elif kind == "bitmap-short":
        put(data, 48, "QQ", 8, 8)
    elif kind == "bitmap-uninitialized":
        put(data, 56, "Q", 0)
    elif kind == "bitmap-encrypted":
        put(data, 12, "H", 0x4000)
    elif kind == "bitmap-compressed":
        attrs[attrs.index(data)] = nonresident(data, [(1, lcn)], len(bitmap), flags=1, unit=4)
    elif kind == "bitmap-tail-hole":
        attrs[attrs.index(data)] = nonresident(data, [(1, lcn), (1, None)], 8192, flags=0x8000)
    elif kind == "bitmap-bad-run":
        attrs[attrs.index(data)] = nonresident(data, [(1, bits + 1)], len(bitmap))
    elif kind == "bitmap-hole":
        attrs[attrs.index(data)] = nonresident(data, [(1, None)], len(bitmap), flags=0x8000)
    elif kind != "bitmap-base":
        raise ValueError(kind)
    if kind != "bitmap-fragmented":
        image[lcn * 4096:lcn * 4096 + len(bitmap)] = bitmap
    set_attributes(image, offset, rec, attrs)
    return bytes(image), files
