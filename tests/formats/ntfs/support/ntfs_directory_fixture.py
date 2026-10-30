"""NTFS directory index storage, namespace and bounded recovery controls."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import struct
from tests.formats.ntfs.support.ntfs_fixture import attributes, put, record, u16, u32, u64
from tests.formats.ntfs.support.ntfs_ea_fixture import allocated, resident, set_attributes
from tests.formats.ntfs.support.ntfs_data_list_fixture import list_bytes, row
from tests.formats.ntfs.support.ntfs_file_metadata_fixture import filename, index_entry, make_record
from tests.formats.ntfs.support.ntfs_security_fixture import (index_buffer, index_root, named_nonresident,
                                   named_resident, read_stream)

VALID = ("dir-resident", "dir-allocation", "dir-bitmap-nonresident", "dir-fragmented",
         "dir-extension", "dir-small-block", "dir-case-links")
LIMITED = ("dir-index-only", "dir-unindexed-name", "dir-root-missing", "dir-root-invalid",
           "dir-node-vbn", "dir-node-fixup", "dir-node-name-key", "dir-bitmap-outside",
           "dir-bad-list-ref", "dir-bitmap-initialized", "dir-child-outside", "dir-bitmap-late-outside",
           "dir-extension-sequence", "dir-extension-owner", "dir-list-malformed",
           "dir-path-escape", "dir-dangling-record", "dir-no-usable-name", "dir-duplicate-fn")
CASES = VALID + LIMITED
TERMINAL = struct.pack("<QHHHH", 0, 16, 0, 2, 0)


def volume(kind):
    from tests.formats.ntfs.support.ntfs_fixture import volume as base
    if kind not in CASES:
        raise ValueError(kind)
    source, files = base()
    image = bytearray(source)
    offset, rec = record(image, 5)
    attrs = [a for _, a in attributes(rec) if u32(a, 0) not in (0x90, 0xa0, 0xb0)]
    old = [a for _, a in attributes(rec)]
    template = next(a for a in old if u32(a, 0) == 0xa0)
    parent = 5 | (u16(rec, 16) << 48)
    names = []
    for number in (64, 65, 66):
        _, child = record(image, number)
        raw = next(a for _, a in attributes(child) if u32(a, 0) == 0x30)
        names.append((number | (u16(child, 16) << 48), read_stream(image, raw)))
    gamma_ref, gamma_name = names[-1]
    if kind in ("dir-index-only", "dir-extension-sequence", "dir-extension-owner", "dir-list-malformed"):
        new_name = "index 空"
        names[-1] = (gamma_ref, filename(gamma_name, new_name, parent))
        files[new_name] = files["gamma"]
    elif kind == "dir-unindexed-name":
        names.pop()
    elif kind == "dir-path-escape":
        names[-1] = (gamma_ref, filename(gamma_name, "../escape", parent))
    elif kind == "dir-dangling-record":
        names[-1] = (9999 | (1 << 48), filename(gamma_name, "unreadable", parent))
    elif kind == "dir-no-usable-name":
        child_offset, child = record(image, 66)
        set_attributes(image, child_offset, child,
                       [a for _, a in attributes(child) if u32(a, 0) != 0x30])
        files["[LOST]/[NONAME]-66"] = files.pop("gamma")
    elif kind == "dir-duplicate-fn":
        child_offset, child = record(image, 66)
        child_attrs = [a for _, a in attributes(child)]
        child_attrs.append(resident(0x30, gamma_name, 50))
        put(child, 18, "H", 2)
        set_attributes(image, child_offset, child, sorted(child_attrs, key=lambda a: u32(a, 0)))
    elif kind == "dir-case-links":
        alias = filename(gamma_name, "GAMMA", parent, kind=0)
        child_offset, child = record(image, 66)
        child_attrs = [a for _, a in attributes(child)]
        child_attrs.append(resident(0x30, alias, 50))
        put(child, 18, "H", 2)
        set_attributes(image, child_offset, child, sorted(child_attrs, key=lambda a: u32(a, 0)))
        names.append((gamma_ref, alias))
        files["GAMMA"] = files["gamma"]
    names.sort(key=lambda r: (r[1][66:66 + r[1][64] * 2].decode("utf-16-le").upper(), r[1][66:]))
    entries = [index_entry(ref, name) for ref, name in names]
    if kind == "dir-resident":
        root = bytearray(index_root(1, [*entries, TERMINAL]))
        put(root, 0, "I", 0x30)
        attrs.append(named_resident(0x90, "$I30", root, 40))
    else:
        block = 1024 if kind == "dir-small-block" else 4096
        fragmented = kind == "dir-fragmented"
        body = (index_buffer([entries[0], TERMINAL], 0, block) +
                index_buffer([*entries[2:], TERMINAL], 1, block)) if fragmented else \
            index_buffer([*entries, TERMINAL], 0, block)
        # Child pointers are deliberately not followed by Linux's unsorted reader.
        child = 0
        root = bytearray(index_root(1, [struct.pack("<QHHHHQ", 0, 24, 0, 3, 0, child)],
                                    block=block, children=True))
        put(root, 0, "I", 0x30)
        if fragmented:
            median = bytearray(entries[1]) + bytes(8)
            put(median, 8, "H", len(median))
            put(median, 12, "H", 1)
            root = bytearray(index_root(1, [median, struct.pack("<QHHHHQ", 0, 24, 0, 3, 0, 1)],
                                        block=block, children=True))
            put(root, 0, "I", 0x30)
        if kind == "dir-child-outside":
            root = bytearray(index_root(1, [struct.pack("<QHHHHQ", 0, 24, 0, 3, 0, 999999)],
                                        block=block, children=True))
            put(root, 0, "I", 0x30)
        body = bytearray(body)
        if kind == "dir-node-vbn":
            put(body, 16, "Q", 7)
        elif kind == "dir-node-fixup":
            body[510] ^= 1
        elif kind == "dir-node-name-key":
            # First key claims one UTF-16 character, then extends into padding.
            put(body, 64 + 10, "H", 68)
            body[64 + 16 + 64] = 255
        if fragmented:
            image[1100 * 4096:1101 * 4096] = body[:4096]
            image[1102 * 4096:1103 * 4096] = body[4096:]
            runs = [(1, 1100), (1, 1102)]
            allocated(image, 6, 0x80, [1100, 1102])
        else:
            image[1100 * 4096:1100 * 4096 + len(body)] = body
            runs = [(1, 1100)]
            allocated(image, 6, 0x80, [1100])
        allocation = named_nonresident(template, 0xa0, "$I30", runs, len(body), 41)
        if block < 4096:
            # Nonresident allocation is cluster aligned, while its logical index is small.
            put(allocation, 40, "Q", 4096)
        bitmap_data = b"\x03" if fragmented else b"\x02" if kind == "dir-bitmap-outside" else b"\x01"
        if kind in ("dir-bitmap-nonresident", "dir-bitmap-initialized", "dir-bitmap-late-outside"):
            if kind == "dir-bitmap-late-outside":
                bitmap_data = bytes(4096) + b"\x01" + bytes(4095)
            image[1200 * 4096:1200 * 4096 + len(bitmap_data)] = bitmap_data
            bitmap = named_nonresident(template, 0xb0, "$I30",
                                      [((len(bitmap_data) + 4095) // 4096, 1200)], len(bitmap_data), 42)
            if kind == "dir-bitmap-initialized":
                put(bitmap, 56, "Q", 0)
            allocated(image, 6, 0x80, [1200, 1201] if kind == "dir-bitmap-late-outside" else [1200])
        else:
            bitmap = named_resident(0xb0, "$I30", bitmap_data, 42)
        attrs += [named_resident(0x90, "$I30", root, 40), allocation, bitmap]
        if kind in ("dir-extension", "dir-bad-list-ref", "dir-extension-sequence", "dir-extension-owner"):
            moved = [attrs.pop(), attrs.pop(), attrs.pop()]
            _, child = record(image, 66)
            rows = [row(a, 5, u16(rec, 16)) for a in attrs]
            rows += [row(a, 16, 2) for a in moved]
            if kind == "dir-bad-list-ref":
                rows = [r[:-1] + (99,) if r[0] == 0x90 else r for r in rows]
            attrs.append(resident(0x20, list_bytes(rows), 43))
            make_record(image, child, 16, moved, owner=parent ^ (1 << 48) if kind == "dir-extension-owner" else parent,
                        sequence=3 if kind == "dir-extension-sequence" else 2)
            allocated(image, 0, 0xb0, [16])
        if kind == "dir-list-malformed":
            attrs.append(resident(0x20, bytes(32), 43))
    if kind in ("dir-root-missing", "dir-no-usable-name"):
        attrs = [a for a in attrs if u32(a, 0) != 0x90]
    elif kind == "dir-root-invalid":
        root_attr = next(a for a in attrs if u32(a, 0) == 0x90)
        put(root_attr, u16(root_attr, 20) + 16 + 8, "I", 0xffffffff)
    set_attributes(image, offset, rec, sorted(attrs, key=lambda a: u32(a, 0)))
    return bytes(image), files
