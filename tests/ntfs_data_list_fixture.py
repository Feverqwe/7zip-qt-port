"""DATA membership, named streams and incomplete mappings over creator images."""
import struct
from ntfs_fixture import attributes, nonresident, put, record, u16, u32, u64
from ntfs_ea_fixture import allocated, resident, set_attributes
from ntfs_security_fixture import attr_name, named_resident

VALID = ("data-list-selected", "data-list-nonresident", "data-list-split")
LIMITED = ("data-list-bad-id", "data-list-ads-bad-id", "data-list-bad-sequence",
           "data-list-missing-middle", "data-list-sparse-middle", "data-list-initialized-tail",
           "data-list-compressed-middle", "data-list-unreadable-list", "data-list-malformed-list",
           "data-list-bitmap", "data-list-security", "data-list-nul-name", "data-list-nul-attribute",
           "data-list-missing-first-run", "data-list-unlisted", "data-list-wrong-owner",
           "data-list-wrong-owner-sequence", "data-list-no-list-tail")
CASES = VALID + LIMITED
PARTIAL = ("data-list-missing-middle", "data-list-sparse-middle", "data-list-compressed-middle",
           "data-list-missing-first-run", "data-list-no-list-tail")
UNAVAILABLE = ("data-list-bad-id", "data-list-unlisted")
LIMITATION = ("Some NTFS DATA mappings are incomplete or unverified. Files or streams "
              "may be outdated, incomplete or unreadable.")
SELECTED = b"selected DATA payload"


def list_bytes(rows):
    result = bytearray()
    for typ, name, vcn, number, sequence, ident in sorted(rows, key=lambda r: (r[0], r[1].upper(), r[1], r[2])):
        name = name.encode("utf-16-le")
        size = (26 + len(name) + 7) & ~7
        row = bytearray(size)
        struct.pack_into("<IHBBQQH", row, 0, typ, size, len(name) // 2, 26, vcn,
                         number | (sequence << 48), ident)
        row[26:26 + len(name)] = name
        result += row
    return result


def row(a, number, sequence):
    return (u32(a, 0), attr_name(a) if a[9] else "", u64(a, 16) if a[8] else 0,
            number, sequence, u16(a, 14))


def extension(image, source, attrs, number, sequence):
    offset, rec = record(image, number)
    rec[:u16(source, 20)] = source[:u16(source, 20)]
    put(rec, 16, "HH", sequence, 0)
    put(rec, 22, "H", 1)
    put(rec, 32, "Q", 66 | (u16(source, 16) << 48))
    put(rec, 40, "H", 100)
    put(rec, 44, "I", number)
    set_attributes(image, offset, rec, attrs)
    allocated(image, 0, 0xb0, [number])


def volume(kind):
    from ntfs_fixture import volume as base
    if kind not in CASES:
        raise ValueError(kind)
    source, files = base("security-index" if kind == "data-list-security" else "base")
    image = bytearray(source)
    if kind in ("data-list-bitmap", "data-list-security"):
        number = 6 if kind == "data-list-bitmap" else 9
        offset, rec = record(image, number)
        attrs = [a for _, a in attributes(rec)]
        rows = [row(a, number, u16(rec, 16)) for a in attrs]
        rows = [(*r[:-1], 95) if r[0] == 0x80 else r for r in rows]
        attrs.append(resident(0x20, list_bytes(rows), 98))
        attrs.sort(key=lambda a: u32(a, 0))
        set_attributes(image, offset, rec, attrs)
        return bytes(image), files
    offset, rec = record(image, 66)
    sequence = u16(rec, 16)
    original = [a for _, a in attributes(rec)]
    template = next(a for a in original if u32(a, 0) == 0x80 and not a[9])
    attrs = [a for a in original if u32(a, 0) != 0x80]
    attrs += [template, named_resident(0x80, "side", b"stale ADS", 94)]
    local = named_resident(0x80, "local", b"independent local ADS", 93)
    attrs.append(local)
    primary = resident(0x80, SELECTED, 15)
    side = named_resident(0x80, "side", b"selected ADS", 16)
    if kind == "data-list-nul-attribute":
        side = named_resident(0x80, "side\0foreign", b"unaddressable ADS", 16)
    unicode = named_resident(0x80, "поток 空", b"Unicode ADS", 17)
    case = named_resident(0x80, "Side", b"case distinct ADS", 18)
    ext_attrs = [primary, case, side, unicode]
    files.update({"gamma": SELECTED, "gamma:local": b"independent local ADS",
                  "gamma:side": b"selected ADS", "gamma:поток 空": b"Unicode ADS",
                  "gamma:Side": b"case distinct ADS"})
    selected = [row(a, 16, 2) for a in ext_attrs]
    if kind in ("data-list-split", *PARTIAL, "data-list-initialized-tail"):
        sparse = kind == "data-list-sparse-middle"
        compressed = kind == "data-list-compressed-middle"
        size = 65536 if compressed else 12288
        first_run = [(1, None)] if sparse else [(1, 1300)]
        primary = nonresident(template, first_run, size,
                              valid=4096 if kind == "data-list-initialized-tail" else size,
                              flags=1 if compressed else 0x8000 if sparse else 0,
                              unit=4 if compressed else 0)
        if kind == "data-list-missing-first-run":
            primary = nonresident(template, [(1, len(image) // 4096 + 1)], size)
        put(primary, 14, "H", 15)
        put(primary, 40, "Q", size)
        if sparse or compressed:
            put(primary, 64, "Q", 8192 if compressed else 4096)
        middle = nonresident(template, [(1, 1302)], 0)
        put(middle, 14, "H", 19)
        put(middle, 16, "QQ", 1, 1)
        put(middle, 40, "QQQ", 0, 0, 0)
        last = nonresident(template, [(1, 1304)], 0)
        put(last, 14, "H", 20)
        put(last, 16, "QQ", 2, 2)
        put(last, 40, "QQQ", 0, 0, 0)
        attrs.append(primary)
        ext_attrs = [case, side, unicode, last]
        selected = [row(primary, 66, sequence), row(middle, 17, 3),
                    *[row(a, 16, 2) for a in ext_attrs]]
        if kind not in ("data-list-split", "data-list-missing-first-run"):
            selected[1] = (*selected[1][:-1], 95)
        extension(image, rec, [middle], 17, 3)
        for lcn, char in ((1300, b"A"), (1302, b"B"), (1304, b"C")):
            image[lcn * 4096:(lcn + 1) * 4096] = char * 4096
        allocated(image, 6, 0x80, [1300, 1302, 1304])
        files["gamma"] = b"A" * 4096 + b"B" * 4096 + b"C" * 4096
        if kind == "data-list-initialized-tail":
            files["gamma"] = b"A" * 4096 + bytes(8192)
        if kind in PARTIAL:
            files.pop("gamma")
    extension(image, rec, ext_attrs, 16, 2)
    if kind in ("data-list-wrong-owner", "data-list-wrong-owner-sequence"):
        ext_offset, ext = record(image, 16)
        put(ext, 32, "Q", (65 if kind == "data-list-wrong-owner" else 66) |
            ((sequence + (kind == "data-list-wrong-owner-sequence")) << 48))
        set_attributes(image, ext_offset, ext, ext_attrs)
    rows = [row(a, 66, sequence) for a in attrs if u32(a, 0) != 0x80 or a is local]
    for r in selected:
        if kind == "data-list-nul-name" and r[1] == "side":
            r = (r[0], "side\0foreign", *r[2:])
        if kind == "data-list-nul-attribute" and r[1] == "side\0foreign":
            r = (r[0], "side", *r[2:])
        if kind == "data-list-unlisted" and not r[1]:
            continue
        if kind in ("data-list-bad-id", "data-list-bad-sequence") and not r[1]:
            r = (*r[:-1], 95) if kind == "data-list-bad-id" else (*r[:4], 3, r[5])
        if kind == "data-list-ads-bad-id" and r[1] == "side":
            r = (*r[:-1], 95)
        rows.append(r)
    listing = list_bytes(rows)
    if kind == "data-list-malformed-list":
        put(listing, 4, "H", 1)
    if kind in ("data-list-nonresident", "data-list-unreadable-list"):
        lcn = 1100 if kind == "data-list-nonresident" else len(image) // 4096 + 1
        listing_attr = nonresident(template, [(1, lcn)], len(listing))
        put(listing_attr, 0, "I", 0x20)
        put(listing_attr, 14, "H", 98)
        if kind == "data-list-nonresident":
            image[lcn * 4096:lcn * 4096 + len(listing)] = listing
            allocated(image, 6, 0x80, [lcn])
    else:
        listing_attr = resident(0x20, listing, 98)
    if kind != "data-list-no-list-tail":
        attrs.append(listing_attr)
    else:
        attrs = [a for a in attrs if a is not template and u16(a, 14) != 94]
        for name in ("gamma:side", "gamma:Side", "gamma:поток 空"):
            files.pop(name)
    attrs.sort(key=lambda a: u32(a, 0))
    put(rec, 40, "H", 100)
    set_attributes(image, offset, rec, attrs)
    if kind in UNAVAILABLE:
        files.pop("gamma")
    elif kind in ("data-list-unreadable-list", "data-list-malformed-list"):
        files["gamma"] = b"B" * 65536
        files["gamma:side"] = b"stale ADS"
        files.pop("gamma:Side")
        files.pop("gamma:поток 空")
    if kind in ("data-list-ads-bad-id", "data-list-nul-name", "data-list-nul-attribute"):
        files.pop("gamma:side")
    return bytes(image), files
