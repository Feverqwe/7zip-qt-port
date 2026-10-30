"""$Volume attributes selected by resident/nonresident attribute lists."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import struct
from tests.formats.ntfs.support.ntfs_fixture import attributes, nonresident, put, record, u16, u32
from tests.formats.ntfs.support.ntfs_ea_fixture import allocated, resident, set_attributes

LABEL = "Том 空"
CASES = ("volume-list-clean", "volume-list-dirty", "volume-list-nonresident",
         "volume-list-base", "volume-list-override", "volume-list-bad-id",
         "volume-list-bad-sequence", "volume-list-wrong-owner", "volume-list-missing",
         "volume-list-bad-list", "volume-list-bad-run")
UNKNOWN = CASES[5:]


def volume(kind):
    from tests.formats.ntfs.support.ntfs_fixture import volume as base
    image, files = base()
    image = bytearray(image)
    offset, rec = record(image, 3)
    base_seq = u16(rec, 16)
    attrs = [a for _, a in attributes(rec) if u32(a, 0) not in (0x60, 0x70)]
    state = bytearray(12)
    state[8:10] = b"\x03\x01"
    dirty = kind not in ("volume-list-clean", "volume-list-base")
    put(state, 10, "H", int(dirty))
    metadata = [resident(0x60, LABEL.encode("utf-16-le"), 10), resident(0x70, state, 11)]
    number, seq = 16, 2
    if kind == "volume-list-base":
        attrs += metadata
        number, seq = 3, base_seq
    else:
        ext_offset, ext = record(image, number)
        ext[:u16(rec, 20)] = rec[:u16(rec, 20)]
        put(ext, 16, "HH", seq, 0)
        put(ext, 22, "H", 1)
        put(ext, 32, "Q", 3 | (base_seq << 48))
        put(ext, 40, "H", 12)
        put(ext, 44, "I", number)
        set_attributes(image, ext_offset, ext, metadata)
        allocated(image, 0, 0xb0, [number])
    if kind in ("volume-list-override", *UNKNOWN):
        # A stale unlisted clean attribute must never substitute for list lookup.
        attrs.append(resident(0x70, bytes(8) + b"\x03\x01\0\0", 5))
    attrs.sort(key=lambda a: u32(a, 0))
    rows = []
    for a in attrs:
        typ = u32(a, 0)
        if typ in (0x60, 0x70):
            continue
        rows.append((typ, 3, base_seq, u16(a, 14)))
    rows += [(0x60, number, seq, 10), (0x70, number, seq, 11)]
    if kind == "volume-list-bad-id":
        rows[-1] = (0x70, number, seq, 99)
    elif kind == "volume-list-bad-sequence":
        rows[-1] = (0x70, number, seq + 1, 11)
    elif kind == "volume-list-wrong-owner":
        rows[-1] = (0x70, 5, u16(record(image, 5)[1], 16), 11)
    elif kind == "volume-list-missing":
        rows.pop()
    rows.sort()
    content = bytearray(b"".join(struct.pack("<IHBBQQH6x", typ, 32, 0, 26, 0,
                                            n | (s << 48), ident)
                                  for typ, n, s, ident in rows))
    if kind == "volume-list-bad-list":
        put(content, 4, "H", 0)
    if kind in ("volume-list-nonresident", "volume-list-bad-run"):
        _, owner = record(image, 64)
        template = next(a for _, a in attributes(owner) if u32(a, 0) == 0x80)
        lcn = 1100 if kind != "volume-list-bad-run" else len(image) // 4096 + 1
        listing = nonresident(template, [(1, lcn)], len(content))
        put(listing, 0, "I", 0x20)
        put(listing, 14, "H", 12)
        if kind != "volume-list-bad-run":
            image[lcn * 4096:lcn * 4096 + len(content)] = content
            allocated(image, 6, 0x80, [lcn])
    else:
        listing = resident(0x20, content, 12)
    attrs.append(listing)
    attrs.sort(key=lambda a: u32(a, 0))
    put(rec, 40, "H", 13)
    set_attributes(image, offset, rec, attrs)
    return bytes(image), files


def limitation(kind):
    if kind in UNKNOWN:
        return "The volume state is unavailable. Files may be inconsistent."
    if kind not in ("volume-list-clean", "volume-list-base"):
        return "The NTFS volume is marked dirty. Some data may be corrupt."
    return ""


def label(kind):
    return "" if kind in ("volume-list-bad-list", "volume-list-bad-run") else LABEL
