"""Native HFS+ hardlinks, Unicode names and targeted catalog mutations."""
from pathlib import Path
import struct
import zlib


def volume(kind="modern"):
    image = bytearray(zlib.decompress(
        (Path(__file__).parent / "fixtures/hfs/hfsplus-modern.img.zlib").read_bytes()))
    def get16(offset):
        return struct.unpack_from(">H", image, offset)[0]
    def get32(offset):
        return struct.unpack_from(">I", image, offset)[0]
    def put32(offset, value):
        struct.pack_into(">I", image, offset, value)
    block = get32(1064)
    catalog = get32(1024 + 0x110 + 16) * block
    size = get16(catalog + 32)
    node = get32(catalog + 24)
    records, keys, threads, catalog_records = {}, {}, {}, []
    while node:
        start = catalog + node * size
        for i in range(get16(start + 10)):
            key = start + get16(start + size - 2 * (i + 1))
            length = get16(key + 6)
            name = image[key + 8:key + 8 + length * 2].decode("utf-16-be")
            record = key + 2 + get16(key)
            end = start + get16(start + size - 2 * (i + 2))
            catalog_records.append((key, end))
            if get16(record) in (1, 2):
                records[name], keys[name] = record, key
            elif get16(record) in (3, 4):
                threads[get32(key + 2)] = record
        node = get32(start)
    files = {"HfsFixture/данные 空": b"Second HFS+ stream\n" * 4097}
    for name in ("alpha", "hard-alpha", "links/second-link"):
        files["HfsFixture/" + name] = bytes(range(256)) * 513
        files["HfsFixture/" + name + ":rsrc"] = b"Resource fork\n" * 173
        files["HfsFixture/" + name + ":com.sevenzip.fixture"] = b"inline attribute content"
    for name in ("café", "각", "emoji 😃", "null-name", "slash:name", "single \u200dX"):
        files["HfsFixture/" + name.replace(':', '_')] = ("payload for " + name + "\n").encode() * 5
    if kind == "modern-permissions":
        for name in ("iNode20", "links"):
            record = records[name]
            put32(record + 32, 0)
            put32(record + 36, 0)
            struct.pack_into(">H", image, record + 42, 0)
        image[records["iNode20"] + 40:records["iNode20"] + 42] = bytes((6, 3))
        file_info = b"BINASZIP" + bytes(range(24))
        folder_info = bytes(range(32))
        image[records["iNode20"] + 48:records["iNode20"] + 80] = file_info
        image[records["links"] + 48:records["links"] + 80] = folder_info
        for name in ("alpha", "hard-alpha", "links/second-link"):
            files["HfsFixture/" + name + ":com.apple.FinderInfo"] = file_info
        files["HfsFixture/links:com.apple.FinderInfo"] = folder_info
    elif kind == "modern-invalid-file-mode":
        struct.pack_into(">H", image, records["iNode20"] + 42, 0o40755)
    elif kind == "modern-invalid-dir-mode":
        struct.pack_into(">H", image, records["links"] + 42, 0o100644)
    elif kind.startswith("modern-thread-"):
        thread = threads[get32(records["iNode20"] + 8)]
        if kind == "modern-thread-parent":
            put32(thread + 4, 2)
        elif kind == "modern-thread-name":
            image[thread + 23] = ord('1')
        elif kind == "modern-thread-type":
            struct.pack_into(">H", image, thread, 3)
        elif kind == "modern-thread-length":
            struct.pack_into(">H", image, thread + 8, 256)
        else:
            raise ValueError(kind)
    elif kind == "modern-nul":
        key = keys["null-name"]
        replacement = "null\0name".encode("utf-16-be")
        image[key + 8:key + 8 + len(replacement)] = replacement
        thread = threads[get32(records["null-name"] + 8)]
        image[thread + 10:thread + 10 + len(replacement)] = replacement
        files["HfsFixture/null␀name"] = files.pop("HfsFixture/null-name")
    elif kind == "modern-date":
        put32(records["alpha"] + 12, get32(records["HfsFixture"] + 12) + 1)
        files["HfsFixture/alpha"] = b""
        del files["HfsFixture/alpha:rsrc"], files["HfsFixture/alpha:com.sevenzip.fixture"]
    elif kind == "modern-missing":
        put32(records["alpha"] + 44, 0x11223344)
    elif kind == "modern-chain":
        record = records["iNode20"]
        image[record + 48:record + 56] = b"hlnkhfs+"
        put32(record + 12, get32(records["HfsFixture"] + 12))
        put32(record + 44, 20)
        for name in ("alpha", "hard-alpha", "links/second-link"):
            files["HfsFixture/" + name] = b""
            del files["HfsFixture/" + name + ":rsrc"], files["HfsFixture/" + name + ":com.sevenzip.fixture"]
    elif kind == "modern-bounds":
        put32(records["iNode20"] + 88 + 16, get32(1068))
    elif kind == "modern-no-root":
        put32(records["HfsFixture"] + 8, 0x5566)
    elif kind == "modern-case":
        key = keys["iNode20"]
        image[key + 8:key + 22] = "INODE20".encode("utf-16-be")
    elif kind == "modern-hidden-case":
        key = keys["\0\0\0\0HFS+ Private Data"]
        replacement = ("\0" * 4 + "HFS+ PRIVATE DATA").encode("utf-16-be")
        image[key + 8:key + 8 + len(replacement)] = replacement
    elif kind == "modern-metadata":
        record = records["iNode20"]
        put32(record + 32, 0x12345678)
        put32(record + 36, 0x87654321)
        put32(record + 44, 70000)
    elif kind == "modern-xattr":
        attr = get32(1024 + 0x160 + 16) * block
        ns = get16(attr + 32)
        leaf = attr + get32(attr + 24) * ns
        for i in range(get16(leaf + 10)):
            key = leaf + get16(leaf + ns - (i + 1) * 2)
            length = get16(key + 12)
            name = image[key + 14:key + 14 + length * 2].decode("utf-16-be")
            if name == "com.sevenzip.fixture":
                replacement = "com.sevenzip.cafe\u0301xx".encode("utf-16-be")
                assert len(replacement) == length * 2
                image[key + 14:key + 14 + length * 2] = replacement
        for name in ("alpha", "hard-alpha", "links/second-link"):
            files["HfsFixture/" + name + ":com.sevenzip.caféxx"] = files.pop(
                "HfsFixture/" + name + ":com.sevenzip.fixture")
    elif kind in ("modern-zlib", "modern-lzvn"):
        payload = bytes(range(256)) * 8
        packed = zlib.compress(payload) if kind == "modern-zlib" else (
            Path(__file__).parent / "fixtures/hfs/alpha.lzvn").read_bytes()
        method = 3 if kind == "modern-zlib" else 7
        data = b"fpmc" + struct.pack("<IQ", method, len(payload)) + packed
        record = records["iNode20"]
        image[record + 88:record + 168] = bytes(80)
        attr = get32(1024 + 0x160 + 16) * block
        ns = get16(attr + 32)
        leaf = attr + get32(attr + 24) * ns
        name = "com.apple.decmpfs".encode("utf-16-be")
        key = struct.pack(">HHIIH", 12 + len(name), 0, get32(record + 8), 0, len(name) // 2) + name
        entry = key + struct.pack(">IIII", 0x10, 0, 0, len(data)) + data
        entry += bytes(len(entry) % 2)
        entries = [entry]
        for i in range(get16(leaf + 10)):
            start = leaf + get16(leaf + ns - (i + 1) * 2)
            end = leaf + get16(leaf + ns - (i + 2) * 2)
            length = get16(start + 12)
            old_name = image[start + 14:start + 14 + length * 2].decode("utf-16-be")
            if old_name != "com.sevenzip.fixture":
                entries.append(bytes(image[start:end]))
        entries.sort(key=lambda b: (struct.unpack_from(">I", b, 4)[0],
                                    b[14:2 + struct.unpack_from(">H", b)[0]]))
        image[leaf:leaf + ns] = bytes(ns)
        struct.pack_into(">IIBBHH", image, leaf, 0, 0, 255, 1, len(entries), 0)
        offset = 14
        for i, entry in enumerate(entries):
            struct.pack_into(">H", image, leaf + ns - (i + 1) * 2, offset)
            image[leaf + offset:leaf + offset + len(entry)] = entry
            offset += len(entry)
        struct.pack_into(">H", image, leaf + ns - (len(entries) + 1) * 2, offset)
        for name in ("alpha", "hard-alpha", "links/second-link"):
            files["HfsFixture/" + name] = payload
            del files["HfsFixture/" + name + ":com.sevenzip.fixture"]
    elif kind in ("modern-hfsx", "modern-binary", "modern-binary-case"):
        image[1024:1028] = b"HX\0\5"
        image[-1024:-1020] = b"HX\0\5"
        if kind != "modern-hfsx":
            if kind == "modern-binary-case":
                key = keys["iNode20"]
                image[key + 8:key + 22] = "INODE20".encode("utf-16-be")
                thread = threads[get32(records["iNode20"] + 8)]
                image[thread + 10:thread + 24] = "INODE20".encode("utf-16-be")
            blobs = [bytes(image[start:end]) for start, end in catalog_records]
            def binary_key(blob):
                name_length = struct.unpack_from(">H", blob, 6)[0]
                units = struct.unpack_from(">" + "H" * name_length, blob, 8)
                return struct.unpack_from(">I", blob, 2)[0], units
            blobs.sort(key=binary_key)
            leaves, leaf, used = [], [], 16
            for blob in blobs:
                if used + len(blob) + 2 > size:
                    leaves.append(leaf)
                    leaf, used = [], 16
                leaf.append(blob)
                used += len(blob) + 2
            leaves.append(leaf)

            def write_node(number, forward, backward, kind, height, entries):
                pos = catalog + number * size
                image[pos:pos + size] = bytes(size)
                struct.pack_into(">IIBBHH", image, pos, forward, backward,
                                 kind, height, len(entries), 0)
                offset = 14
                for i, entry in enumerate(entries):
                    struct.pack_into(">H", image, pos + size - 2 * (i + 1), offset)
                    image[pos + offset:pos + offset + len(entry)] = entry
                    offset += len(entry)
                struct.pack_into(">H", image, pos + size - 2 * (len(entries) + 1), offset)

            index = []
            for i, leaf in enumerate(leaves):
                number = i + 2
                write_node(number, number + 1 if i + 1 < len(leaves) else 0,
                           number - 1 if i else 0, 255, 1, leaf)
                key_size = 2 + struct.unpack_from(">H", leaf[0])[0]
                index.append(leaf[0][:key_size] + struct.pack(">I", number))
            write_node(1, 0, 0, 0, 2, index)
            struct.pack_into(">HIIII", image, catalog + 14, 2, 1, len(blobs), 2, len(leaves) + 1)
            put32(catalog + 40, get32(catalog + 36) - len(leaves) - 2)
            image[catalog + 51] = 0xbc
            bitmap = catalog + get16(catalog + size - 6)
            bitmap_end = catalog + get16(catalog + size - 8)
            image[bitmap:bitmap_end] = bytes(bitmap_end - bitmap)
            for i in range(len(leaves) + 2):
                image[bitmap + i // 8] |= 0x80 >> (i % 8)
    elif kind != "modern":
        raise ValueError(kind)
    return bytes(image), files
