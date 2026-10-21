"""Bounded $Volume state variants; ordinary file mappings remain unchanged."""
from ntfs_fixture import attributes, put, record, u16, u32
from ntfs_ea_fixture import resident, set_attributes
from ntfs_security_fixture import named_resident

CASES = ("volume-clean", "volume-dirty", "volume-resize", "volume-short",
         "volume-missing", "volume-named", "volume-compressed", "volume-duplicate")


def volume(kind):
    from ntfs_fixture import volume as base
    image, files = base()
    image = bytearray(image)
    offset, rec = record(image, 3)
    attrs = [a for _, a in attributes(rec)]
    index = next(i for i, a in enumerate(attrs) if u32(a, 0) == 0x70)
    attr = attrs[index]
    start = u16(attr, 20)
    data = bytearray(attr[start:start + u32(attr, 16)])
    put(data, 10, "H", 1 if kind == "volume-dirty" else 2 if kind == "volume-resize" else 0)
    attrs[index] = resident(0x70, data, u16(attr, 14))
    if kind == "volume-short":
        attrs[index] = resident(0x70, data[:10], u16(attr, 14))
    elif kind == "volume-missing":
        del attrs[index]
    elif kind == "volume-named":
        attrs[index] = named_resident(0x70, "other", data, u16(attr, 14))
    elif kind == "volume-compressed":
        put(attrs[index], 12, "H", 1)
    elif kind == "volume-duplicate":
        put(data, 10, "H", 1)
        attrs.insert(index + 1, resident(0x70, data, u16(attr, 14) + 1))
    elif kind not in CASES:
        raise ValueError(kind)
    set_attributes(image, offset, rec, attrs)
    return bytes(image), files


def limitation(kind):
    if kind == "volume-dirty":
        return "The NTFS volume is marked dirty. Some data may be corrupt."
    if kind in ("volume-short", "volume-missing", "volume-named", "volume-compressed"):
        return "The volume state is unavailable. Files may be inconsistent."
    return ""
