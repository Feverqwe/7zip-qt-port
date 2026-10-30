"""Deterministic fs-verity images; hashlib produces the independent oracle tree."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import hashlib
import struct
from tests.formats.ext.support.ext_modern_fixture import ModernVolume


class VerityVolume(ModernVolume):
    def __init__(self, algorithm="sha256", salt=b"", size=35 * 1024 + 17, block_bits=10, signer=None):
        super().__init__()
        self.algorithm = algorithm
        self.salt = salt
        self.verity_block_size = 1 << block_bits
        leaf = self.extent_blocks[0] * 1024
        first = struct.unpack_from("<I", self.data, leaf + 20)[0]
        unwritten = struct.unpack_from("<I", self.data, leaf + 32)[0]
        tail = struct.unpack_from("<I", self.data, leaf + 44)[0]
        self.size = size
        self.content = {}
        entries = []
        if size > 1024:
            self.content = {2: b"A" * 1024, 35: b"tail"}
            entries = [(2, 1, first), (9, 0x8002, unwritten), (35, 1, tail)]
        elif size:
            self.content = {0: bytes(range(100))[:size]}
            first = self.allocate(self.content[0])
            entries = [(0, 1, first)]
        self.first_data = first * 1024
        pad = 64 if algorithm == "sha256" else 128
        prefix = salt.ljust((len(salt) + pad - 1) // pad * pad, b"\0")
        digest = lambda value: hashlib.new(algorithm, prefix + value).digest()
        bs = self.verity_block_size
        hashes = [digest(self.read(pos, bs).ljust(bs, b"\0")) for pos in range(0, size, bs)]
        levels = []
        while len(hashes) > 1:
            packed = b"".join(hashes)
            level = [packed[pos:pos + bs].ljust(bs, b"\0") for pos in range(0, len(packed), bs)]
            levels.append(b"".join(level))
            hashes = [digest(block) for block in level]
        self.tree = b"".join(reversed(levels))
        self.root_hash = hashes[0] if hashes else bytes(hashlib.new(algorithm).digest_size)
        descriptor = bytearray(256)
        struct.pack_into("<BBBBIQ", descriptor, 0, 1, 1 if algorithm == "sha256" else 2,
                         block_bits, len(salt), 0, size)
        descriptor[16:16 + len(self.root_hash)] = self.root_hash
        descriptor[80:80 + len(salt)] = salt
        self.file_digest = hashlib.new(algorithm, descriptor).digest()
        signature = signer(1 if algorithm == "sha256" else 2, self.file_digest) if signer else b""
        struct.pack_into("<I", descriptor, 4, len(signature))
        self.metadata_logical = (size + 65535) // 65536 * 64
        descriptor += signature
        metadata = self.tree + descriptor + bytes((-len(descriptor) - 4) % 1024) + struct.pack("<I", len(descriptor))
        blocks = len(metadata) // 1024
        self.metadata_physical = self.allocate(metadata[:1024])
        for pos in range(1024, len(metadata), 1024):
            self.allocate(metadata[pos:pos + 1024])
        self.descriptor_physical = self.metadata_physical * 1024 + len(self.tree)
        self.signature_physical = self.descriptor_physical + 256
        self.signature_size = len(signature)
        entries.append((self.metadata_logical, blocks, self.metadata_physical))
        self.data[self.file_inode + 40:self.file_inode + 100] = self.extent_header(0, entries, 4)
        self.put32(self.file_inode + 4, size)
        self.put32(self.file_inode + 28, 2 * (sum((length - 0x8000 if length > 0x8000 else length)
                                               for _, length, _ in entries)))
        self.put32(self.file_inode + 32, (1 << 19) | (1 << 20))
        self.put32(1024 + 100, 2 | 0x400 | 0x8000)
        self.refresh()
