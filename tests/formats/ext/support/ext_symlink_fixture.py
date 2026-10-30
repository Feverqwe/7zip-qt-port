"""Fast/slow symlink images with ext4 cluster and EA-inode geometry."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import struct
from tests.formats.ext.support.ext_fixture import Volume, NAME


class SymlinkVolume(Volume):
    def __init__(self, case="bigalloc"):
        super().__init__("fastlink-xattr")
        bs = self.block_size
        self.target = NAME
        if case.startswith("ea-slow"):
            self.target = "long-" + "a" * 60
            block = self.allocate(self.target.encode())
            self.inode(self.link_inode, 0o120777, len(self.target), 2 * bs // 512)
            self.data[self.link_inode + 40:self.link_inode + 100] = bytes(60)
            self.put32(self.link_inode + 40, self.table if case.endswith("alias") else block)
            self.superblock()
        if case.startswith("ea-"):
            self.put32(1024 + 96, 2 | 0x400 | (0x40 if case == "ea-fast-extents" else 0))
            if case == "ea-fast-extents":
                self.put32(self.link_inode + 32, 1 << 19)
        elif case.startswith("bigalloc"):
            # Convert the directory and regular file to extents; bigalloc uses
            # one 4 KiB cluster per external xattr even with 1 KiB blocks.
            head, tail = struct.unpack_from("<I", self.file_block)[0], struct.unpack_from("<I", self.file_block, 8)[0]
            self.data[self.file_inode + 40:self.file_inode + 100] = self.extent_header(0, [(0, 1, head), (2, 1, tail)], 4)
            self.data[self.root_inode + 40:self.root_inode + 100] = self.extent_header(0, [(0, 1, self.root_block)], 4)
            self.put32(self.file_inode + 32, 1 << 19)
            self.put32(self.root_inode + 32, 1 << 19)
            self.put32(1024 + 96, 2 | 0x40)
            self.put32(1024 + 100, 2 | 0x200 | (8 if case.endswith("huge") else 0))
            self.put32(1024 + 28, 2)
            self.put32(1024 + 36, 256)
            # The allocation bitmap addresses clusters rather than blocks.
            bitmap = bytearray(bs)
            for cluster in {(block - self.first) // 4 for block in self.used}:
                bitmap[cluster // 8] |= 1 << (cluster % 8)
            for cluster in range(256, bs * 8):
                bitmap[cluster // 8] |= 1 << (cluster % 8)
            self.data[self.block_map * bs:(self.block_map + 1) * bs] = bitmap
            self.put32(self.link_inode + 28, 4 if case.endswith("huge") else 8)
            if case == "bigalloc-classic":
                self.data[self.file_inode + 40:self.file_inode + 100] = self.file_block
                self.data[self.root_inode + 40:self.root_inode + 100] = bytes(60)
                self.put32(self.root_inode + 40, self.root_block)
                self.put32(self.file_inode + 32, 0)
                self.put32(self.root_inode + 32, 0)
                self.put32(1024 + 96, 2)
            if case.endswith("huge"):
                self.put32(self.link_inode + 32, 1 << 18)
        elif case == "append":
            self.put32(self.link_inode + 32, 0x20)
        elif case == "immutable":
            self.put32(self.link_inode + 32, 0x10)
        elif case == "embedded-nul":
            self.data[self.link_inode + 41] = 0
        elif case == "unterminated":
            self.data[self.link_inode + 40 + len(self.target.encode())] = ord("!")
        elif case == "empty":
            self.put32(self.link_inode + 4, 0)
        else:
            raise ValueError(case)
