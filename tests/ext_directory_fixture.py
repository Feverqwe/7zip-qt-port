"""Checksummed directories with mapped, missing and unwritten logical blocks."""
import struct
from ext_modern_fixture import ModernVolume
from ext_fixture import Volume, NAME


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
            self.put32(self.root_inode + 48, tail)
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
