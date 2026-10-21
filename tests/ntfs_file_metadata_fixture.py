"""File metadata membership, diagnostic names and bounded parent reconstruction."""
import struct
from ntfs_fixture import attributes, nonresident, put, record, u16, u32, u64
from ntfs_ea_fixture import allocated, resident, set_attributes
from ntfs_data_list_fixture import list_bytes, row
from ntfs_security_fixture import named_resident, read_stream, index_buffer, index_root

VALID = ("meta-list-selected", "meta-list-nonresident", "meta-hardlinks-dos",
         "meta-base-si", "meta-listed-si", "meta-deep")
LIMITED = ("meta-unlisted-si", "meta-bad-fn-id", "meta-bad-fn-value", "meta-nonresident-fn",
           "meta-nul-fn", "meta-list-malformed", "meta-parent-missing", "meta-parent-stale",
           "meta-orphan-dos", "meta-cycle-self", "meta-cycle-pair")
CASES = VALID + LIMITED
LIMITATION = ("Some NTFS file metadata is unavailable or unverified. Names, times and "
              "permissions may be missing or outdated.")
MTIME = 132537600000000000
EXT_MTIME = MTIME + 864000000000


def filename(template, name, parent, kind=3, directory=False):
    data = bytearray(template[:66])
    put(data, 0, "Q", parent)
    data[64:66] = bytes((len(name.encode("utf-16-le")) // 2, kind))
    if directory:
        put(data, 40, "QQI", 0, 0, 0x10000000)
    return bytes(data) + name.encode("utf-16-le")


def index_entry(ref, name):
    size = (16 + len(name) + 7) & ~7
    data = bytearray(size)
    put(data, 0, "QHHHH", ref, size, len(name), 0, 0)
    data[16:16 + len(name)] = name
    return data


def root_names(image, replacements):
    """Rebuild the creator's one leaf, preserving all unrelated directory entries."""
    _, root = record(image, 5)
    alloc = next(a for _, a in attributes(root) if u32(a, 0) == 0xa0)
    raw = bytearray(read_stream(image, alloc)[:4096])
    usa = u16(raw, 4)
    for sector in range(1, 9):
        raw[sector * 512 - 2:sector * 512] = raw[usa + sector * 2:usa + sector * 2 + 2]
    entries, cursor = [], 24 + u32(raw, 24)
    while not u16(raw, cursor + 12) & 2:
        ref = u64(raw, cursor)
        size, length = u16(raw, cursor + 8), u16(raw, cursor + 10)
        if ref & ((1 << 48) - 1) != 66:
            entries.append((ref, bytes(raw[cursor + 16:cursor + 16 + length])))
        cursor += size
    entries += replacements
    entries.sort(key=lambda r: (r[1][66:66 + r[1][64] * 2].decode("utf-16-le").upper(), r[1][66:]))
    terminal = struct.pack("<QHHHH", 0, 16, 0, 2, 0)
    block = index_buffer([*(index_entry(ref, data) for ref, data in entries), terminal], 0)
    run = u16(alloc, 32)
    width, count = alloc[run] >> 4, alloc[run] & 15
    lcn = int.from_bytes(alloc[run + 1 + count:run + 1 + count + width], "little", signed=True)
    image[lcn * 4096:lcn * 4096 + 4096] = block


def make_record(image, template, number, attrs, *, directory=False, owner=0, sequence=1):
    rec = bytearray(template)
    put(rec, 16, "HH", sequence, 1)
    put(rec, 22, "H", 3 if directory else 1)
    put(rec, 32, "Q", owner)
    put(rec, 40, "H", 100)
    put(rec, 44, "I", number)
    offset = u64(image, 48) * 4096 + number * 1024
    set_attributes(image, offset, rec, sorted(attrs, key=lambda a: u32(a, 0)))


def directory_attrs(std, template, name, parent, child_ref, child_name):
    standard = bytearray(std)
    put(standard, 32, "I", 0x10000000)
    body = filename(template, name, parent, directory=True)
    terminal = struct.pack("<QHHHH", 0, 16, 0, 2, 0)
    index = bytearray(index_root(1, [index_entry(child_ref, child_name), terminal]))
    put(index, 0, "I", 0x30)
    return [resident(0x10, standard, 1), resident(0x30, body, 2),
            named_resident(0x90, "$I30", index, 3)]


def deep(image, files, rec, std, name_template):
    old = u64(image, 48) * 4096
    count = 367
    table = bytes(image[old:old + 67 * 1024]) + bytes((count - 67) * 1024)
    new = 1600 * 4096
    image[new:new + len(table)] = table
    put(image, 48, "Q", 1600)
    put(image, len(image) - 512 + 48, "Q", 1600)
    offset, mft = record(image, 0)
    attrs = [a for _, a in attributes(mft)]
    data = next(a for a in attrs if u32(a, 0) == 0x80 and not a[9])
    attrs[attrs.index(data)] = nonresident(data, [((count + 3) // 4, 1600)], count * 1024)
    bitmap = next(a for a in attrs if u32(a, 0) == 0xb0)
    put(bitmap, 48, "QQ", (count + 7) // 8, (count + 7) // 8)
    set_attributes(image, offset, mft, attrs)
    allocated(image, 0, 0xb0, range(67, count))
    allocated(image, 6, 0x80, range(1600, 1600 + (count + 3) // 4))
    _, root = record(image, 5)
    root_ref = 5 | (u16(root, 16) << 48)
    names = [f"d{i:03d}" for i in range(300)]
    for index, number in enumerate(range(67, count)):
        parent = root_ref if index == 0 else (number - 1) | (1 << 48)
        child_ref = (number + 1) | (1 << 48) if index < 299 else 66 | (u16(rec, 16) << 48)
        child = filename(name_template, names[index + 1], number | (1 << 48), directory=True) if index < 299 else filename(name_template, "gamma", number | (1 << 48))
        make_record(image, rec, number, directory_attrs(std, name_template, names[index], parent, child_ref, child), directory=True)
    offset, gamma = record(image, 66)
    attrs = [a for _, a in attributes(gamma)]
    fn = next(a for a in attrs if u32(a, 0) == 0x30)
    attrs[attrs.index(fn)] = resident(0x30, filename(name_template, "gamma", 366 | (1 << 48)), u16(fn, 14))
    set_attributes(image, offset, gamma, attrs)
    root_names(image, [(67 | (1 << 48), filename(name_template, names[0], root_ref, directory=True))])
    mirror = u64(image, 56) * 4096
    image[mirror:mirror + 4096] = image[new:new + 4096]
    files['/'.join([*names, "gamma"])] = files.pop("gamma")
    return image, files


def volume(kind):
    from ntfs_fixture import volume as base
    if kind not in CASES:
        raise ValueError(kind)
    source, files = base()
    image = bytearray(source)
    offset, rec = record(image, 66)
    attrs = [a for _, a in attributes(rec)]
    standard = next(a for a in attrs if u32(a, 0) == 0x10)
    fn = next(a for a in attrs if u32(a, 0) == 0x30)
    std, name_template = bytearray(read_stream(image, standard)), read_stream(image, fn)
    parent, sequence = u64(name_template, 0), u16(rec, 16)
    if kind == "meta-deep":
        return tuple((bytes(i) if isinstance(i, bytearray) else i) for i in deep(image, files, rec, std, name_template))
    if kind.startswith("meta-parent-") or kind.startswith("meta-cycle-"):
        if kind == "meta-parent-missing":
            parent = 9999 | (1 << 48)
            path = "[LOST]/gamma"
        elif kind == "meta-parent-stale":
            parent ^= 1 << 48
            path = "gamma"
        else:
            parent = 16 | (1 << 48)
            other = 16 if kind == "meta-cycle-self" else 17
            child = filename(name_template, "gamma", parent)
            make_record(image, rec, 16, directory_attrs(std, name_template, "nodea", other | (1 << 48), 66 | (sequence << 48), child), directory=True)
            allocated(image, 0, 0xb0, [16])
            if other == 17:
                make_record(image, rec, 17, directory_attrs(std, name_template, "nodeb", parent, 16 | (1 << 48), filename(name_template, "nodea", 17 | (1 << 48), directory=True)), directory=True)
                allocated(image, 0, 0xb0, [17])
            path = "[LOST]/nodea/gamma"
        attrs[attrs.index(fn)] = resident(0x30, filename(name_template, "gamma", parent), u16(fn, 14))
        set_attributes(image, offset, rec, attrs)
        files[path] = files.pop("gamma")
        return bytes(image), files
    ext, selected = [], []
    if kind in ("meta-base-si", "meta-listed-si", "meta-unlisted-si"):
        put(std, 8, "Q", MTIME)
        put(std, 32, "I", 0x21)
        attrs[attrs.index(standard)] = resident(0x10, std, u16(standard, 14))
        if kind != "meta-base-si":
            attrs = [a for a in attrs if u32(a, 0) != 0x10]
        newer = bytearray(std)
        put(newer, 8, "Q", EXT_MTIME)
        ext.append(resident(0x10, newer, 30))
        if kind != "meta-unlisted-si":
            selected.append(row(ext[-1], 16, 2))
        rows = [row(a, 66, sequence) for a in attrs if u32(a, 0) != 0x10]
    else:
        names = [("omega", 1 if kind == "meta-hardlinks-dos" else 3, 20)]
        if kind == "meta-hardlinks-dos":
            names += [("alias", 0, 21), ("OMEGA~1", 2, 22)]
        if kind == "meta-orphan-dos":
            names = [("OMEGA~1", 2, 20)]
        physical = [(66 | (sequence << 48), filename(name_template, n, parent, t)) for n, t, _ in names]
        root_names(image, physical)
        ext = [resident(0x30, payload, ident) for (_, payload), (_, _, ident) in zip(physical, names)]
        if kind == "meta-bad-fn-value":
            ext[0][u16(ext[0], 20) + 64] = 255
        elif kind == "meta-nul-fn":
            ext[0] = resident(0x30, filename(name_template, "ome\0a", parent), 20)
        elif kind == "meta-nonresident-fn":
            template = next(a for a in attrs if u32(a, 0) == 0x80)
            ext[0] = nonresident(template, [(1, 1300)], 76)
            put(ext[0], 0, "I", 0x30)
            put(ext[0], 14, "H", 20)
            allocated(image, 6, 0x80, [1300])
        selected = [row(a, 16, 2) for a in ext]
        if kind == "meta-bad-fn-id":
            selected[0] = (*selected[0][:-1], 95)
        rows = [row(a, 66, sequence) for a in attrs if u32(a, 0) != 0x30]
        payload = files.pop("gamma")
        if kind in ("meta-bad-fn-id", "meta-bad-fn-value", "meta-nonresident-fn", "meta-nul-fn"):
            # The directory index still names the independently readable file.
            files["omega"] = payload
        elif kind == "meta-list-malformed":
            files["gamma"] = payload
        else:
            for name, typ, _ in names:
                if typ != 2 or kind == "meta-orphan-dos":
                    files[name] = payload
        put(rec, 18, "H", len(names))
    make_record(image, rec, 16, ext, owner=66 | (sequence << 48), sequence=2)
    allocated(image, 0, 0xb0, [16])
    listing = list_bytes([*rows, *selected])
    if kind == "meta-list-malformed":
        put(listing, 4, "H", 0)
    if kind == "meta-list-nonresident":
        template = next(a for a in attrs if u32(a, 0) == 0x80)
        attr = nonresident(template, [(1, 1100)], len(listing))
        put(attr, 0, "I", 0x20)
        put(attr, 14, "H", 90)
        image[1100 * 4096:1100 * 4096 + len(listing)] = listing
        allocated(image, 6, 0x80, [1100])
    else:
        attr = resident(0x20, listing, 90)
    attrs.append(attr)
    set_attributes(image, offset, rec, sorted(attrs, key=lambda a: u32(a, 0)))
    return bytes(image), files
