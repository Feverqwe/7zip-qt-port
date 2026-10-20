"""Indexed NTFS security descriptors, generated from the official creator image."""
import struct
from ntfs_fixture import attributes, nonresident, put, record, save_record, u16, u32, u64
from ntfs_ea_fixture import allocated, resident, set_attributes

VALID = ("security-index", "security-physical-order", "security-mirror", "security-partial-sds",
         "security-tree", "security-root-key", "security-many", "security-small-block",
         "security-fragmented-index", "security-extension", "security-sparse-sds",
         "security-empty", "security-reserved", "security-no-metadata", "security-fixup-mismatch",
         "security-deleted-corrupt")
BAD = ("security-root-offset", "security-root-used", "security-root-terminal", "security-key-overrun", "security-subnode-flag",
       "security-view-short", "security-header-mismatch", "security-sds-range", "security-sd-short",
       "security-sid-overflow", "security-sid-length", "security-acl-revision", "security-ace-overrun",
       "security-node-vbn", "security-node-fixup-range", "security-node-key-overrun",
       "security-index-range", "security-cycle")


def attr_name(attr):
    start = u16(attr, 10)
    return attr[start:start + attr[9] * 2].decode("utf-16-le")


def read_stream(image, attr):
    if not attr[8]:
        return bytes(attr[u16(attr, 20):u16(attr, 20) + u32(attr, 16)])
    result, pos, lcn = bytearray(), u16(attr, 32), 0
    while attr[pos]:
        length, width = attr[pos] & 15, attr[pos] >> 4
        count = int.from_bytes(attr[pos + 1:pos + 1 + length], "little")
        if width:
            lcn += int.from_bytes(attr[pos + 1 + length:pos + 1 + length + width], "little", signed=True)
            result += image[lcn * 4096:(lcn + count) * 4096]
        else:
            result += bytes(count * 4096)
        pos += 1 + length + width
    return bytes(result[:u64(attr, 48)])


def descriptor(seed=700):
    sid = lambda value: struct.pack("<BB6sI", 1, 1, b"\0\0\0\0\0\5", value)
    owner, group = sid(seed), sid(32)
    ace = struct.pack("<BBHI", 0, 0, 8 + len(owner), 0x1f01ff) + owner
    acl = struct.pack("<BBHHH", 2, 0, 8 + len(ace), 1, 0) + ace
    return struct.pack("<BBHIIII", 1, 0, 0x8004, 20, 20 + len(owner), 0,
                       20 + len(owner) + len(group)) + owner + group + acl


def expected(kind):
    return b"" if kind in ("security-empty", "security-reserved", "security-no-metadata",
                           "security-deleted-corrupt") else descriptor(
        500 if kind == "security-root-key" else 335 if kind == "security-many" else 700)


def named_resident(typ, name, data, ident):
    encoded = name.encode("utf-16-le")
    start = (24 + len(encoded) + 7) & ~7
    attr = bytearray((start + len(data) + 7) & ~7)
    put(attr, 0, "II", typ, len(attr))
    put(attr, 9, "BH", len(name), 24)
    put(attr, 14, "HIH", ident, len(data), start)
    attr[24:24 + len(encoded)] = encoded
    attr[start:start + len(data)] = data
    return attr


def named_nonresident(template, typ, name, entries, size, ident, flags=0):
    attr = nonresident(template, entries, size, flags=flags)
    name = name.encode("utf-16-le")
    old = u16(attr, 32)
    start = (old + len(name) + 7) & ~7
    mapping = attr[old:]
    attr = attr[:old] + bytearray(start - old) + mapping
    put(attr, 0, "II", typ, len(attr))
    put(attr, 9, "BH", len(name) // 2, old)
    put(attr, 14, "H", ident)
    put(attr, 32, "H", start)
    attr[old:old + len(name)] = name
    return attr


def index_entry(key, header=None, child=None):
    size = (16 + len(key) + (len(header) if header else 0) + (8 if child is not None else 0) + 7) & ~7
    data = bytearray(size)
    flags = (1 if child is not None else 0) | (2 if not key else 0)
    put(data, 0, "HHIHHHH", 16 + len(key) if header else 0, len(header) if header else 0,
        0, size, len(key), flags, 0)
    data[16:16 + len(key)] = key
    if header:
        data[16 + len(key):16 + len(key) + len(header)] = header
    if child is not None:
        put(data, size - 8, "Q", child)
    return data


def index_root(rule, entries, block=4096, children=False):
    body = b"".join(entries)
    return struct.pack("<IIIB3xIIII", 0, rule, block, 1 if block >= 4096 else block // 512,
                       16, 16 + len(body), 16 + len(body), int(children)) + body


def index_buffer(entries, vbn, block=4096, children=False):
    data = bytearray(block)
    body = b"".join(entries)
    assert 64 + len(body) <= block
    put(data, 0, "IHHQQ", 0x58444e49, 40, block // 512 + 1, 0, vbn)
    put(data, 24, "IIII", 40, 40 + len(body), block - 24, int(children))
    data[64:64 + len(body)] = body
    put(data, 40, "H", 0x9a73)
    for sector in range(1, block // 512 + 1):
        data[40 + sector * 2:42 + sector * 2] = data[sector * 512 - 2:sector * 512]
        data[sector * 512 - 2:sector * 512] = data[40:42]
    return data


def volume(kind):
    from ntfs_fixture import volume as base
    original, files = base()
    image = bytearray(original)
    _, old = record(image, 9)
    attrs = [a for _, a in attributes(old)]
    old_sds = next(a for a in attrs if u32(a, 0) == 0x80 and attr_name(a) == "$SDS")
    old_content = read_stream(image, old_sds)
    old_sii = read_stream(image, next(a for a in attrs if u32(a, 0) == 0x90 and attr_name(a) == "$SII"))
    descriptors = {}
    cursor = 16 + u32(old_sii, 16)
    while not u16(old_sii, cursor + 12) & 2:
        offset = u64(old_sii, cursor + 28)
        size = u32(old_sii, cursor + 36)
        descriptors[u32(old_sii, cursor + 16)] = old_content[offset + 20:offset + size]
        cursor += u16(old_sii, cursor + 8)
    gamma_id = 500 if kind == "security-root-key" else 335 if kind == "security-many" else 700
    descriptors[gamma_id] = descriptor(gamma_id)
    if kind in ("security-tree", "security-fragmented-index", "security-small-block",
                "security-extension", "security-root-key", "security-node-vbn", "security-node-fixup-range",
                "security-node-key-overrun", "security-index-range", "security-cycle", "security-fixup-mismatch"):
        descriptors[500] = descriptor(500)
        if kind == "security-root-key":
            descriptors[700] = descriptor(700)
    if kind == "security-many":
        descriptors.update((i, descriptor(i)) for i in range(258, 336))
    if kind in ("security-empty", "security-reserved"):
        descriptors.pop(gamma_id)
        if kind == "security-reserved":
            gamma_id = 17
    if kind == "security-no-metadata":
        descriptors = {}
    if kind in ("security-sd-short", "security-deleted-corrupt"):
        descriptors[gamma_id] = bytes(8)
    elif kind.startswith("security-sid-") or kind.startswith("security-acl-") or kind == "security-ace-overrun":
        raw = bytearray(descriptors[gamma_id])
        if kind == "security-sid-overflow":
            put(raw, 4, "I", 0xfffffff8)
        elif kind == "security-sid-length":
            raw[21] = 255
        elif kind == "security-acl-revision":
            raw[44] = 1
        else:
            put(raw, 54, "H", 0xffff)
        descriptors[gamma_id] = bytes(raw)
    content, headers = bytearray(), {}
    order = sorted(descriptors, reverse=kind == "security-physical-order")
    base_offset = 1 << 32 if kind == "security-sparse-sds" else 0
    for ident in order:
        data = descriptors[ident]
        checksum = 0
        for pos in range(0, len(data), 4):
            checksum = (((checksum << 3) | (checksum >> 29)) + int.from_bytes(data[pos:pos + 4], "little")) & 0xffffffff
        header = struct.pack("<IIQI", checksum, ident, base_offset + len(content), 20 + len(data))
        headers[ident] = header
        content += header + data
        content += bytes(-len(content) % 16)
    primary = bytes(content)
    if kind not in ("security-partial-sds", "security-sparse-sds", "security-no-metadata"):
        content = bytearray(primary + bytes(0x40000 - len(primary)) + primary)
    if kind == "security-mirror":
        content[-1] ^= 1
    if kind == "security-header-mismatch":
        content[struct.unpack_from("<Q", headers[gamma_id], 8)[0]] ^= 1
    if kind == "security-sds-range":
        h = bytearray(headers[gamma_id]);put(h, 8, "Q", len(content) - 8);headers[gamma_id] = bytes(h)
    entries = []
    if kind == "security-sparse-sds":
        entries.append((base_offset // 4096, None))
    count = (len(content) + 4095) // 4096
    for i in range(count):
        piece = content[i * 4096:(i + 1) * 4096]
        image[(1300 + i) * 4096:(1301 + i) * 4096] = piece + bytes(4096 - len(piece))
    if count:
        entries.append((count, 1300))
        allocated(image, 6, 0x80, range(1300, 1300 + count))
    else:
        entries.append((1, None))
    sds = named_nonresident(old_sds, 0x80, "$SDS", entries, base_offset + len(content), 2,
                            flags=0x8000 if base_offset or not count else 0)
    attrs = [a for a in attrs if u32(a, 0) not in (0x80, 0x90, 0xA0, 0xB0)] + [sds]
    index_attrs, extension_attrs = [], []
    for name, rule in (("$SDH", 0x12), ("$SII", 0x10)):
        rows = [(struct.pack("<II", u32(h, 0), ident) if name == "$SDH" else struct.pack("<I", ident), h)
                for ident, h in headers.items()]
        rows.sort(key=lambda row: (u32(row[0], 0), u32(row[0], 4) if name == "$SDH" else 0))
        tree = len(rows) > 10 or len(descriptors) == 4
        block = 1024 if kind == "security-small-block" and name == "$SII" else 4096
        nodes = []
        if tree:
            pivot = len(rows) // 2
            if kind == "security-many" and name == "$SII":
                root_entries = [index_entry(b"", child=0)]
                node_rows = [rows]
            else:
                step = block // (4096 if block >= 4096 else 512)
                root_entries = [index_entry(*rows[pivot], child=0), index_entry(b"", child=step)]
                node_rows = [rows[:pivot], rows[pivot + 1:]]
            for i, group in enumerate(node_rows):
                vbn = i * block // (4096 if block >= 4096 else 512)
                body = [index_entry(*row) for row in group] + [index_entry(b"")]
                child = kind == "security-cycle" and name == "$SII" and i == 1
                if child:
                    body = [index_entry(b"", child=vbn)]
                nodes.append(index_buffer(body, vbn, block, child))
            data = bytearray(index_root(rule, root_entries, block, True))
        else:
            data = bytearray(index_root(rule, [index_entry(*row) for row in rows] + [index_entry(b"")]))
        if name == "$SII":
            if kind == "security-root-offset":put(data, 16, "I", 0)
            elif kind == "security-root-used":put(data, 20, "I", len(data) + 16)
            elif kind == "security-root-terminal":put(data, len(data) - 4, "H", 0)
            elif kind == "security-key-overrun":put(data, 32 + 10, "H", 0xffff)
            elif kind == "security-subnode-flag":put(data, 28, "I", 1)
            elif kind == "security-view-short":
                body = [index_entry(*row) for row in rows[:-1]]
                short = index_entry(rows[-1][0], bytes(4))
                body += [short, index_entry(b"")]
                data = bytearray(index_root(rule, body))
            elif kind == "security-index-range":put(data, len(data) - 8, "Q", 1 << 63)
            if len(nodes) > 1:
                if kind == "security-node-vbn":put(nodes[1], 16, "Q", 99)
                elif kind == "security-node-fixup-range":put(nodes[1], 4, "H", 4094)
                elif kind == "security-node-key-overrun":put(nodes[1], 64 + 10, "H", 0xffff)
                elif kind == "security-fixup-mismatch":nodes[1][510] ^= 1
        root_attr = named_resident(0x90, name, data, 3 if name == "$SDH" else 4)
        target = extension_attrs if kind == "security-extension" and name == "$SII" else index_attrs
        target.append(root_attr)
        if nodes:
            first = 1200 if name == "$SII" else 1250
            lcns = [first + 2 * i for i in range((sum(map(len, nodes)) + 4095) // 4096)]
            packed = b"".join(nodes)
            for i, lcn in enumerate(lcns):
                piece = packed[i * 4096:(i + 1) * 4096]
                image[lcn * 4096:(lcn + 1) * 4096] = piece + bytes(4096 - len(piece))
            allocated(image, 6, 0x80, lcns)
            target += [named_nonresident(old_sds, 0xA0, name, [(1, lcn) for lcn in lcns], len(packed), 10 if name == "$SDH" else 11),
                       named_resident(0xB0, name, bytes([(1 << len(nodes)) - 1]), 12 if name == "$SDH" else 13)]
    attrs += index_attrs
    if extension_attrs:
        extension = bytearray(old)
        put(extension, 16, "H", 2);put(extension, 32, "Q", 9 | (u16(old, 16) << 48));put(extension, 44, "I", 16)
        extension_attrs.sort(key=lambda a: (u32(a, 0), attr_name(a)))
        set_attributes(image, u64(image, 0x30) * 4096 + 16 * 1024, extension, extension_attrs)
        allocated(image, 0, 0xB0, [16])
        rows = []
        combined = [(a, 9, u16(old, 16)) for a in attrs] + [(a, 16, 2) for a in extension_attrs]
        for a, number, sequence in sorted(combined, key=lambda row: (u32(row[0], 0), attr_name(row[0]))):
            name = attr_name(a).encode("utf-16-le");length = (26 + len(name) + 7) & ~7
            row = bytearray(struct.pack("<IHBBQQH", u32(a, 0), length, len(name) // 2, 26, 0,
                                        number | (sequence << 48), u16(a, 14)) + name)
            row += bytes(length - len(row));rows.append(row)
        attrs.append(resident(0x20, b"".join(rows), 20))
    attrs.sort(key=lambda a: (u32(a, 0), attr_name(a)))
    set_attributes(image, u64(image, 0x30) * 4096 + 9 * 1024, old, attrs)
    offset, gamma = record(image, 66)
    attrs = [a for _, a in attributes(gamma) if u32(a, 0) != 0x50]
    std = next(a for a in attrs if u32(a, 0) == 0x10)
    payload = bytearray(read_stream(image, std));payload += bytes(max(0, 72 - len(payload)))
    put(payload, 52, "I", gamma_id)
    attrs[attrs.index(std)] = resident(0x10, payload, u16(std, 14))
    if kind == "security-deleted-corrupt":
        put(gamma, 22, "H", u16(gamma, 22) & ~1)
    set_attributes(image, offset, gamma, attrs)
    return bytes(image), files
