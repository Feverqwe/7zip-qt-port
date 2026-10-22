"""Checksummed directories with mapped, missing and unwritten logical blocks."""
import struct
from ext_modern_fixture import ModernVolume
from ext_fixture import Volume, NAME, wide_indirect_volume


class DirectoryVolume(ModernVolume):
    def __init__(self, mapping="extent", fault=None, encoding=None, encoding_flags=0, tail_logical=2):
        super().__init__()
        bs = self.block_size
        leaf = bytearray(bs)
        leaf[:bs - 12] = self.dirent(12, b"after-hole.bin", bs - 12)
        struct.pack_into("<IHBBI", leaf, bs - 12, 0, 12, 0, 0xDE, 0)
        tail = self.allocate(leaf)
        self.tail_block = tail
        self.directory_head_offset = self.root_block * bs
        self.directory_tail_offset = tail * bs
        self.tail_logical = tail_logical
        self.directory_size = (tail_logical + 1) * bs
        self.dir_leaves.append(tail)
        self.put32(self.root_inode + 4, self.directory_size & 0xFFFFFFFF)
        self.put32(self.root_inode + 108, self.directory_size >> 32)
        if self.directory_size >= 1 << 32:
            self.put32(1024 + 96, self.u32(1024 + 96) | 0x4000)
        self.put32(self.root_inode + 28, 2 * bs // 512)
        struct.pack_into("<H", self.data, self.file_inode + 26, 2)
        if mapping == "indirect":
            self.data[self.root_inode + 40:self.root_inode + 100] = bytes(60)
            self.put32(self.root_inode + 40, self.root_block)
            refs = bs // 4
            logical = tail_logical
            if logical < 12:
                offsets = [logical]
            elif logical < 12 + refs:
                offsets = [12, logical - 12]
            elif logical < 12 + refs + refs * refs:
                logical -= 12 + refs
                offsets = [13, logical // refs, logical % refs]
            else:
                logical -= 12 + refs + refs * refs
                offsets = [14, logical // (refs * refs), logical // refs % refs, logical % refs]
            base = self.root_inode + 40
            self.directory_mapping_offset = 0
            for offset in offsets[:-1]:
                block = self.allocate()
                if not self.directory_mapping_offset:
                    self.directory_mapping_offset = block * bs
                self.put32(base + offset * 4, block)
                base = block * bs
            self.put32(base + offsets[-1] * 4, tail)
            self.put32(self.root_inode + 28, (len(offsets) + 1) * bs // 512)
        else:
            entries = [(0, 1, self.root_block), (tail_logical, 1, tail)]
            if mapping in ("unwritten", "allocated-zero"):
                middle = self.allocate(b"unwritten metadata secret" if mapping == "unwritten" else b"")
                entries.insert(1, (1, 0x8001 if mapping == "unwritten" else 1, middle))
            self.put32(self.root_inode + 32, 1 << 19)
            self.data[self.root_inode + 40:self.root_inode + 100] = self.extent_header(0, entries, 4)
        if encoding is not None:
            self.put32(1024 + 96, self.u32(1024 + 96) | 0x20000)
            self.put32(self.root_inode + 32, self.u32(self.root_inode + 32) | 0x40000000)
            struct.pack_into("<HH", self.data, 1024 + 0x27C, encoding, encoding_flags)
        self.refresh()
        if fault == "checksum":
            self.data[tail * bs + bs - 1] ^= 1
        elif fault == "mapping":
            self.put32(base + offsets[-1] * 4, self.table)

    def u32(self, offset):
        return struct.unpack_from("<I", self.data, offset)[0]


class NamespaceVolume(Volume):
    def __init__(self, invalid_target=False):
        super().__init__("direct")
        bs = self.block_size
        contents = ModernVolume.dirent(13, b".", 12, 2) + ModernVolume.dirent(2, b"..", 12, 2)
        if invalid_target:
            contents += ModernVolume.dirent(12, b"before-error.bin", 24)
            contents += ModernVolume.dirent(14, b"unreadable.bin", 24)
            contents += ModernVolume.dirent(12, b"after-bad-inode.bin", bs - len(contents))
        else:
            contents += ModernVolume.dirent(12, b"before-error.bin", bs - len(contents))
        first = self.allocate(contents)
        self.allocate()
        last = self.allocate(ModernVolume.dirent(12, b"after-error.bin", bs))
        inode = self.table * bs + 12 * 128
        self.inode(inode, 0o40755, 2 * bs, 2 * bs // 512, links=2)
        self.put32(inode + 40, first)
        self.put32(inode + 44, last)
        struct.pack_into("<H", self.data, self.root_inode + 26, 3)
        struct.pack_into("<H", self.data, self.file_inode + 26, 4 if invalid_target else 3)
        name = NAME.encode()
        length = (8 + len(name) + 3) & ~3
        root = ModernVolume.dirent(2, b".", 12, 2) + ModernVolume.dirent(2, b"..", 12, 2)
        root += ModernVolume.dirent(12, name, length)
        root += ModernVolume.dirent(13, b"DIRECTORY", bs - len(root), 2)
        self.data[self.root_block * bs:(self.root_block + 1) * bs] = root
        self.directory_head_offset = first * bs
        self.directory_tail_offset = last * bs
        self.superblock((13,))


def wide_indirect_directory_volume():
    v = wide_indirect_volume()
    bs = v.block_size
    refs = bs // 4
    root = bytearray(v.data[v.file_inode + 40:v.file_inode + 100])
    struct.pack_into("<I", root, 0, v.root_block)
    last_index = ((1 << 32) - 1 - 12 - refs - refs * refs) % refs
    leaf = v.indirect[max(v.indirect, key=len)] * bs
    tail = struct.unpack_from("<I", v.data, leaf + last_index * 4)[0]
    v.data[tail * bs:(tail + 1) * bs] = bytes(bs)
    for offset, length, name in ((0, 12, b"x"), (12, bs - 12, b"last-entry.bin")):
        struct.pack_into("<IHBB", v.data, tail * bs + offset, 13, length, len(name), 1)
        v.data[tail * bs + offset + 8:tail * bs + offset + 8 + len(name)] = name
    v.inode(v.root_inode, 0o40755, (1 << 32) * bs, 5 * bs // 512, links=2)
    v.data[v.root_inode + 40:v.root_inode + 100] = root
    v.data[v.file_inode:v.file_inode + 128] = bytes(128)
    payload = b"small file in a wide directory\n"
    v.inode(v.file_inode, 0o100644, len(payload), bs // 512)
    v.put32(v.file_inode + 40, v.allocate(payload))
    struct.pack_into("<H", v.data, v.link_inode + 26, 3)
    v.size = len(payload)
    v.content = {0: payload}
    v.files[NAME] = payload
    v.files["last-entry.bin"] = v.files["link"]
    v.superblock()
    v.put32(1024 + 96, 2 | 0x4000)
    v.put32(1024 + 100, 2 | 8)
    return v


def type_hint_volume(kind="conflict", inline=False, indexed=False):
    hint = {"unknown": 0, "unsupported": 255, "conflict": 2,
            "legacy": 255, "directory": 1}[kind]
    if indexed:
        v = ModernVolume(indexed=True)
        leaf = v.dir_leaves[0] * v.block_size
        records = (v.dirent(12, NAME.encode(), 56, hint) +
                   v.dirent(12, b"following.bin", v.block_size - 12 - 56))
        v.data[leaf:leaf + len(records)] = records
        struct.pack_into("<H", v.data, v.file_inode + 26, 2)
        v.files = {NAME: v.read(0, v.size), "following.bin": v.read(0, v.size)}
    elif inline:
        v = ModernVolume(inline=True)
        payload = (struct.pack("<I", 2) + v.dirent(12, NAME.encode(), 56, hint) +
                   v.dirent(12, b"following.bin", 40))
        v.set_inline(v.inline_dir_inode, payload)
        v.files = {"nested/" + NAME: v.payload, "nested/following.bin": v.payload}
    else:
        v = NamespaceVolume()
        bs = v.block_size
        records = ModernVolume.dirent(13, b".", 12, 2)
        records += ModernVolume.dirent(2, b"..", 12, 2)
        records += ModernVolume.dirent(12, b"before-error.bin", 24, hint)
        records += ModernVolume.dirent(12, b"following.bin", bs - len(records))
        first = v.directory_head_offset
        v.data[first:first + bs] = records
        struct.pack_into("<H", v.data, v.file_inode + 26, 4)
        v.files = {"DIRECTORY/before-error.bin": v.read(0, v.size),
                   "DIRECTORY/following.bin": v.read(0, v.size),
                   "DIRECTORY/after-error.bin": v.read(0, v.size)}
    if kind == "directory":
        root_child = v.root_block * v.block_size
        if not indexed:
            root_child += 24 + ((8 + len(NAME.encode()) + 3) & ~3)
        v.data[root_child + 7] = 1
    if kind == "legacy":
        incompat = struct.unpack_from("<I", v.data, 1024 + 96)[0]
        v.put32(1024 + 96, incompat & ~2)
    if inline or indexed:
        v.refresh()
    return v
