"""Small ext recovery inputs with independent readable inode groups."""
import base64
import json
import struct
import sys
import zlib

from ext_fixture import NAME, Volume
from ext_modern_fixture import ModernVolume


def two_groups():
    v = Volume("direct", full_group=True)
    bs = v.block_size
    v.data.extend(bytes(1024 * bs))
    v.blocks += 1024
    block_map, inode_map, table, data = 1027, 1028, 1029, 1031
    payload = b"independent group\n"
    v.inode(table * bs + 11 * 128, 0o100644, len(payload), bs // 512)
    v.put32(table * bs + 11 * 128 + 40, data)
    v.data[data * bs:data * bs + len(payload)] = payload
    used = set(range(1025, 1032))
    for block in used:
        bit = block - 1025
        v.data[block_map * bs + bit // 8] |= 1 << (bit % 8)
    for bit in range(1024, bs * 8):
        v.data[block_map * bs + bit // 8] |= 1 << (bit % 8)
    for bit in (11, *range(16, bs * 8)):
        v.data[inode_map * bs + bit // 8] |= 1 << (bit % 8)
    struct.pack_into("<IIIHHH", v.data, v.gdt * bs + 32,
                     block_map, inode_map, table, 1024 - len(used), 15, 0)
    v.put32(1024, 32)
    v.put32(1024 + 4, v.blocks)
    v.put32(1024 + 12, v.blocks - v.first - len(v.used) - len(used))
    v.put32(1024 + 16, 20)
    records = ((2, b".", 2), (2, b"..", 2), (12, NAME.encode(), 1),
               (28, b"other.bin", 1))
    pos = v.root_block * bs
    for index, (ino, name, kind) in enumerate(records):
        length = ((8 + len(name) + 3) // 4) * 4 if index + 1 < len(records) else v.root_block * bs + bs - pos
        struct.pack_into("<IHBB", v.data, pos, ino, length, len(name), kind)
        v.data[pos + 8:pos + 8 + len(name)] = name
        pos += length
    return v, payload, data * bs


def volume(kind):
    v, second, second_offset = two_groups()
    files = {12: v.read(0, v.size), 28: second}
    fault_offset = fault_end = 0
    bs = v.block_size
    if kind == "short-tail":
        del v.data[-bs:]
    elif kind == "short-file":
        del v.data[second_offset + 7:]
    elif kind == "block-bitmap":
        bit = v.block_map - v.first
        v.data[v.block_map * bs + bit // 8] &= ~(1 << (bit % 8))
    elif kind == "allocation-blocks":
        v.put32(1024 + 12, v.blocks + 1)
    elif kind.startswith("allocation-inodes"):
        v.put32(1024 + 16, 0xFFFFFFFF if kind.endswith("overflow") else 32)
    elif kind == "inode-bitmap":
        fault_offset, fault_end = v.inode_map * bs, (v.inode_map + 1) * bs
    elif kind == "block-bitmap-io":
        fault_offset, fault_end = v.block_map * bs, (v.block_map + 1) * bs
    elif kind == "descriptor-root-io":
        fault_offset, fault_end = v.gdt * bs, v.gdt * bs + 32
    elif kind == "descriptor-sibling-io":
        fault_offset, fault_end = v.gdt * bs + 32, v.gdt * bs + 64
    elif kind == "descriptor-all-io":
        fault_offset, fault_end = v.gdt * bs, v.gdt * bs + 64
    elif kind == "descriptor-table-range":
        v.put32(v.gdt * bs + 8, v.blocks + 1)
    elif kind == "bitmap-address":
        v.put32(v.gdt * bs, v.blocks + 1)
        v.put32(v.gdt * bs + 4, v.blocks + 2)
    elif kind == "bitmap-overlap":
        v.put32(v.gdt * bs, v.inode_map)
    elif kind == "root-directory-io":
        fault_offset, fault_end = v.root_block * bs, (v.root_block + 1) * bs
    elif kind == "root-inode":
        v.data[v.root_inode:v.root_inode + 128] = bytes(128)
    elif kind in ("file-mapping", "overlap-file-mapping"):
        v.put32(v.file_inode + 40, v.table)
        if kind == "overlap-file-mapping":
            v.put32(v.gdt * bs, v.inode_map)
    elif kind == "control":
        pass
    elif kind in ("descriptor-checksum", "root-block-uninit", "root-inode-uninit",
                  "inode-bitmap-checksum", "root-directory-checksum"):
        v = ModernVolume()
        files = {12: v.read(0, v.size)}
        if kind == "descriptor-checksum":
            v.data[v.gdt * bs + 30] ^= 1
        elif kind.endswith("-uninit"):
            struct.pack_into("<H", v.data, v.gdt * bs + 18,
                             1 if kind == "root-inode-uninit" else 2)
            v.refresh()
        elif kind == "inode-bitmap-checksum":
            v.data[v.inode_map * bs] ^= 1
        else:
            v.data[v.root_block * bs + 1020] ^= 1
    else:
        raise ValueError(kind)
    return bytes(v.data), files, fault_offset, fault_end


if __name__ == "__main__":
    image, files, offset, end = volume(sys.argv[1])
    print(json.dumps({"size": len(image), "image": base64.b64encode(zlib.compress(image)).decode(),
                      "files": {str(ino): value.hex() for ino, value in files.items()},
                      "name": NAME, "file_size": len(files.get(12, b"")),
                      "payload": files.get(12, b"").hex(),
                      "fault_offset": offset, "fault_end": end}))
