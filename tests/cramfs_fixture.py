"""Deterministic Linux cramfs images, including extended physical block layouts."""
import struct
import zlib
import lzma


def inode(mode, length, offset, name_length=0, endian="<", uid=1234, gid=42):
    if endian == "<":
        fields = (mode | (uid << 16), length | (gid << 24), (offset // 4 << 6) | (name_length // 4))
    else:
        fields = ((mode << 16) | uid, (length << 8) | gid, (name_length // 4 << 26) | (offset // 4))
    return struct.pack(endian + "III", *fields)


def checksum(data, endian="<", prefix=0):
    struct.pack_into(endian + "I", data, prefix + 4, len(data))
    struct.pack_into(endian + "I", data, prefix + 32, 0)
    struct.pack_into(endian + "I", data, prefix + 32, zlib.crc32(data[prefix:]))


def declare_size(data, size, endian="<", prefix=0):
    data = bytearray(data)
    struct.pack_into(endian + "I", data, prefix + 4, size)
    struct.pack_into(endian + "I", data, prefix + 32, 0)
    struct.pack_into(endian + "I", data, prefix + 32, zlib.crc32(data[prefix:size]))
    return data


def image(blocks, endian="<", storage=None, prefix=0, shifted=0, reverse=False,
          name=b"data.bin", short_zlib=False, version2=True, mode=0o100644, directory=None,
          legacy_block_bits=None):
    storage = storage or ["zlib" if any(block) else "hole" for block in blocks]
    extended = any(kind in ("raw", "direct-raw", "direct-zlib") for kind in storage)
    name = name + bytes((-len(name)) % 4)
    directory_start = prefix + 76 + shifted
    dirname = directory + bytes((-len(directory)) % 4) if directory else b""
    file_directory_start = directory_start + (12 + len(dirname) if directory else 0)
    table = file_directory_start + 12 + len(name)
    file_entry = inode(mode, sum(map(len, blocks)), table if blocks else 0, len(name), endian) + name
    result = bytearray(table + 4 * len(blocks))
    flags = int(version2) | 0x100 | (0x800 if extended else 0) | (0x400 if shifted else 0)
    if legacy_block_bits is not None:
        flags |= ((legacy_block_bits - 12) << 11) | ((2 if "lzma" in storage else 1) << 14)
    result[prefix:prefix + 32] = struct.pack(endian + "IIII16s", 0x28CD3D45, 0, flags, 0,
                                            b"Compressed ROMFS")
    root_size = 12 + len(dirname) if dirname else len(file_entry)
    result[prefix + 64:prefix + 76] = inode(0o40755, root_size, directory_start, endian=endian)
    if dirname:
        result[directory_start:file_directory_start] = inode(0o40755, len(file_entry), file_directory_start,
                                                              len(dirname), endian) + dirname
    result[file_directory_start:table] = file_entry
    order = reversed(range(len(blocks))) if reverse else range(len(blocks))
    for index in order:
        block, kind = blocks[index], storage[index]
        direct = kind.startswith("direct-")
        if direct:
            result.extend(bytes((-len(result)) % 4))
        start = len(result)
        if kind != "hole":
            if kind == "lzma":
                packed = (b"\x5d" + struct.pack("<II", 4096, len(block)) +
                          lzma.compress(block, format=lzma.FORMAT_RAW,
                                        filters=[{"id": lzma.FILTER_LZMA1, "dict_size": 4096, "lc": 3, "lp": 0, "pb": 2}]))
            else:
                packed = block if kind.endswith("raw") else zlib.compress(block.rstrip(b"\0") if short_zlib else block)
            if kind == "direct-zlib":
                result.extend(struct.pack(endian + "H", len(packed)))
            result.extend(packed)
        pointer = ((start // 4) | 0x40000000) if direct else len(result)
        if kind.endswith("raw"):
            pointer |= 0x80000000
        struct.pack_into(endian + "I", result, table + index * 4, pointer)
    struct.pack_into(endian + "IIII", result, prefix + 32, 0, 0, len(blocks), 2)
    checksum(result, endian, prefix)
    return bytes(result)


def ambiguous_origin(endian="<"):
    first = image([b"selected candidate\n"], endian, name=b"alpha", version2=False)
    second = image([b"other candidate\n"], endian, name=b"bravo", version2=False)
    data = bytearray(512 + len(second))
    data[76:len(first)] = first[76:]
    data[512:] = second
    return data, b"selected candidate\n"


def partial_origin(endian="<"):
    payload = b"readable alternative\n"
    packed = zlib.compress(payload)
    data = bytearray(644 + len(packed))
    data[512:588] = image([], endian, version2=False)[:76]
    data[576:588] = inode(0o40755, 32, 76, endian=endian)
    data[588:608] = inode(0o100644, len(payload), 128, 8, endian) + b"alpha\0\0\0"
    data[608:620] = inode(0o100644, 0, 0, endian=endian)
    struct.pack_into(endian + "I", data, 640, 132 + len(packed))
    data[644:] = packed
    return data, payload


def partial_table(endian="<", prefix=512, kind="direct-raw", version2=True):
    payload = b"A" * 4096
    data = bytearray(image([payload, b"missing tail"], endian, [kind, kind],
                           prefix=prefix, name=b"alpha", version2=version2))
    root = prefix + 76
    old_table = root + 20
    data.extend(bytes((-len(data)) % 4))
    table = len(data)
    data.extend(data[old_table:old_table + 4])
    data[root:root + 12] = inode(0o100644, 4096 + 12, table, 8, endian)
    checksum(data, endian, prefix)
    return data, payload


def partial_directory(state, endian="<"):
    prefix, root = 512, 588
    child = root + 40
    child_size = 40 if state == "directory-cycle" else 36
    tables = child + child_size
    data = bytearray(tables + 8)
    files = {"alpha": b"healthy sibling\n", "BROKEN/prefix.txt": b"readable prefix\n"}
    data[prefix:prefix + 32] = struct.pack(endian + "IIII16s", 0x28CD3D45, 0, 0x801, 0,
                                         b"Compressed ROMFS")
    data[prefix + 64:prefix + 76] = inode(0o40755, 40, root, endian=endian)
    data[root:root + 20] = inode(0o40755, child_size, child, 8, endian) + b"BROKEN\0\0"
    data[root + 20:root + 40] = inode(0o100644, len(files["alpha"]), tables, 8, endian) + b"alpha\0\0\0"
    data[child:child + 24] = inode(0o100644, len(files["BROKEN/prefix.txt"]), tables + 4,
                                  12, endian) + b"prefix.txt\0\0"
    if state == "directory-cycle":
        data[child + 24:child + 40] = inode(0o40755, 40, root, 4, endian) + b"loop"
    else:
        data[child + 24:child + 36] = inode(0o100644, 0, 0, 0, endian)
    for index, payload in enumerate(files.values()):
        data.extend(bytes((-len(data)) % 4))
        struct.pack_into(endian + "I", data, tables + index * 4, 0xc0000000 | (len(data) // 4))
        data.extend(payload)
    struct.pack_into(endian + "II", data, prefix + 40, 2, 5 if state == "directory-cycle" else 4)
    checksum(data, endian, prefix)
    return data, files, child + 24


if __name__ == "__main__":
    import base64
    import json
    import sys
    payload = b"readable CramFS payload\n"
    data = bytearray(image([payload], storage=["direct-raw"], prefix=512, name=b"alpha"))
    state = sys.argv[1]
    files, fault_offset = {"alpha": payload}, 0
    if state == "crc":
        data[512 + 32] ^= 1
    elif state == "short":
        struct.pack_into("<I", data, 512 + 4, len(data) + 16)
    elif state == "counts":
        struct.pack_into("<II", data, 512 + 40, 0xffffffff, 0xffffffff)
        checksum(data, prefix=512)
    elif state == "size":
        data = declare_size(data, len(data) - 1, prefix=512)
    elif state == "table":
        data, payload = partial_table()
        files = {"alpha": payload}
    elif state == "origin":
        data, payload = ambiguous_origin()
        files = {"alpha": payload}
    elif state.startswith("directory"):
        data, files, fault_offset = partial_directory(state)
    elif state != "clean":
        raise ValueError(state)
    print(json.dumps({"size": len(data),
                      "image": base64.b64encode(zlib.compress(data)).decode(),
                      "fault_offset": fault_offset,
                      "files": {name: base64.b64encode(value).decode() for name, value in files.items()}}))
