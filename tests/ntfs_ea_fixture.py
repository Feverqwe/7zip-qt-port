"""NTFS3 EA values and WSL permissions in real creator MFT records."""
import stat
import struct
from ntfs_fixture import attributes, nonresident, put, record, save_record, u16, u32, u64

VALID = ("ea-resident", "ea-nonresident", "ea-fragmented", "ea-extension", "ea-readonly",
         "ea-incomplete", "ea-long-uid", "ea-short-info", "ea-no-data", "ea-next-size",
         "ea-zero-size", "ea-device-wide", "ea-mode-high", "ea-chr", "ea-blk", "ea-fifo", "ea-sock",
         "ea-directory", "ea-stale-directory-bit")
BAD = ("ea-record-small", "ea-record-overrun", "ea-record-short", "ea-info-zero",
       "ea-info-overrun", "ea-attr-overrun", "ea-limit", "ea-bad-mode", "ea-bad-def")


def properties(kind):
    if kind.startswith("ea-xattrs") or kind in ("ea-incomplete", "ea-long-uid", "ea-short-info", "ea-no-data"):
        return {}
    mode = {"ea-directory": stat.S_IFDIR, "ea-chr": stat.S_IFCHR, "ea-blk": stat.S_IFBLK,
            "ea-fifo": stat.S_IFIFO, "ea-sock": stat.S_IFSOCK}.get(kind, stat.S_IFREG) | 0o751
    if kind == "ea-readonly":
        mode &= ~0o222
    return {"uid": 2067, "gid": 4097, "mode": mode,
            "major": 123 if kind in ("ea-chr", "ea-blk") else None,
            "minor": 4567 if kind in ("ea-chr", "ea-blk") else None}


def resident(typ, data, ident):
    attr = bytearray((24 + len(data) + 7) & ~7)
    put(attr, 0, "II", typ, len(attr))
    put(attr, 14, "HIH", ident, len(data), 24)
    attr[24:24 + len(data)] = data
    return attr


def entry(name, data, *, step=None):
    name = name.encode()
    result = bytearray(struct.pack("<IBBH", 0, 0, len(name), len(data)) + name + b"\0" + data)
    result += bytes(-len(result) % 4)
    put(result, 0, "I", len(result) if step is None else step)
    return result


def set_attributes(image, offset, rec, attrs):
    start = u16(rec, 20)
    content = b"".join(attrs) + b"\xff\xff\xff\xff\0\0\0\0"
    assert start + len(content) <= 1024
    rec[start:] = content + bytes(1024 - start - len(content))
    put(rec, 24, "I", start + len(content))
    save_record(image, offset, rec)


def allocated(image, owner, typ, bits):
    _, rec = record(image, owner)
    attr = next(a for _, a in attributes(rec) if u32(a, 0) == typ)
    run = u16(attr, 32)
    length, width = attr[run] & 15, attr[run] >> 4
    lcn = int.from_bytes(attr[run + 1 + length:run + 1 + length + width], "little")
    for bit in bits:
        image[lcn * 4096 + bit // 8] |= 1 << (bit % 8)


def volume(kind):
    from ntfs_fixture import volume as base
    original, files = base()
    image = bytearray(original)
    offset, rec = record(image, 11 if kind == "ea-directory" else 66)
    attrs = [a for _, a in attributes(rec)]
    mode = properties(kind).get("mode", stat.S_IFREG | 0o751)
    if kind == "ea-readonly":
        mode = stat.S_IFREG | 0o751
    elif kind == "ea-mode-high":
        mode |= 0x80000000
    elif kind == "ea-bad-mode":
        mode = 0o751
    values = [("$LXUID", struct.pack("<I", 2067)), ("$LXGID", struct.pack("<I", 4097)),
              ("$LXMOD", struct.pack("<I", mode)), ("$LXDEV", struct.pack("<I", (123 << 20) | 4567))]
    if kind.startswith("ea-xattrs"):
        values = [("user.comment", b"EA payload\0with binary\xff"), ("user.empty", b""),
                  ("user.имя", b"Unicode name"), ("user.slash/name:evil", b"escaped name"),
                  ("user.case", b"lower"), ("user.CASE", b"upper"),
                  ("user.comment", b"stale duplicate")]
        if kind == "ea-xattrs-fragmented":
            values.insert(0, ("user.large", b"F" * 9000))
        seen = set()
        for name, value in values:
            if name in seen:
                continue
            seen.add(name)
            escaped = ''.join(chr(c) if chr(c).isascii() and (chr(c).isalnum() or chr(c) in '._-$')
                              else '%%%02X' % c for c in name.encode())
            suffix = '#1' if name == 'user.CASE' or (kind == 'ea-xattrs-collision' and
                                                    name == 'user.comment') else ''
            files['gamma:$EA.' + escaped + suffix] = value
        if kind == "ea-xattrs-collision":
            from ntfs_security_fixture import named_resident
            attrs.append(named_resident(0x80, "$EA.user.comment", b"original ADS", 15))
            files['gamma:$EA.user.comment'] = b"original ADS"
    if kind == "ea-incomplete":
        values.pop(1)
    if kind == "ea-long-uid":
        values[0] = ("$LXUID", bytes(5))
    if kind == "ea-device-wide":
        values[-1] = ("$LXDEV", bytes(12))
    if kind == "ea-fragmented":
        values.insert(0, ("user.large", b"F" * 9000))
    data = bytearray(b"".join(entry(name, value) for name, value in values))
    if kind == "ea-zero-size":
        put(data, len(data) - 20, "I", 0)
    elif kind == "ea-next-size":
        put(data, 0, "I", 24)
        data[20:20] = bytes(4)
    elif kind == "ea-record-small":
        put(data, 0, "I", 4)
    elif kind == "ea-record-overrun":
        put(data, 0, "I", len(data) + 4)
    elif kind == "ea-record-short":
        data = data[:7]
    info = bytearray(struct.pack("<HHI", max(0, len(data) - 4 * len(values)), 0, len(data)))
    if kind == "ea-short-info":
        info = info[:7]
    elif kind == "ea-info-zero":
        put(info, 4, "I", 0)
    elif kind == "ea-info-overrun":
        put(info, 4, "I", len(data) + 4)
    if kind == "ea-readonly":
        std = next(a for a in attrs if u32(a, 0) == 0x10)
        put(std, u16(std, 20) + 32, "I", u32(std, u16(std, 20) + 32) | 1)
    elif kind == "ea-stale-directory-bit":
        std = next(a for a in attrs if u32(a, 0) == 0x10)
        put(std, u16(std, 20) + 32, "I", u32(std, u16(std, 20) + 32) | 0x10)
    info_attr = resident(0xD0, info, 12)
    ea_attr = resident(0xE0, data, 13)
    if kind in ("ea-nonresident", "ea-fragmented", "ea-attr-overrun", "ea-xattrs-fragmented"):
        count = (len(data) + 4095) // 4096
        clusters = [1100 + 2 * i for i in range(count)]
        for i, cluster in enumerate(clusters):
            piece = data[i * 4096:(i + 1) * 4096]
            image[cluster * 4096:(cluster + 1) * 4096] = piece + bytes(4096 - len(piece))
        template = next(a for a in attrs if u32(a, 0) == 0x80)
        template = bytearray(template)
        put(template, 0, "I", 0xE0)
        ea_attr = nonresident(template, [(1, cluster) for cluster in clusters], len(data))
        put(ea_attr, 14, "H", 13)
        if kind == "ea-attr-overrun":
            put(ea_attr, 40, "QQQ", 131072, 131072, 131072)
        allocated(image, 6, 0x80, clusters)
    if kind in ("ea-limit", "ea-bad-def"):
        def_offset, defs = record(image, 4)
        def_attr = next(a for _, a in attributes(defs) if u32(a, 0) == 0x80)
        rp = u16(def_attr, 32)
        lcn = int.from_bytes(def_attr[rp + 2:rp + 2 + (def_attr[rp] >> 4)], "little")
        for pos in range(lcn * 4096, lcn * 4096 + u64(def_attr, 48), 0xa0):
            if kind == "ea-bad-def":
                put(image, pos + 0x80, "I", 0)
                break
            if u32(image, pos + 0x80) == 0xE0:
                put(image, pos + 0x98, "Q", 32)
                break
    if kind in ("ea-extension", "ea-xattrs-extension"):
        extension = bytearray(rec)
        put(extension, 16, "H", 2)
        put(extension, 32, "Q", 66 | (u16(rec, 16) << 48))
        put(extension, 44, "I", 16)
        set_attributes(image, u64(image, 0x30) * 4096 + 16 * 1024, extension, [ea_attr])
        rows = []
        for a in [*attrs, info_attr]:
            rows.append(struct.pack("<IHBBQQH6x", u32(a, 0), 32, 0, 26, 0,
                                    66 | (u16(rec, 16) << 48), u16(a, 14)))
        rows.append(struct.pack("<IHBBQQH6x", 0xE0, 32, 0, 26, 0, 16 | (2 << 48), 13))
        attrs.insert(1, resident(0x20, b"".join(rows), 14))
        allocated(image, 0, 0xB0, [16])
    attrs.append(info_attr)
    if kind not in ("ea-no-data", "ea-extension", "ea-xattrs-extension"):
        attrs.append(ea_attr)
    set_attributes(image, offset, rec, attrs)
    if kind in ("ea-chr", "ea-blk", "ea-fifo", "ea-sock"):
        files.pop("gamma")
    return bytes(image), files
