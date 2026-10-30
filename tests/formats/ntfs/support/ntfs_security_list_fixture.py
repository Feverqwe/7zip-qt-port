"""Authoritative named SII roots and allocation extents."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import struct
from tests.formats.ntfs.support.ntfs_fixture import attributes, put, record, u16, u32, u64
from tests.formats.ntfs.support.ntfs_ea_fixture import allocated, resident, set_attributes
from tests.formats.ntfs.support.ntfs_data_list_fixture import list_bytes, row
from tests.formats.ntfs.support.ntfs_security_fixture import (attr_name, descriptor, index_entry, index_root,
                                   named_nonresident, named_resident, read_stream)

VALID = ("security-list-selected", "security-list-nonresident", "security-list-split",
         "security-list-metadata-flags")
LIMITED = ("security-list-bad-id", "security-list-bad-sequence", "security-list-wrong-owner",
           "security-list-owner-sequence", "security-list-missing-root", "security-list-missing-tail",
           "security-list-no-list", "security-list-malformed", "security-list-unreadable",
           "security-list-unverified-record", "security-list-root-vcn", "security-list-duplicate")
CASES = VALID + LIMITED


def volume(kind):
    from tests.formats.ntfs.support.ntfs_fixture import volume as base
    if kind not in CASES:
        raise ValueError(kind)
    source, files = base("security-tree")
    image = bytearray(source)
    base_offset, secure = record(image, 9)
    old = [a for _, a in attributes(secure)]
    sds = next(a for a in old if u32(a, 0) == 0x80 and attr_name(a) == "$SDS")
    selected = [a for a in old if a[9] and (attr_name(a) == "$SII" or
                 (u32(a, 0) == 0x90 and attr_name(a) == "$SDH"))]
    attrs = [a for a in old if a not in selected]
    primary_alloc = next(a for a in selected if u32(a, 0) == 0xa0)
    tail = []
    if kind in ("security-list-split", "security-list-missing-tail"):
        first = named_nonresident(primary_alloc, 0xa0, "$SII", [(1, 1200)], 8192, 11)
        put(first, 40, "Q", 8192)
        last = named_nonresident(primary_alloc, 0xa0, "$SII", [(1, 1202)], 0, 21)
        put(last, 16, "QQ", 1, 1)
        put(last, 40, "QQQ", 0, 0, 0)
        selected[selected.index(primary_alloc)] = first
        tail = [last]
    if kind == "security-list-metadata-flags":
        alloc = next(a for a in selected if u32(a, 0) == 0xa0)
        put(alloc, 12, "H", 0x4000)
        put(alloc, 56, "Q", 0)

    # A self-consistent obsolete root refers to a different descriptor with the
    # same ID in unused SDS padding. Only the list-selected root is authoritative.
    stale = descriptor(999)
    checksum = 0
    for i in range(0, len(stale), 4):
        checksum = (((checksum << 3) | (checksum >> 29)) + int.from_bytes(stale[i:i + 4], "little")) & 0xffffffff
    header = struct.pack("<IIQI", checksum, 700, 0x20000, 20 + len(stale))
    image[1300 * 4096 + 0x20000:1300 * 4096 + 0x20000 + len(header) + len(stale)] = header + stale
    stale_root = named_resident(0x90, "$SII", index_root(0x10,
                                 [index_entry(struct.pack("<I", 700), header), index_entry(b"")]), 99)
    if kind != "security-list-no-list":
        attrs.append(stale_root)

    rows = [row(a, 9, u16(secure, 16)) for a in attrs if a is not stale_root]
    for number, group in ((16, selected), (17, tail)):
        if not group:
            continue
        off, rec = record(image, number)
        rec[:u16(secure, 20)] = secure[:u16(secure, 20)]
        put(rec, 16, "HH", 2, 0)
        put(rec, 22, "H", 1)
        put(rec, 32, "Q", 9 | (u16(secure, 16) << 48))
        put(rec, 44, "I", number)
        if kind == "security-list-wrong-owner":put(rec, 32, "Q", 8 | (u16(secure, 16) << 48))
        if kind == "security-list-owner-sequence":put(rec, 32, "Q", 9 | ((u16(secure, 16) + 1) << 48))
        set_attributes(image, off, rec, sorted(group, key=lambda a: u32(a, 0)))
        if kind == "security-list-unverified-record":image[off + 510] ^= 1
        allocated(image, 0, 0xb0, [number])
        rows += [row(a, number, 2) for a in group]
    changed = []
    for r in rows:
        typ, name, vcn, number, seq, ident = r
        if typ == 0x90 and name == "$SII":
            if kind == "security-list-bad-id":ident += 50
            if kind == "security-list-bad-sequence":seq += 1
            if kind == "security-list-root-vcn":vcn = 1
            if kind == "security-list-missing-root":continue
            if kind == "security-list-duplicate":changed.append(r)
        if typ == 0xa0 and name == "$SII" and vcn and kind == "security-list-missing-tail":
            ident += 50
        changed.append((typ, name, vcn, number, seq, ident))
    content = list_bytes(changed)
    if kind == "security-list-malformed":content[4:6] = bytes(2)
    if kind != "security-list-no-list":
        if kind in ("security-list-nonresident", "security-list-unreadable"):
            lcn = 1100 if kind.endswith("nonresident") else len(image) // 4096 + 1
            listing = named_nonresident(sds, 0x20, "", [(1, lcn)], len(content), 50)
            if lcn == 1100:
                image[lcn * 4096:lcn * 4096 + len(content)] = content
                allocated(image, 6, 0x80, [lcn])
        else:
            listing = resident(0x20, content, 50)
        attrs.append(listing)
    set_attributes(image, base_offset, secure, sorted(attrs, key=lambda a: (u32(a, 0), attr_name(a) if a[9] else "")))
    if kind == "security-list-missing-tail":
        offset, alpha = record(image, 64)
        attrs = [a for _, a in attributes(alpha)]
        standard = next(a for a in attrs if u32(a, 0) == 0x10)
        data = bytearray(read_stream(image, standard));data += bytes(max(0, 72 - len(data)))
        put(data, 52, "I", 500)
        attrs[attrs.index(standard)] = resident(0x10, data, u16(standard, 14))
        set_attributes(image, offset, alpha, attrs)
    return bytes(image), files


def expected(kind):
    return descriptor(700) if kind in VALID else b""
