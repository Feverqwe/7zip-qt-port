"""Small ext volumes with sparse logical files and bounded extent trees."""
import base64
import json
import struct
import sys
import zlib

NAME = "данные 空.bin"


class Volume:
    def __init__(self, layout="double-hole", block_bits=10, full_group=False):
        self.block_bits, self.block_size = block_bits, 1 << block_bits
        bs = self.block_size
        self.inode_count = max(16, bs // 128)
        self.first = int(block_bits == 10)
        self.blocks = 1025 if full_group else 1024 if block_bits == 10 else 128
        self.data = bytearray(self.blocks * bs)
        self.gdt = self.first + 1
        self.block_map, self.inode_map, self.table = self.gdt + 1, self.gdt + 2, self.gdt + 3
        self.root_block = self.table + (self.inode_count * 128 + bs - 1) // bs
        self.next_block = self.root_block + 1
        self.used = set(range(self.first, self.next_block))
        self.content = {}
        self.extent_blocks = []
        self.file_inode = self.table * bs + 11 * 128
        self.root_inode = self.table * bs + 128
        self.indirect = {}
        self.file_block = bytearray(60)
        self.flags = 0
        self.link_inode = None
        refs = bs // 4
        start_double, start_triple = 12 + refs, 12 + refs + refs * refs
        logical = {"direct": 2, "single": 14, "double-hole": start_double + 5,
                   "double-subtree": start_double + refs + 3,
                   "triple": start_triple + 2 * refs + 3,
                   "triple-hole": start_triple + 3}.get(layout, 2)
        if layout == "sparse-short":
            self.size = 17
        elif layout == "triple-hole":
            self.size = logical * bs + 17
        elif layout.startswith("extent"):
            self.flags = 1 << 19
            self.size = 7 * bs + 4
            first = self.allocate(b"A" * bs)
            secret = self.allocate(b"unwritten secret".ljust(bs, b"!"))
            self.allocate(b"unwritten secret".ljust(bs, b"!"))
            tail = self.allocate(b"tail")
            self.content = {2: b"A" * bs, 7: b"tail"}
            leaf = self.extent_header(0, [(2, 1, first), (4, 0x8002, secret), (7, 1, tail)], 4)
            depth = 1 if layout == "extent-tree" else 5 if layout == "extent-depth5" else 0
            if layout == "extent-two-branches":
                left = self.allocate(self.extent_header(0, [(2, 1, first)], 4))
                right = self.allocate(self.extent_header(0, [(4, 0x8002, secret), (7, 1, tail)], 4))
                self.extent_blocks = [left, right]
                leaf = self.extent_header(1, [(2, left), (4, right)], 4)
            for level in range(depth):
                block = self.allocate(leaf)
                self.extent_blocks.append(block)
                leaf = self.extent_header(level + 1, [(2, block)], 4)
            self.file_block[:] = leaf
        else:
            self.size = logical * bs + 4
            self.add_data(0, b"head")
            self.add_data(logical, b"tail")
        if layout == "fastlink-xattr":
            acl = self.allocate(struct.pack("<III", 0xEA020000, 1, 1))
            self.link_inode = self.table * bs + 12 * 128
            self.inode(self.link_inode, 0o120777, len(NAME.encode()), bs // 512)
            target = NAME.encode()
            self.data[self.link_inode + 40:self.link_inode + 40 + len(target)] = target
            self.put32(self.link_inode + 104, acl)
        self.inode(self.file_inode, 0o100644, self.size, (len(self.used) - (self.root_block - self.first + 1)
                                                       - int(self.link_inode is not None)) * bs // 512)
        self.data[self.file_inode + 40:self.file_inode + 100] = self.file_block
        self.put32(self.file_inode + 32, self.flags)
        self.inode(self.root_inode, 0o40755, bs, bs // 512, links=2)
        self.put32(self.root_inode + 40, self.root_block)
        self.directory()
        self.superblock()

    def put32(self, offset, value):
        struct.pack_into("<I", self.data, offset, value)

    def allocate(self, contents=b""):
        block = self.next_block
        assert block < self.blocks
        self.next_block += 1
        self.used.add(block)
        self.data[block * self.block_size:block * self.block_size + len(contents)] = contents
        return block

    def inode(self, offset, mode, size, sectors, links=1):
        struct.pack_into("<HHI", self.data, offset, mode, 1234, size & 0xFFFFFFFF)
        struct.pack_into("<HHII", self.data, offset + 24, 5678, links, sectors, 0)
        for field in (8, 12, 16):
            self.put32(offset + field, 1700000000)
        self.put32(offset + 108, size >> 32)

    def add_data(self, logical, content):
        refs = self.block_size // 4
        if logical < 12:
            offsets = [logical]
        elif logical < 12 + refs:
            offsets = [12, logical - 12]
        elif logical < 12 + refs + refs * refs:
            n = logical - 12 - refs
            offsets = [13, n // refs, n % refs]
        else:
            n = logical - 12 - refs - refs * refs
            offsets = [14, n // (refs * refs), n // refs % refs, n % refs]
        buffer, base = self.file_block, 0
        for depth, position in enumerate(offsets[:-1]):
            block = struct.unpack_from("<I", buffer, base + 4 * position)[0]
            if not block:
                block = self.allocate()
                struct.pack_into("<I", buffer, base + 4 * position, block)
                self.indirect[tuple(offsets[:depth + 1])] = block
            buffer, base = self.data, block * self.block_size
        block = self.allocate(content)
        struct.pack_into("<I", buffer, base + 4 * offsets[-1], block)
        self.content[logical] = content

    @staticmethod
    def extent_header(depth, entries, maximum):
        data = bytearray(60)
        struct.pack_into("<HHHHI", data, 0, 0xF30A, len(entries), maximum, depth, 0)
        for i, fields in enumerate(entries):
            if depth:
                logical, child = fields
                struct.pack_into("<IIHH", data, 12 + i * 12, logical, child, 0, 0)
            else:
                logical, length, physical = fields
                struct.pack_into("<IHHI", data, 12 + i * 12, logical, length, 0, physical)
        return data

    def directory(self):
        records = [(2, b".", 2), (2, b"..", 2), (12, NAME.encode(), 1)]
        if self.link_inode is not None:
            records.append((13, b"link", 7))
        pos = self.root_block * self.block_size
        end = pos + self.block_size
        for i, (ino, name, kind) in enumerate(records):
            length = ((8 + len(name) + 3) // 4) * 4 if i + 1 < len(records) else end - pos
            struct.pack_into("<IHBB", self.data, pos, ino, length, len(name), kind)
            self.data[pos + 8:pos + 8 + len(name)] = name
            pos += length

    def superblock(self):
        allocated = set(range(1, 11)) | {12} | ({13} if self.link_inode is not None else set())
        s = 1024
        for offset, value in ((0, self.inode_count), (4, self.blocks), (12, self.blocks - self.first - len(self.used)),
                              (16, self.inode_count - len(allocated)), (20, self.first),
                              (24, self.block_bits - 10), (28, self.block_bits - 10),
                              (32, 1024), (36, 1024), (40, self.inode_count), (76, 1), (84, 11),
                              (92, 8 if self.link_inode is not None else 0),
                              (96, 2 | ((1 << 6) if self.flags else 0)), (100, 2)):
            self.put32(s + offset, value)
        struct.pack_into("<H", self.data, s + 56, 0xEF53)
        struct.pack_into("<H", self.data, s + 58, 1)
        struct.pack_into("<H", self.data, s + 88, 128)
        struct.pack_into("<IIIHHH", self.data, self.gdt * self.block_size,
                         self.block_map, self.inode_map, self.table,
                         self.blocks - self.first - len(self.used), self.inode_count - len(allocated), 1)
        for block in self.used:
            bit = block - self.first
            self.data[self.block_map * self.block_size + bit // 8] |= 1 << (bit % 8)
        for ino in allocated:
            bit = ino - 1
            self.data[self.inode_map * self.block_size + bit // 8] |= 1 << (bit % 8)
        for bit in range(self.blocks - self.first, self.block_size * 8):
            self.data[self.block_map * self.block_size + bit // 8] |= 1 << (bit % 8)
        for bit in range(self.inode_count, self.block_size * 8):
            self.data[self.inode_map * self.block_size + bit // 8] |= 1 << (bit % 8)

    def read(self, offset, length):
        length = min(length, max(0, self.size - offset))
        result = bytearray(length)
        for logical, contents in self.content.items():
            start = logical * self.block_size
            left, right = max(start, offset), min(start + len(contents), offset + length)
            if right > left:
                result[left - offset:right - offset] = contents[left - start:right - start]
        return bytes(result)


if __name__ == "__main__":
    layout = sys.argv[1] if len(sys.argv) > 1 else "double-hole"
    if layout in ("directory-stream", "directory-corrupt"):
        from ext_directory_fixture import DirectoryVolume
        v = DirectoryVolume(fault="checksum" if layout == "directory-corrupt" else None)
    elif layout.startswith("xattrs"):
        from ext_xattr_fixture import XattrVolume
        v = XattrVolume(layout)
    elif layout.startswith("block-"):
        from ext_block_fixture import BlockVolume
        v = BlockVolume(mapping="descriptor-overlap") if layout == "block-overlap" else \
            BlockVolume(target=layout.removeprefix("block-"))
    elif layout in ("verity-salted", "verity-corrupt"):
        from ext_verity_fixture import VerityVolume
        v = VerityVolume("sha512", b"salt")
        if layout == "verity-corrupt":
            v.data[v.metadata_physical * v.block_size] ^= 1
    elif layout.startswith("modern-inline") or layout.startswith("modern-time"):
        from ext_modern_fixture import ModernVolume
        v = ModernVolume(inline=layout.startswith("modern-inline"))
        if layout.startswith("modern-time"):
            struct.pack_into("<H", v.data, v.file_inode + 128, 0 if layout == "modern-time-zero" else 12)
            v.put32(v.file_inode + 8, 0)
            v.put32(v.file_inode + 16, 0)
            v.put32(v.file_inode + 136, (123456789 << 2) | 1)
            # Outside the partial extension; the zero-size case uses Linux's
            # initialized standard extension instead.
            v.put32(v.file_inode + 140, 1)
            if layout == "modern-time-zero":
                v.put32(v.file_inode + 144, 0)
                v.put32(v.file_inode + 148, (234567890 << 2) | 2)
            v.refresh()
        if layout == "modern-inline-replacement":
            v.payload = bytes(value ^ 0xFF for value in v.payload)
            v.set_inline(v.file_inode, v.payload)
            v.refresh()
    else:
        v = Volume(layout)
    if "--bundle" in sys.argv:
        samples = [{"offset": pos, "bytes": v.read(pos, 37).hex()}
                   for pos in (0, max(0, v.size - 17), 1010)]
        print(json.dumps({"size": len(v.data), "image": base64.b64encode(zlib.compress(v.data)).decode(),
                          "file_size": v.size, "name": getattr(v, "name", NAME), "samples": samples,
                          "files": {name: value.hex() for name, value in getattr(v, "files", {}).items()},
                          "ea_data_offset": getattr(v, "ea_data_offset", 0),
                          "directory_head_offset": getattr(v, "directory_head_offset", 0),
                          "directory_tail_offset": getattr(v, "directory_tail_offset", 0),
                          "block_bitmap_offset": getattr(v, "block_map", 0) * getattr(v, "block_size", 0)}))
    else:
        sys.stdout.buffer.write(v.data)
