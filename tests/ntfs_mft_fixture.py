"""MFT bootstrap with DATA extents in an attribute-list extension record."""
import struct


def volume(kind):
    from ntfs_fixture import volume as base_volume, record, save_record, attributes
    from ntfs_fixture import u16, u32, u64, put, runs, nonresident
    original, files = base_volume()
    image = bytearray(original)
    mft = u64(image, 0x30) * 4096
    offset, rec = record(image, 0)
    attrs = [a for _, a in attributes(rec)]
    data = next(a for a in attrs if u32(a, 0) == 0x80)
    if kind in ("mft-list-named-only", "mft-list-nul-name", "mft-list-named-decoy", "mft-list-nul-decoy"):
        from ntfs_security_fixture import named_nonresident
        from ntfs_ea_fixture import set_attributes
        name = "\0" if "nul" in kind else "unrelated"
        named = named_nonresident(data, 0x80, name,
                                  [(u64(data, 40) // 4096, u64(image, 0x30))],
                                  u64(data, 48), u16(data, 14) + 1)
        # Keep bytes readable to detect choosing ADS solely by its sorted position.
        if kind.endswith("decoy"):
            attrs.insert(attrs.index(data) + 1, named)
        else:
            attrs[attrs.index(data)] = named
        set_attributes(image, offset, rec, attrs)
        return bytes(image), files
    base_seq = u16(rec, 16)
    split, target, extension_number = 8, 1000, 16
    extension = bytearray(rec)
    ext_data = nonresident(data, [(19 - split, target)], 0)
    if kind == "mft-list-nul-extension":
        from ntfs_security_fixture import named_nonresident
        ext_data = named_nonresident(data, 0x80, "\0", [(19 - split, target)], 0, 0)
    put(ext_data, 16, "QQ", split, 18)
    put(ext_data, 40, "QQQ", 0, 0, 0)
    put(ext_data, 14, "H", 0)
    put(extension, 16, "HHHH", 2, 0, u16(rec, 20), 1)
    put(extension, 32, "Q", base_seq << 48)
    put(extension, 44, "I", extension_number)
    start = u16(extension, 20)
    body = ext_data + b"\xff\xff\xff\xff\0\0\0\0"
    extension[start:] = body + bytes(1024 - start - len(body))
    put(extension, 24, "I", start + len(body))
    save_record(image, mft + extension_number * 1024, extension)
    # Move the remainder of $MFT away from its physically contiguous creator layout.
    image[target * 4096:(target + 19 - split) * 4096] = image[mft + split * 4096:mft + 19 * 4096]
    mapping = runs([(split, 4)])
    data[64:] = mapping + bytes(len(data) - 64 - len(mapping))
    put(data, 24, "Q", split - 1)
    rows = []
    for attr in attrs:
        typ = u32(attr, 0)
        rows.append(struct.pack("<IHBBQQH6x", typ, 32, 0, 26, 0, base_seq << 48, u16(attr, 14)))
        if typ == 0x80:
            ref_seq, instance, number = 2, 0, extension_number
            if "bad-sequence" in kind:
                ref_seq += 1
            if "bad-id" in kind:
                instance = 99
            if "unmapped" in kind:
                number = 40
            rows.append(struct.pack("<IHBBQQH6x", typ, 32, 0, 26, split,
                                    number | (ref_seq << 48), instance))
    content = bytearray(b"".join(rows))
    if "bad-list" in kind:
        put(content, 4, "H", 0)
    if "nonresident" in kind:
        attr_list = nonresident(data, [(1, 1100)], len(content))
        image[1100 * 4096:1100 * 4096 + len(content)] = content
    else:
        attr_list = bytearray((24 + len(content) + 7) & ~7)
        put(attr_list, 0, "II", 0x20, len(attr_list))
        put(attr_list, 16, "IH", len(content), 24)
        attr_list[24:24 + len(content)] = content
    put(attr_list, 0, "I", 0x20)
    put(attr_list, 14, "H", 14)
    put(rec, 40, "H", 15)
    attrs.insert(1, attr_list)
    body = b"".join(attrs) + b"\xff\xff\xff\xff\0\0\0\0"
    start = u16(rec, 20)
    rec[start:] = body + bytes(1024 - start - len(body))
    put(rec, 24, "I", start + len(body))
    save_record(image, offset, rec)
    # Mark the extension record and the newly used physical clusters allocated.
    for number, bits in [(0, [extension_number]), (6, list(range(target, target + 19 - split)) + [1100])]:
        _, owner = record(original, number)
        attr = next(a for _, a in attributes(owner) if u32(a, 0) == (0xb0 if number == 0 else 0x80))
        run = u16(attr, 32)
        size_bytes, offset_bytes = attr[run] & 15, attr[run] >> 4
        lcn = int.from_bytes(attr[run + 1 + size_bytes:run + 1 + size_bytes + offset_bytes], "little")
        for bit in bits:
            image[lcn * 4096 + bit // 8] |= 1 << (bit % 8)
    return bytes(image), files
