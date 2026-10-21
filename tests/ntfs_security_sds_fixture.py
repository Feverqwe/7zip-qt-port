"""Range-local SDS reads with independently validated mapping ownership."""
import struct
from ntfs_fixture import attributes, put, record, u16, u32, u64
from ntfs_ea_fixture import allocated, resident, set_attributes
from ntfs_data_list_fixture import list_bytes, row
from ntfs_security_fixture import (attr_name, descriptor, index_entry, index_root,
                                   named_nonresident, named_resident, read_stream)

CASES = ("security-sds-map-split", "security-sds-map-missing-tail", "security-sds-map-bad-sequence",
         "security-sds-map-wrong-owner", "security-sds-map-unverified-tail",
         "security-sds-map-unverified-primary", "security-sds-map-metadata-flags",
         "security-sds-map-missing-middle", "security-sds-map-sparse",
         "security-sds-map-primary-sequence")


def volume(kind):
    if kind not in CASES:
        raise ValueError(kind)
    from ntfs_security_list_fixture import volume as base
    image, files = base("security-list-selected")
    image = bytearray(image)
    offset, secure = record(image, 9)
    attrs = [a for _, a in attributes(secure) if u32(a, 0) != 0x20 and
             not (u32(a, 0) == 0x90 and attr_name(a) == "$SII")]
    original = next(a for a in attrs if u32(a, 0) == 0x80)
    content = read_stream(image, original)
    # Original controls put 500 in the primary cluster; relocate 700 into a
    # separate segment and publish its matching header in the authoritative root.
    headers = {}
    cursor = 0
    while cursor + 20 <= len(content):
        size = u32(content, cursor + 16)
        if size < 20:
            break
        headers[u32(content, cursor + 4)] = content[cursor:cursor + 20]
        cursor = (cursor + size + 15) & ~15
    target = 8192 if kind == "security-sds-map-missing-middle" else 4096
    header = bytearray(headers[700]);put(header, 8, "Q", target)
    image[(1300 * 4096 + target):(1300 * 4096 + target + 20 + len(descriptor(700)))] = header + descriptor(700)
    size = target + 4096
    primary = named_nonresident(original, 0x80, "$SDS", [(1, 1300)], size, 2)
    put(primary, 40, "Q", size)
    if kind == "security-sds-map-metadata-flags":
        put(primary, 12, "H", 0x4000)
        put(primary, 56, "Q", 0)
    attrs[attrs.index(original)] = primary
    rows = [row(a, 9, u16(secure, 16)) for a in attrs]
    if kind == "security-sds-map-primary-sequence":
        rows = [(typ, name, vcn, number, sequence + (typ == 0x80), ident)
                for typ, name, vcn, number, sequence, ident in rows]
    ext_offset, extension = record(image, 16)
    ext_attrs = [a for _, a in attributes(extension)]
    root = next(a for a in ext_attrs if u32(a, 0) == 0x90 and attr_name(a) == "$SII")
    body = index_root(0x10, [index_entry(struct.pack("<I", 500), headers[500]),
                            index_entry(struct.pack("<I", 700), header), index_entry(b"")])
    ext_attrs[ext_attrs.index(root)] = named_resident(0x90, "$SII", body, u16(root, 14))
    set_attributes(image, ext_offset, extension, ext_attrs)
    rows += [row(a, 16, 2) for a in ext_attrs]
    for vcn in range(1, size // 4096):
        number = 16 + vcn
        off, rec = record(image, number)
        rec[:u16(secure, 20)] = secure[:u16(secure, 20)]
        put(rec, 16, "HH", 2, 0)
        put(rec, 22, "H", 1)
        put(rec, 32, "Q", 9 | (u16(secure, 16) << 48))
        put(rec, 44, "I", number)
        segment = named_nonresident(original, 0x80, "$SDS",
                                    [(1, None if kind == "security-sds-map-sparse" else 1300 + vcn)], 0, 30 + vcn,
                                    flags=0x8000 if kind == "security-sds-map-sparse" else 0)
        put(segment, 16, "QQ", vcn, vcn)
        put(segment, 40, "QQQ", 0, 0, 0)
        if kind == "security-sds-map-wrong-owner":put(rec, 32, "Q", 8 | (u16(secure, 16) << 48))
        set_attributes(image, off, rec, [segment])
        if kind == "security-sds-map-unverified-tail":image[off + 510] ^= 1
        allocated(image, 0, 0xb0, [number])
        allocated(image, 6, 0x80, [1300 + vcn])
        entry = list(row(segment, number, 2))
        if kind == "security-sds-map-bad-sequence":entry[4] += 1
        if kind == "security-sds-map-missing-tail" or (kind == "security-sds-map-missing-middle" and vcn == 1):entry[5] += 50
        rows.append(tuple(entry))
    attrs.append(resident(0x20, list_bytes(rows), 50))
    set_attributes(image, offset, secure, sorted(attrs, key=lambda a: (u32(a, 0), attr_name(a) if a[9] else "")))
    if kind == "security-sds-map-unverified-primary":image[offset + 510] ^= 1
    off, alpha = record(image, 64)
    attrs = [a for _, a in attributes(alpha)]
    standard = next(a for a in attrs if u32(a, 0) == 0x10)
    data = bytearray(read_stream(image, standard));data += bytes(max(0, 72 - len(data)))
    put(data, 52, "I", 500)
    attrs[attrs.index(standard)] = resident(0x10, data, u16(standard, 14))
    set_attributes(image, off, alpha, attrs)
    return bytes(image), files


def expected(kind):
    return descriptor(700) if kind in ("security-sds-map-split", "security-sds-map-metadata-flags",
                                      "security-sds-map-missing-middle") else b""
