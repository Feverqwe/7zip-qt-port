"""Authoritative list selection and partial-read failures for NTFS reparse data."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import struct
from tests.formats.ntfs.support.ntfs_fixture import attributes, nonresident, put, record, u16, u32, u64
from tests.formats.ntfs.support.ntfs_ea_fixture import allocated, resident, set_attributes
from tests.formats.ntfs.support.ntfs_security_fixture import named_resident

VALID = ("reparse-list-selected", "reparse-list-nonresident", "reparse-list-split",
         "reparse-list-unlisted", "reparse-list-wof", "reparse-list-metadata-flags")
RAW = ("reparse-list-bad-id", "reparse-list-bad-sequence", "reparse-list-bad-run", "reparse-list-directory")
UNREADABLE = ("reparse-list-too-large", "reparse-list-missing-tail", "reparse-list-sparse-tail")
BAD = RAW + UNREADABLE
CASES = VALID + BAD
LIMITATION = "Some NTFS reparse metadata is unavailable. Unclassified entries expose underlying DATA streams where readable."


def volume(kind):
    if kind not in CASES:
        raise ValueError(kind)
    from tests.formats.ntfs.support.ntfs_fixture import volume as base
    split = kind in ("reparse-list-split", "reparse-list-missing-tail")
    sparse = kind == "reparse-list-sparse-tail"
    image, files = base("wof-xpress4k" if kind == "reparse-list-wof" else
                        "link-fragmented" if split or sparse else "link-symlink")
    image = bytearray(image)
    offset, rec = record(image, 66)
    sequence = u16(rec, 16)
    original = [a for _, a in attributes(rec)]
    reparse = next(a for a in original if u32(a, 0) == 0xc0)
    if sparse:
        reparse = nonresident(reparse, [(1, 1400), (1, None)], u64(reparse, 48), flags=0x8000)
        put(reparse, 0, "I", 0xc0)
        put(reparse, 14, "H", 5)
    attrs = [a for a in original if u32(a, 0) != 0xc0]
    # Stale unlisted base metadata must not override the list-selected payload.
    stale = struct.pack("<IHHHHHHI", 0xa000000c, 22, 0, 0, 0, 0, 10, 1) + "wrong".encode("utf-16-le")
    attrs.append(resident(0xc0, stale, 99))
    ext_attrs = []
    if split:
        size = u64(reparse, 48)
        primary = nonresident(reparse, [(1, 1400)], size)
        put(primary, 40, "Q", 8192)
        tail = nonresident(reparse, [(1, 1402)], 0)
        put(tail, 16, "QQ", 1, 1)
        put(tail, 40, "QQQ", 0, 0, 0)
        put(tail, 14, "H", 6)
        attrs.append(primary)
        ext_attrs.append(tail)
        selected = [(primary, 66, sequence), (tail, 16, 2)]
    else:
        if kind in ("reparse-list-bad-run", "reparse-list-too-large", "reparse-list-metadata-flags"):
            payload = reparse[u16(reparse, 20):u16(reparse, 20) + u32(reparse, 16)]
            if kind == "reparse-list-too-large":
                payload += bytes(16385 - len(payload))
            template = next(a for a in attrs if u32(a, 0) == 0x80 and a[8])
            count = (len(payload) + 4095) // 4096
            lcn = len(image) // 4096 + 1 if kind == "reparse-list-bad-run" else 1400
            reparse = nonresident(template, [(count, lcn)], len(payload),
                                 flags=0x4000 if kind == "reparse-list-metadata-flags" else 0)
            if kind == "reparse-list-metadata-flags":
                put(reparse, 56, "Q", 0)
            put(reparse, 0, "I", 0xc0)
            put(reparse, 14, "H", 5)
            if kind != "reparse-list-bad-run":
                image[lcn * 4096:lcn * 4096 + len(payload)] = payload
                allocated(image, 6, 0x80, list(range(lcn, lcn + count)))
        ext_attrs.append(reparse)
        selected = [(reparse, 16, 2)]
    ext_offset, ext = record(image, 16)
    ext[:u16(rec, 20)] = rec[:u16(rec, 20)]
    put(ext, 16, "HH", 2, 0)
    put(ext, 22, "H", 1)
    put(ext, 32, "Q", 66 | (sequence << 48))
    put(ext, 40, "H", 100)
    put(ext, 44, "I", 16)
    set_attributes(image, ext_offset, ext, ext_attrs)
    allocated(image, 0, 0xb0, [16])
    attrs.append(named_resident(0x80, "side", b"independent ADS", 98))
    files["gamma:side"] = b"independent ADS"
    rows = []
    for a in attrs:
        if u32(a, 0) == 0xc0:
            continue
        # Named DATA is not relevant to reparse lookup; keep a valid named row.
        name = a[u16(a, 10):u16(a, 10) + a[9] * 2] if a[9] else b""
        rows.append((u32(a, 0), name, 0, 66, sequence, u16(a, 14)))
    for a, number, seq in selected:
        if kind == "reparse-list-unlisted" or (kind == "reparse-list-missing-tail" and number == 16):
            continue
        ident = 97 if kind in ("reparse-list-bad-id", "reparse-list-directory") else u16(a, 14)
        if kind == "reparse-list-bad-sequence":seq += 1
        rows.append((0xc0, b"", u64(a, 16) if a[8] else 0, number, seq, ident))
    content = bytearray()
    for typ, name, vcn, number, seq, ident in sorted(rows):
        size = (26 + len(name) + 7) & ~7
        row = bytearray(size)
        struct.pack_into("<IHBBQQH", row, 0, typ, size, len(name) // 2, 26, vcn,
                         number | (seq << 48), ident)
        row[26:26 + len(name)] = name
        content += row
    if kind == "reparse-list-nonresident":
        template = next(a for a in attrs if u32(a, 0) == 0x80 and a[8])
        listing = nonresident(template, [(1, 1100)], len(content))
        put(listing, 0, "I", 0x20)
        put(listing, 14, "H", 7)
        image[1100 * 4096:1100 * 4096 + len(content)] = content
        allocated(image, 6, 0x80, [1100])
    else:
        listing = resident(0x20, content, 7)
    attrs.append(listing)
    attrs.sort(key=lambda a: u32(a, 0))
    if kind == "reparse-list-directory":
        put(rec, 22, "H", u16(rec, 22) | 2)
        si = next(a for a in attrs if u32(a, 0) == 0x10)
        put(si, u16(si, 20) + 32, "I", u32(si, u16(si, 20) + 32) | 0x10)
    put(rec, 40, "H", 100)
    set_attributes(image, offset, rec, attrs)
    if kind in UNREADABLE:
        files.pop("gamma", None)
    elif kind == "reparse-list-unlisted" or kind in RAW:
        files["gamma"] = b"B" * 65536
    return bytes(image), files
