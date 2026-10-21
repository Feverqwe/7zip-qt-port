"""hfsutils-produced classic HFS and targeted fork/catalog variants."""
from pathlib import Path
import struct
import zlib


def volume(kind="classic"):
    image = bytearray(zlib.decompress(
        (Path(__file__).parent / "fixtures/hfs/hfs-classic.img.zlib").read_bytes()))
    files = {
        "HfsFixture/alpha": bytes(range(256)) * 513,
        "HfsFixture/alpha:rsrc": b"Resource fork\n" * 173,
        "HfsFixture/alpha:hfs.type": b"TEXT",
        "HfsFixture/alpha:hfs.creator": b"SZIP",
        "HfsFixture/nested/café": b"Classic HFS nested data\n" * 719,
        "HfsFixture/nested/small63": b"small 63\n",
        "HfsFixture/slash_name": b"slash payload\n",
        "HfsFixture/empty": b"",
    }
    def get16(offset):
        return struct.unpack_from(">H", image, offset)[0]
    def get32(offset):
        return struct.unpack_from(">I", image, offset)[0]
    def put16(offset, value):
        struct.pack_into(">H", image, offset, value)
    def put32(offset, value):
        struct.pack_into(">I", image, offset, value)
    block = get32(1044)
    allocation = get16(1052) * 512
    catalog = allocation + get16(1174) * block
    node_size = get16(catalog + 32)
    node = get32(catalog + 24)
    records, threads = {}, {}
    while node:
        start = catalog + node * node_size
        for i in range(get16(start + 10)):
            key = start + get16(start + node_size - 2 * (i + 1))
            name = bytes(image[key + 7:key + 7 + image[key + 6]]).decode("latin-1")
            record = key + (image[key] | 1) + 1
            if image[record] in (1, 2):
                records[name] = record
            elif image[record] in (3, 4):
                threads[get32(key + 2)] = record
        node = get32(start)
    alpha = records["alpha"]
    if kind in ("classic-overflow", "classic-catalog-overflow"):
        identifier = get32(alpha + 20)
        extra = []
        if kind == "classic-catalog-overflow":
            physical = get16(1174)
            count = get32(1170) // block
            for i in range(3):
                struct.pack_into(">HH", image, 1174 + i * 4, physical + i, 1)
            for logical, count in ((3, 3), (6, count - 6)):
                extra.append(struct.pack(">BBIH", 7, 0, 4, logical) +
                             struct.pack(">HH", physical + logical, count) + bytes(8))
        for resource, size_offset, extent_offset in ((False, 30, 74), (True, 40, 86)):
            physical = get16(alpha + extent_offset)
            count = get32(alpha + size_offset) // block
            for i in range(3):
                struct.pack_into(">HH", image, alpha + extent_offset + i * 4, physical + i, 1)
            key = lambda logical: struct.pack(">BBIH", 7, 255 if resource else 0, identifier, logical)
            if not resource:
                extra.append(key(3) + b"".join(struct.pack(">HH", physical + i, 1) for i in range(3, 6)))
                extra.append(key(6) + struct.pack(">HH", physical + 6, count - 6) + bytes(8))
            else:
                extra.append(key(3) + struct.pack(">HH", physical + 3, count - 3) + bytes(8))
        extents = allocation + get16(1158) * block
        ns = get16(extents + 32)
        struct.pack_into(">HIIII", image, extents + 14, 2, 1, len(extra), 2, 3)
        put32(extents + 40, get32(extents + 36) - 4)
        bitmap = extents + get16(extents + ns - 6)
        image[bitmap] = 0xf0
        def make_node(number, forward, backward, node_type, height, entries):
            start = extents + number * ns
            image[start:start + ns] = bytes(ns)
            struct.pack_into(">IIBBHH", image, start, forward, backward, node_type, height, len(entries), 0)
            offset = 14
            for i, entry in enumerate(entries):
                put16(start + ns - 2 * (i + 1), offset)
                image[start + offset:start + offset + len(entry)] = entry
                offset += len(entry)
            put16(start + ns - 2 * (len(entries) + 1), offset)
        make_node(1, 0, 0, 0, 2, [extra[0][:8] + struct.pack(">I", 2),
                                  extra[1][:8] + struct.pack(">I", 3)])
        make_node(2, 3, 0, 255, 1, extra[:1])
        make_node(3, 0, 2, 255, 1, extra[1:])
    elif kind == "classic-non-power-of-two":
        original = bytes(image)
        new_block = 1536
        cursor = 0
        relocated = bytearray(allocation + get16(1042) * new_block + 1024)
        relocated[:allocation] = image[:allocation]
        # Relocate complete metadata files first; their B-tree node size stays 512.
        metadata = []
        for size_offset, extent_offset in ((1154, 1158), (1170, 1174)):
            size = get32(size_offset)
            count = (size + new_block - 1) // new_block
            old_start = allocation + get16(extent_offset) * block
            new_start = allocation + cursor * new_block
            relocated[new_start:new_start + size] = image[old_start:old_start + size]
            struct.pack_into(">HH", relocated, extent_offset, cursor, count)
            metadata.append((old_start, new_start, size))
            cursor += count
        old_catalog, new_catalog, _ = metadata[1]
        for record in records.values():
            if original[record] != 2:
                continue
            for logical_offset, physical_offset, extent_offset in ((26, 30, 74), (36, 40, 86)):
                size = get32(record + logical_offset)
                allocated = (get32(record + physical_offset) + new_block - 1) // new_block
                old_start = allocation + get16(record + extent_offset) * block
                new_start = allocation + cursor * new_block
                relocated[new_start:new_start + size] = original[old_start:old_start + size]
                new_record = new_catalog + record - old_catalog
                struct.pack_into(">I", relocated, new_record + physical_offset, allocated * new_block)
                relocated[new_record + extent_offset:new_record + extent_offset + 12] = bytes(12)
                if allocated:
                    struct.pack_into(">HH", relocated, new_record + extent_offset, cursor, allocated)
                cursor += allocated
        bitmap = get16(1038) * 512
        relocated[bitmap:allocation] = bytes(allocation - bitmap)
        for bit in range(cursor):
            relocated[bitmap + bit // 8] |= 0x80 >> (bit % 8)
        struct.pack_into(">I", relocated, 1044, new_block)
        struct.pack_into(">H", relocated, 1058, get16(1042) - cursor)
        relocated[-1024:-512] = relocated[1024:1536]
        image = relocated
    elif kind in ("classic-unclean", "classic-free-overflow", "classic-unclean-free-overflow"):
        if "unclean" in kind:
            put16(1034, get16(1034) & ~0x100)
        if "free-overflow" in kind:
            put16(1058, get16(1042) + 1)
    elif kind == "classic-locked":
        image[alpha + 2] |= 1
    elif kind == "classic-preallocated":
        put32(alpha + 26, 1024)
        files["HfsFixture/alpha"] = files["HfsFixture/alpha"][:1024]
        put32(records["slash/name"] + 26, 0)
        files["HfsFixture/slash_name"] = b""
    elif kind == "classic-missing-overflow":
        put16(alpha + 76, 3)
    elif kind == "classic-bad-fork":
        put16(alpha + 74, get16(1042))
    elif kind == "classic-short-fork":
        put32(alpha + 26, get32(alpha + 30) + 1)
    elif kind == "classic-bad-cnid":
        put32(alpha + 20, 1)
    elif kind == "classic-thread-parent":
        put32(threads[2] + 10, 2)
    elif kind == "classic-leaf-cycle":
        leaf = catalog + get32(catalog + 24) * node_size
        put32(leaf, get32(catalog + 24))
    elif kind == "classic-node-offset":
        leaf = catalog + get32(catalog + 24) * node_size
        put16(leaf + node_size - 4, node_size)
    elif kind == "classic-truncated":
        image = image[:allocation + get16(1042) * block - 1]
    elif kind != "classic":
        raise ValueError(kind)
    return bytes(image), files
