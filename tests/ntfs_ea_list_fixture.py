"""EA list authority, raw metadata transport and readable partial openings."""
import struct
from ntfs_fixture import attributes, nonresident, put, record, u16, u32, u64
from ntfs_ea_fixture import allocated, entry, resident, set_attributes

VALID = ("ea-list-selected", "ea-list-nonresident", "ea-list-split",
         "ea-list-metadata-flags", "ea-list-unlisted")
LIMITED = ("ea-list-bad-id", "ea-list-bad-sequence", "ea-list-missing-tail", "ea-list-bad-run")
CASES = VALID + LIMITED
LIMITATION = ("Some NTFS extended attributes are unavailable. WSL ownership and file "
              "types may be unknown. Readable DATA streams remain accessible.")


def volume(kind):
    from ntfs_ea_fixture import volume as base
    if kind not in CASES:
        raise ValueError(kind)
    split = kind in ("ea-list-split", "ea-list-missing-tail")
    source, files = base("ea-fragmented" if split else "ea-resident")
    image = bytearray(source)
    offset, rec = record(image, 66)
    original = [a for _, a in attributes(rec)]
    info = next(a for a in original if u32(a, 0) == 0xd0)
    ea = next(a for a in original if u32(a, 0) == 0xe0)
    attrs = [a for a in original if u32(a, 0) not in (0xd0, 0xe0)]
    stale = b"".join(entry(n, struct.pack("<I", v)) for n, v in
                     (("$LXUID", 999), ("$LXGID", 998), ("$LXMOD", 0o40777)))
    attrs += [resident(0xd0, struct.pack("<HHI", len(stale) - 12, 0, len(stale)), 96),
              resident(0xe0, stale, 97)]
    selected = [(0xd0, info, 16, 2)]
    ext_attrs = [info]
    if split:
        size = u64(ea, 48)
        primary = nonresident(ea, [(1, 1100)], size)
        put(primary, 40, "Q", 12288)
        tail = nonresident(ea, [(1, 1102), (1, 1104)], 0)
        put(tail, 16, "QQ", 1, 2)
        put(tail, 40, "QQQ", 0, 0, 0)
        put(tail, 14, "H", 14)
        attrs.append(primary)
        ext_attrs.append(tail)
        selected += [(0xe0, primary, 66, u16(rec, 16)), (0xe0, tail, 16, 2)]
    else:
        if kind in ("ea-list-metadata-flags", "ea-list-bad-run"):
            payload = ea[u16(ea, 20):u16(ea, 20) + u32(ea, 16)]
            template = next(a for a in attrs if u32(a, 0) == 0x80)
            lcn = len(image) // 4096 + 1 if kind == "ea-list-bad-run" else 1100
            ea = nonresident(template, [(1, lcn)], len(payload), flags=0x4000)
            put(ea, 0, "I", 0xe0)
            put(ea, 14, "H", 13)
            put(ea, 56, "Q", 0)
            if kind != "ea-list-bad-run":
                image[lcn * 4096:lcn * 4096 + len(payload)] = payload
                allocated(image, 6, 0x80, [lcn])
        ext_attrs.append(ea)
        selected.append((0xe0, ea, 16, 2))
    ext_offset, ext = record(image, 16)
    ext[:u16(rec, 20)] = rec[:u16(rec, 20)]
    put(ext, 16, "HH", 2, 0)
    put(ext, 22, "H", 1)
    put(ext, 32, "Q", 66 | (u16(rec, 16) << 48))
    put(ext, 40, "H", 100)
    put(ext, 44, "I", 16)
    set_attributes(image, ext_offset, ext, ext_attrs)
    allocated(image, 0, 0xb0, [16])
    rows = []
    for a in attrs:
        if u32(a, 0) in (0xd0, 0xe0):
            continue
        name = a[u16(a, 10):u16(a, 10) + a[9] * 2] if a[9] else b""
        rows.append((u32(a, 0), name, 0, 66, u16(rec, 16), u16(a, 14)))
    for typ, a, number, seq in selected:
        if kind == "ea-list-unlisted" or (kind == "ea-list-missing-tail" and typ == 0xe0 and number == 16):
            continue
        ident = 95 if kind == "ea-list-bad-id" and typ == 0xe0 else u16(a, 14)
        if kind == "ea-list-bad-sequence" and typ == 0xe0:
            seq += 1
        rows.append((typ, b"", u64(a, 16) if a[8] else 0, number, seq, ident))
    content = bytearray()
    for typ, name, vcn, number, seq, ident in sorted(rows):
        size = (26 + len(name) + 7) & ~7
        row = bytearray(size)
        struct.pack_into("<IHBBQQH", row, 0, typ, size, len(name) // 2, 26, vcn,
                         number | (seq << 48), ident)
        row[26:26 + len(name)] = name
        content += row
    if kind == "ea-list-nonresident":
        template = next(a for a in attrs if u32(a, 0) == 0x80)
        listing = nonresident(template, [(1, 1106)], len(content))
        put(listing, 0, "I", 0x20)
        put(listing, 14, "H", 98)
        image[1106 * 4096:1106 * 4096 + len(content)] = content
        allocated(image, 6, 0x80, [1106])
    else:
        listing = resident(0x20, content, 98)
    attrs.append(listing)
    attrs.sort(key=lambda a: u32(a, 0))
    put(rec, 40, "H", 100)
    set_attributes(image, offset, rec, attrs)
    return bytes(image), files
