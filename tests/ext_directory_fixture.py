"""Checksummed directories with mapped, missing and unwritten logical blocks."""
import struct
from ext_modern_fixture import ModernVolume


class DirectoryVolume(ModernVolume):
    def __init__(self, mapping="extent", fault=None, encoding=None, encoding_flags=0):
        super().__init__()
        bs = self.block_size
        leaf = bytearray(bs)
        leaf[:bs - 12] = self.dirent(12, b"after-hole.bin", bs - 12)
        struct.pack_into("<IHBBI", leaf, bs - 12, 0, 12, 0, 0xDE, 0)
        tail = self.allocate(leaf)
        self.dir_leaves.append(tail)
        self.put32(self.root_inode + 4, 3 * bs)
        self.put32(self.root_inode + 28, 2 * bs // 512)
        struct.pack_into("<H", self.data, self.file_inode + 26, 2)
        if mapping == "indirect":
            self.data[self.root_inode + 40:self.root_inode + 100] = bytes(60)
            self.put32(self.root_inode + 40, self.root_block)
            self.put32(self.root_inode + 48, tail)
        else:
            entries = [(0, 1, self.root_block), (2, 1, tail)]
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
