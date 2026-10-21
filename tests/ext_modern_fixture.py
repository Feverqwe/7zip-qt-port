"""Bounded modern ext images with independently generated metadata checksums."""
import struct
from ext_fixture import Volume, NAME


def crc32c(seed, data):
    for value in data:
        seed ^= value
        for _ in range(8):
            seed = (seed >> 1) ^ (0x82F63B78 if seed & 1 else 0)
    return seed


class ModernVolume(Volume):
    def __init__(self, inline=False, seed_feature=False, descriptor_size=64, indexed=False,
                 short_leaf=False, empty_leaf=False):
        super().__init__("extent-tree")
        self.inline = inline
        self.indexed = indexed
        self.index_blocks = []
        self.dir_leaves = [self.root_block]
        self.descriptor_size = descriptor_size
        self.old_table = self.table
        old = bytes(self.data[self.table * 1024:self.table * 1024 + 2048])
        self.table = self.allocate()
        for _ in range(3):
            self.allocate()
        for n in range(16):
            offset = self.table * 1024 + n * 256
            self.data[offset:offset + 128] = old[n * 128:(n + 1) * 128]
            if n in (1, 11):
                struct.pack_into("<H", self.data, offset + 128, 32)
        self.root_inode = self.table * 1024 + 256
        self.file_inode = self.table * 1024 + 11 * 256
        struct.pack_into("<H", self.data, 1024 + 88, 256)
        struct.pack_into("<H", self.data, 1024 + 254, descriptor_size)
        self.put32(1024 + 96, 2 | 0x40 | 0x80 | (0x2000 if seed_feature else 0) | (0x8000 if inline else 0))
        self.put32(1024 + 100, 2 | 0x400)
        self.data[1024 + 104:1024 + 120] = bytes(range(16))
        self.data[1024 + 0x175] = 1
        self.seed = 0x1BADB002 if seed_feature else crc32c(0xFFFFFFFF, bytes(range(16)))
        self.put32(1024 + 0x270, self.seed)
        self.put32(self.gdt * 1024 + 8, self.table)
        self.data[self.gdt * 1024 + 32:self.gdt * 1024 + descriptor_size] = bytes(descriptor_size - 32)
        self.payload = bytes(range(100))
        if inline:
            self.size = len(self.payload)
            self.set_inline(self.file_inode, self.payload)
            first = self.dirent(12, NAME.encode(), 56)
            second = self.dirent(12, b"alias.bin", 40)
            self.inline_dir_inode = self.table * 1024 + 12 * 256
            self.inode(self.inline_dir_inode, 0o40755, 100, 0, links=2)
            struct.pack_into("<H", self.data, self.inline_dir_inode + 128, 32)
            self.set_inline(self.inline_dir_inode, struct.pack("<I", 2) + first + second)
            struct.pack_into("<H", self.data, self.file_inode + 26, 3)
            struct.pack_into("<H", self.data, self.root_inode + 26, 3)
            self.put32(1024 + 16, 3)
            struct.pack_into("<H", self.data, self.gdt * 1024 + 14, 3)
            self.data[self.inode_map * 1024 + 1] |= 1 << 4
            directory = self.dirent(2, b".", 12, 2) + self.dirent(2, b"..", 12, 2)
            directory += self.dirent(12, NAME.encode(), (8 + len(NAME.encode()) + 3) & ~3)
            directory += self.dirent(13, b"nested", 1012 - len(directory), 2)
            self.data[self.root_block * 1024:self.root_block * 1024 + 1012] = directory
        else:
            # Leave room for the leaf checksum dirent and terminate the preceding record.
            pos = self.root_block * 1024
            last = pos + 24
            struct.pack_into("<H", self.data, last + 4, 1024 - 24 - 12)
        struct.pack_into("<IHBBI", self.data, self.root_block * 1024 + 1012, 0, 12, 0, 0xDE, 0)
        if indexed:
            self.make_indexed_directory()
            if short_leaf:
                start = self.dir_leaves[0] * 1024
                self.data[start:start + 1012] = (self.dirent(12, b"a", 12) +
                                                self.dirent(12, NAME.encode(), 1000))
                struct.pack_into("<H", self.data, self.file_inode + 26, 2)
            if empty_leaf:
                empty = self.allocate(self.dirent(0, b"", 1024))
                struct.pack_into("<IHBBI", self.data, empty * 1024 + 1012, 0, 12, 0, 0xDE, 0)
                self.put32(self.root_inode + 4, 4096)
                self.put32(self.root_inode + 28, 8)
                self.put32(self.root_inode + 52, empty)
                # A second leaf at the end of the htree hash range.
                index = self.index_blocks[1][0] * 1024
                struct.pack_into("<H", self.data, index + 10, 2)
                struct.pack_into("<II", self.data, index + 16, 0xFFFFFFFE, 3)
                self.dir_leaves.append(empty)
        self.refresh()

    def make_indexed_directory(self):
        self.put32(1024 + 92, 0x20)
        self.put32(self.root_inode + 32, 0x1000)
        self.put32(self.root_inode + 4, 3072)
        self.put32(self.root_inode + 28, 6)
        index = self.allocate()
        leaf = self.allocate()
        self.put32(self.root_inode + 44, index)
        self.put32(self.root_inode + 48, leaf)
        start = self.root_block * 1024
        self.data[start:start + 1024] = bytes(1024)
        self.data[start:start + 24] = self.dirent(2, b".", 12, 2) + self.dirent(2, b"..", 12, 2)
        struct.pack_into("<H", self.data, start + 16, 1012)
        struct.pack_into("<IBBBB", self.data, start + 24, 0, 1, 8, 1, 0)
        struct.pack_into("<HHI", self.data, start + 32, 123, 1, 1)
        struct.pack_into("<IHBBHHI", self.data, index * 1024, 0, 1024, 0, 0, 126, 1, 2)
        self.data[leaf * 1024:leaf * 1024 + 1012] = self.dirent(12, NAME.encode(), 1012)
        struct.pack_into("<IHBBI", self.data, leaf * 1024 + 1012, 0, 12, 0, 0xDE, 0)
        self.dir_leaves = [leaf]
        self.index_blocks = [(self.root_block, 32, 123), (index, 8, 126)]

    @staticmethod
    def dirent(ino, name, length, kind=1):
        return struct.pack("<IHBB", ino, length, len(name), kind) + name + bytes(length - 8 - len(name))

    def set_inline(self, offset, payload):
        self.put32(offset + 32, 1 << 28)
        self.put32(offset + 4, len(payload))
        self.put32(offset + 28, 0)
        self.data[offset + 40:offset + 100] = payload[:60].ljust(60, b"\0")
        self.put32(offset + 160, 0xEA020000)
        extra = payload[60:]
        struct.pack_into("<BBHIII", self.data, offset + 164, 4, 7, 256 - 164 - len(extra), 0, len(extra), 0)
        self.data[offset + 180:offset + 184] = b"data"
        self.data[offset + 184:offset + 188] = bytes(4)
        self.data[offset + 256 - len(extra):offset + 256] = extra

    def inode_seed(self, ino):
        return crc32c(crc32c(self.seed, struct.pack("<I", ino)), bytes(4))

    def add_external_xattr(self):
        self.xattr_block = self.allocate(struct.pack("<III", 0xEA020000, 1, 1))
        self.put32(self.file_inode + 104, self.xattr_block)
        sectors = struct.unpack_from("<I", self.data, self.file_inode + 28)[0]
        self.put32(self.file_inode + 28, sectors + 2)
        self.refresh()

    def read(self, offset, length):
        return self.payload[offset:offset + length] if self.inline else super().read(offset, length)

    def refresh(self):
        bs = self.block_size
        gd = self.gdt * bs
        free = self.blocks - self.first - len(self.used)
        self.put32(1024 + 12, free)
        struct.pack_into("<H", self.data, gd + 12, free)
        for block in self.used:
            bit = block - self.first
            self.data[self.block_map * bs + bit // 8] |= 1 << (bit % 8)
        if hasattr(self, "xattr_block"):
            start = self.xattr_block * bs
            self.put32(start + 16, 0)
            checksum = crc32c(crc32c(self.seed, struct.pack("<Q", self.xattr_block)),
                              self.data[start:start + bs])
            self.put32(start + 16, checksum)
        for block in self.dir_leaves:
            start = block * bs
            self.put32(start + bs - 4, crc32c(self.inode_seed(2), self.data[start:start + bs - 12]))
        for block, offset, limit in self.index_blocks:
            start = block * bs
            tail = start + offset + limit * 8
            count = struct.unpack_from("<H", self.data, start + offset + 2)[0]
            checksum = crc32c(self.inode_seed(2), self.data[start:start + offset + count * 8])
            checksum = crc32c(checksum, self.data[tail:tail + 4] + bytes(4))
            self.put32(tail + 4, checksum)
        if not self.inline:
            for block in self.extent_blocks:
                start = block * bs
                capacity = struct.unpack_from("<H", self.data, start + 4)[0]
                tail = start + 12 + 12 * capacity
                self.put32(tail, crc32c(self.inode_seed(12), self.data[start:tail]))
        for ino in ((2, 12, 13) if self.inline else (2, 12)):
            start = self.table * bs + (ino - 1) * 256
            struct.pack_into("<H", self.data, start + 124, 0)
            has_checksum_hi = struct.unpack_from("<H", self.data, start + 128)[0] >= 4
            if has_checksum_hi:
                struct.pack_into("<H", self.data, start + 130, 0)
            checksum = crc32c(self.inode_seed(ino), self.data[start:start + 256])
            struct.pack_into("<H", self.data, start + 124, checksum & 0xFFFF)
            if has_checksum_hi:
                struct.pack_into("<H", self.data, start + 130, checksum >> 16)
        for block, lo, hi, size in ((self.block_map, 24, 56, 128), (self.inode_map, 26, 58, 2)):
            checksum = crc32c(self.seed, self.data[block * bs:block * bs + size])
            struct.pack_into("<H", self.data, gd + lo, checksum & 0xFFFF)
            struct.pack_into("<H", self.data, gd + hi, checksum >> 16)
        struct.pack_into("<H", self.data, gd + 30, 0)
        checksum = crc32c(crc32c(self.seed, bytes(4)), self.data[gd:gd + self.descriptor_size])
        struct.pack_into("<H", self.data, gd + 30, checksum & 0xFFFF)
        self.put32(2044, crc32c(0xFFFFFFFF, self.data[1024:2044]))
