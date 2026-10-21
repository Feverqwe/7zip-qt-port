"""NTFS standard information boundaries and metadata ownership."""
import struct
from ntfs_fixture import attributes, nonresident, put, record, u16, u32, u64
from ntfs_ea_fixture import allocated, resident, set_attributes
from ntfs_security_fixture import descriptor, read_stream

VALID = ("standard-full", "standard-legacy", "standard-partial56", "standard-partial64",
         "standard-duplicate", "standard-extension", "standard-deleted-short")
BAD = ("standard-short", "standard-nonresident", "standard-missing", "standard-bad-second",
       "standard-record-total")
MTIME = 132537600000000000


def expected(kind):
    return b"" if kind in ("standard-legacy", "standard-partial56", "standard-partial64",
                           "standard-deleted-short") else descriptor(700)


def volume(kind):
    from ntfs_security_fixture import volume as security_volume
    original, files = security_volume("security-index")
    image = bytearray(original)
    offset, rec = record(image, 66)
    attrs = [a for _, a in attributes(rec)]
    std = next(a for a in attrs if u32(a, 0) == 0x10)
    payload = bytearray(read_stream(image, std))
    put(payload, 8, "Q", MTIME)
    put(payload, 32, "I", 0x21)
    length = {"standard-legacy": 48, "standard-partial56": 56, "standard-partial64": 64,
              "standard-short": 47, "standard-deleted-short": 47}.get(kind, 72)
    value = resident(0x10, payload[:length], u16(std, 14))
    attrs[attrs.index(std)] = value
    if kind in ("standard-duplicate", "standard-bad-second"):
        second = bytearray(payload)
        put(second, 8, "Q", MTIME + 864000000000)
        put(second, 32, "I", 0x20)
        put(second, 52, "I", 500)
        attrs.insert(1, resident(0x10, second[:47] if kind == "standard-bad-second" else second, 30))
    elif kind == "standard-extension":
        attrs.remove(value)
        extension = bytearray(rec)
        put(extension, 16, "H", 2)
        put(extension, 32, "Q", 66 | (u16(rec, 16) << 48))
        put(extension, 44, "I", 16)
        set_attributes(image, u64(image, 0x30) * 4096 + 16 * 1024, extension, [value])
        rows = [(0x10, 16 | (2 << 48), u16(value, 14))]
        rows += [(u32(a, 0), 66 | (u16(rec, 16) << 48), u16(a, 14)) for a in attrs]
        content = b"".join(struct.pack("<IHBBQQH6x", typ, 32, 0, 26, 0, ref, ident)
                           for typ, ref, ident in rows)
        attrs.insert(0, resident(0x20, content, 31))
        allocated(image, 0, 0xB0, [16])
    elif kind == "standard-nonresident":
        data = next(a for a in attrs if u32(a, 0) == 0x80)
        mapping = u16(data, 32)
        lcn = int.from_bytes(data[mapping + 2:mapping + 4], "little")
        value = nonresident(data, [(16, lcn)], 65536) + bytes(72)
        put(value, 0, "II", 0x10, len(value))
        attrs[0] = value
    elif kind == "standard-missing":
        attrs.remove(value)
    elif kind == "standard-deleted-short":
        put(rec, 22, "H", u16(rec, 22) & ~1)
        files.pop("gamma")
    elif kind == "standard-record-total":
        put(rec, 28, "I", 512)
    elif kind not in VALID and kind != "standard-short":
        raise ValueError(kind)
    set_attributes(image, offset, rec, attrs)
    return bytes(image), files
