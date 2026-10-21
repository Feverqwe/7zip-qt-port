"""Bounded ext plaintext symlinks with independent inode/storage lengths."""
import struct
from ext_modern_fixture import ModernVolume, crc32c


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


class ReadlinkVolume(ModernVolume):
    def __init__(self, layout="block-tail"):
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
            self.expected = payload.split(b"\0", 1)[0][:bs - 1]
            if layout == "large-size":
                self.put32(self.file_inode + 4, 16 * bs + 3)
            if layout == "hole":
                self.put32(self.file_inode + 32, 0)
                self.data[self.file_inode + 40:self.file_inode + 100] = bytes(60)
        self.refresh()
        self.size = len(self.expected)

    def read(self, offset, length):
        return self.expected[offset:offset + length]
