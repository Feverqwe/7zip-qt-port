"""Checksummed ext htrees with reachable and stale allocated directory blocks."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import struct
from tests.formats.ext.support.ext_modern_fixture import ModernVolume


class HtreeVolume(ModernVolume):
    def __init__(self, layout="orphan"):
        super().__init__(indexed=True)
        bs = self.block_size
        index = self.index_blocks[1][0]
        self.index_offset = index * bs
        self.next_index_offset = self.index_offset
        extra = self.allocate()
        self.put32(self.root_inode + 52, extra)
        self.put32(self.root_inode + 4, 4 * bs)
        self.put32(self.root_inode + 28, 8)
        if layout.startswith("orphan"):
            start = extra * bs
            self.data[start:start + 1012] = self.dirent(12, b"not-indexed.bin", 1012)
            struct.pack_into("<IHBBI", self.data, start + 1012, 0, 12, 0, 0xDE, 0)
            self.dir_leaves.append(extra)
        elif layout == "depth2":
            struct.pack_into("<IHBBHHI", self.data, extra * bs, 0, bs, 0, 0, 126, 1, 1)
            self.put32(self.root_block * bs + 36, 3)
            self.data[self.root_block * bs + 30] = 2
            self.put32(1024 + 96, struct.unpack_from("<I", self.data, 1024 + 96)[0] | 0x4000)
            self.index_blocks.append((extra, 8, 126))
            self.next_index_offset = extra * bs
        elif layout in ("next-index", "collision", "duplicate"):
            leaf = self.allocate()
            self.put32(self.root_inode + 56, leaf)
            self.put32(self.root_inode + 4, 5 * bs)
            self.put32(self.root_inode + 28, 10)
            struct.pack_into("<H", self.data, self.root_block * bs + 34, 2)
            boundary = 0xFFFFFFFF if layout == "collision" else 0xFFFFFFFE
            struct.pack_into("<II", self.data, self.root_block * bs + 40,
                             boundary, 1 if layout == "duplicate" else 3)
            struct.pack_into("<IHBBHHI", self.data, extra * bs, 0, bs, 0, 0, 126, 1, 4)
            self.index_blocks.append((extra, 8, 126))
            self.next_index_offset = extra * bs
            self.data[leaf * bs:leaf * bs + 1012] = self.dirent(0, b"", 1012)
            struct.pack_into("<IHBBI", self.data, leaf * bs + 1012, 0, 12, 0, 0xDE, 0)
            self.dir_leaves.append(leaf)
        elif layout in ("legacy", "legacy-extension"):
            self.put32(1024 + 100, 2)
            struct.pack_into("<H", self.data, index * bs + 8, 127)
            if layout == "legacy-extension":
                self.data[self.root_block * bs + 30] = 1
                self.data[self.root_block * bs + 29] = 12
                self.data[self.root_block * bs + 32:self.root_block * bs + 36] = bytes(4)
                struct.pack_into("<HHI", self.data, self.root_block * bs + 36, 123, 1, 1)
            else:
                struct.pack_into("<H", self.data, self.root_block * bs + 32, 124)
        elif layout == "fallback":
            self.data[self.root_block * bs + 28] = 255
            self.put32(self.root_inode + 52, 0)
            self.put32(self.root_inode + 28, 6)
            self.used.discard(extra)
        elif layout == "masked":
            self.put32(self.root_block * bs + 36, 0xF0000001)
            self.put32(index * bs + 12, 0xA0000002)
        elif layout == "hole":
            self.put32(index * bs + 12, 3)
            self.put32(self.root_inode + 52, 0)
        else:
            raise ValueError(layout)
        self.refresh()
        if layout == "orphan-corrupt":
            self.data[extra * bs + 1020] ^= 1
