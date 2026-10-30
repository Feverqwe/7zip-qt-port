"""Bounded ext plaintext symlinks with independent inode/storage lengths."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import struct
from tests.formats.ext.support.ext_modern_fixture import ModernVolume, crc32c
from tests.formats.ext.support.ext_fixture import NAME


def encrypted_link_size(image, case, size):
    data = bytearray(image)
    block_size = 1024 << struct.unpack_from("<I", data, 1024 + 24)[0]
    inode_size = struct.unpack_from("<H", data, 1024 + 88)[0]
    table = struct.unpack_from("<I", data, block_size + 8)[0]
    offset = table * block_size + (case["symlink_inode"] - 1) * inode_size
    inode = bytearray(data[offset:offset + inode_size])
    struct.pack_into("<I", inode, 4, size)
    inode[124:126] = bytes(2)
    inode[130:132] = bytes(2)
    seed = crc32c(0xFFFFFFFF, data[1024 + 104:1024 + 120])
    seed = crc32c(seed, struct.pack("<I", case["symlink_inode"]))
    seed = crc32c(seed, inode[100:104])
    checksum = crc32c(seed, inode)
    struct.pack_into("<H", inode, 124, checksum & 0xFFFF)
    struct.pack_into("<H", inode, 130, checksum >> 16)
    data[offset:offset + inode_size] = inode
    return bytes(data)


def encrypted_link_payload_length(image, case, length):
    data = bytearray(image)
    bs = 1024 << struct.unpack_from("<I", data, 1024 + 24)[0]
    inode_size = struct.unpack_from("<H", data, 1024 + 88)[0]
    table = struct.unpack_from("<I", data, bs + 8)[0]
    inode = table * bs + (case["symlink_inode"] - 1) * inode_size
    # The native fixture stores these ciphertexts in one initialized extent.
    assert struct.unpack_from("<H", data, inode + 46)[0] == 0
    assert struct.unpack_from("<I", data, inode + 52)[0] == 0
    block = struct.unpack_from("<I", data, inode + 60)[0]
    block |= struct.unpack_from("<H", data, inode + 58)[0] << 32
    struct.pack_into("<H", data, block * bs, length)
    return bytes(data)


def encrypted_link_indirect(image, case):
    """Keep native ciphertext/context while changing only its storage map."""
    data = bytearray(image)
    bs = 1024 << struct.unpack_from("<I", data, 1024 + 24)[0]
    inode_size = struct.unpack_from("<H", data, 1024 + 88)[0]
    table = struct.unpack_from("<I", data, bs + 8)[0]
    inode = table * bs + (case["symlink_inode"] - 1) * inode_size
    assert struct.unpack_from("<H", data, inode + 42)[0] == 1
    assert struct.unpack_from("<H", data, inode + 46)[0] == 0
    assert struct.unpack_from("<I", data, inode + 52)[0] == 0
    assert struct.unpack_from("<H", data, inode + 56)[0] == 1
    assert struct.unpack_from("<H", data, inode + 58)[0] == 0
    physical = struct.unpack_from("<I", data, inode + 60)[0]
    size = struct.unpack_from("<I", data, inode + 4)[0]
    flags = struct.unpack_from("<I", data, inode + 32)[0]
    data[inode + 40:inode + 100] = bytes(60)
    struct.pack_into("<I", data, inode + 40, physical)
    struct.pack_into("<I", data, inode + 32, flags & ~(1 << 19))
    return encrypted_link_size(data, case, size)


class ReadlinkVolume(ModernVolume):
    def __init__(self, layout="block-tail"):
        neighbors = layout.startswith("isolated-")
        layout = layout.removeprefix("isolated-")
        classic = layout.startswith("classic-")
        layout = layout.removeprefix("classic-")
        super().__init__()
        bs = self.block_size
        self.data[self.root_block * bs + 31] = 7
        struct.pack_into("<H", self.data, self.file_inode, 0o120777)
        self.first_data_offset = 0
        self.second_data_offset = 0
        if layout.startswith("inline"):
            self.put32(1024 + 96, struct.unpack_from("<I", self.data, 1024 + 96)[0] | 0x8000)
            payload = b"inline-visible\0private" if layout == "inline-nul" else b"x" * 63 + b"y" * 32
            if layout == "inline-empty":
                payload = b""
            self.set_inline(self.file_inode, payload)
            self.expected = payload.split(b"\0", 1)[0]
            if layout == "inline-oversize":
                self.put32(self.file_inode + 4, 1000)
        else:
            payload = b"visible\0hidden bytes" if layout == "block-nul" else b"t" * bs
            first = self.allocate(payload)
            second = self.allocate(b"must not be returned past the first block")
            self.first_data_offset = first * bs
            self.second_data_offset = second * bs
            self.put32(self.file_inode + 4, len(payload) if layout == "block-nul" else bs + 39)
            self.put32(self.file_inode + 28, 2 * bs // 512)
            self.put32(self.file_inode + 32, 1 << 19)
            self.data[self.file_inode + 40:self.file_inode + 100] = self.extent_header(
                0, [(0, 1, first), (1, 0x8001 if layout == "unwritten-tail" else 1, second)], 4)
            if classic:
                self.put32(self.file_inode + 32, 0)
                self.data[self.file_inode + 40:self.file_inode + 100] = bytes(60)
                self.put32(self.file_inode + 40, first)
                self.put32(self.file_inode + 44, second)
            self.expected = payload.split(b"\0", 1)[0][:bs - 1]
            if layout == "large-size":
                self.put32(self.file_inode + 4, 16 * bs + 3)
            if layout == "hole":
                self.put32(self.file_inode + 32, 0)
                self.data[self.file_inode + 40:self.file_inode + 100] = bytes(60)
        if neighbors:
            self.add_neighbors()
        self.refresh()
        self.size = len(self.expected)

    def add_neighbors(self):
        bs = self.block_size
        self.healthy = b"readable sibling\0with exact bytes\xff"
        block = self.allocate(self.healthy)
        self.extra_inodes = (13, 14)
        for ino, mode, payload, sectors in (
                (13, 0o100644, self.healthy, bs // 512),
                (14, 0o120777, b"healthy.bin", 0)):
            start = self.table * bs + (ino - 1) * 256
            self.inode(start, mode, len(payload), sectors)
            struct.pack_into("<H", self.data, start + 128, 32)
            if ino == 13:
                self.put32(start + 40, block)
            else:
                self.data[start + 40:start + 40 + len(payload)] = payload
            bit = ino - 1
            self.data[self.inode_map * bs + bit // 8] |= 1 << (bit % 8)
        free = struct.unpack_from("<I", self.data, 1024 + 16)[0] - 2
        self.put32(1024 + 16, free)
        struct.pack_into("<H", self.data, self.gdt * bs + 14, free)
        records = self.dirent(2, b".", 12, 2) + self.dirent(2, b"..", 12, 2)
        name = NAME.encode()
        records += self.dirent(12, name, (8 + len(name) + 3) & ~3, 7)
        records += self.dirent(13, b"healthy.bin", 20)
        records += self.dirent(14, b"later-link", bs - 12 - len(records), 7)
        self.data[self.root_block * bs:self.root_block * bs + bs - 12] = records
        self.files = {"healthy.bin": self.healthy, "later-link": b"healthy.bin"}

    def read(self, offset, length):
        return self.expected[offset:offset + length]
