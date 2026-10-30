"""Disposable ext images whose file references alias filesystem metadata."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import struct
from tests.formats.ext.support.ext_fixture import Volume


class BlockVolume(Volume):
    def __init__(self, mapping="direct", target="table"):
        super().__init__("single" if mapping.startswith("indirect") else
                         "extent-tree" if mapping == "extent-index" else
                         "extent" if mapping.startswith("extent") else "direct")
        bs = self.block_size
        if mapping == "descriptor-overlap":
            self.put32(self.gdt * bs, self.inode_map)
            return
        targets = {"super": self.first, "gdt": self.gdt, "block-map": self.block_map,
                   "inode-map": self.inode_map, "table": self.table}
        if target.startswith("journal"):
            self.journal_payload = b"private journal bytes".ljust(bs, b"!")
            journal = self.allocate(self.journal_payload)
            journal_inode = self.table * bs + 7 * 128
            self.inode(journal_inode, 0o100600, bs, bs // 512)
            self.put32(journal_inode + 40, journal)
            self.superblock()
            self.put32(1024 + 92, 4)
            self.put32(1024 + 224, 8)
            if target == "journal-owner":
                return
            targets[target] = journal
        block = targets[target]
        if mapping == "direct":
            self.put32(self.file_inode + 40, block)
        elif mapping == "unused-direct":
            self.put32(self.file_inode + 40 + 11 * 4, block)
        elif mapping == "indirect-index":
            self.put32(self.file_inode + 40 + 12 * 4, block)
        elif mapping == "indirect-data":
            self.put32(self.indirect[(12,)] * bs, block)
        elif mapping == "extent-index":
            self.put32(self.file_inode + 40 + 16, block)
        elif mapping.startswith("extent"):
            struct.pack_into("<IHHI", self.data, self.file_inode + 40 + 12,
                             2, 0x8001 if mapping == "extent-unwritten" else 1, 0, block)
        elif mapping == "xattr":
            self.put32(self.file_inode + 104, block)
        else:
            raise ValueError(mapping)


class JournalVolume(Volume):
    """A sparse journal with independent user data and bounded storage."""
    def __init__(self, mapping="single", fault=None):
        super().__init__(mapping)
        bs = self.block_size
        self.journal_size = self.size
        offsets = (2 * bs, 4 * bs, self.size - 17) if self.extent_blocks else (0, 12 * bs, self.size - 17)
        self.journal_samples = [{"offset": offset, "bytes": self.read(offset, 37).hex()}
                                for offset in offsets]
        journal_inode = self.table * bs + 7 * 128
        self.data[journal_inode:journal_inode + 128] = self.data[self.file_inode:self.file_inode + 128]
        self.journal_metadata = (self.extent_blocks[0] if self.extent_blocks else
                                 next(iter(self.indirect.values()))) * bs
        self.journal_tail = (struct.unpack_from("<I", self.data, self.journal_metadata + 20)[0]
                             if self.extent_blocks else self.next_block - 1)
        # Keep the user file independent of journal-owned data.
        self.content = {0: b"independent user file\n"}
        self.size = len(self.content[0])
        self.data[self.file_inode:self.file_inode + 128] = bytes(128)
        self.inode(self.file_inode, 0o100600, self.size, bs // 512)
        self.put32(self.file_inode + 40, self.allocate(self.content[0]))
        if fault == "alias":
            self.put32(self.file_inode + 40, self.journal_tail)
        elif fault == "data-bound":
            self.put32(journal_inode + 40, self.blocks)
        elif fault == "index-bound":
            self.put32(journal_inode + 40 + 12 * 4, self.blocks)
        elif fault == "overlap":
            self.put32(journal_inode + 40, self.table)
        self.superblock()
        self.put32(1024 + 92, 4)
        self.put32(1024 + 224, 8)
