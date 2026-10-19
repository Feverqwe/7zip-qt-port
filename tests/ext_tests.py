"""Production ext sparse reads, extent boundaries and malformed directories."""
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib
from ext_fixture import Volume, NAME
from ext_modern_fixture import ModernVolume

RUNTIME = Path(sys.argv.pop(1))
MODULE = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""


class ExtTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-ext-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        core = "7z.dll" if sys.platform == "win32" else "7z.so"
        for name in ("7z" + SUFFIX, "7zz" + SUFFIX, core):
            shutil.copy2(RUNTIME / name, cls.runtime / name)
        (cls.runtime / "Formats").mkdir()
        shutil.copy2(MODULE, cls.runtime / "Formats" / MODULE.name)

    def run_image(self, v, engine="7z", command="x", name=NAME):
        with tempfile.TemporaryDirectory(dir=self.runtime) as temporary:
            path = Path(temporary) / "образ ext 空.img"
            path.write_bytes(v.data)
            args = [str(self.runtime / (engine + SUFFIX)), command, "-tExt", "-sccUTF-8", "-bsp0"]
            args += ["-so", "-bso0", "-bse2"] if command == "x" else ["-slt"]
            args += ["--", str(path)]
            if name is not None:
                args += [name]
            result = subprocess.run(args, stdin=subprocess.DEVNULL, capture_output=True, timeout=10)
            self.assertEqual(path.read_bytes(), v.data)
            return result

    def test_sparse_indirect_and_extent_reads(self):
        for engine in ("7z", "7zz"):
            for layout in ("direct", "single", "double-hole", "double-subtree", "sparse-short",
                           "extent", "extent-tree", "extent-depth5", "extent-two-branches"):
                with self.subTest(engine=engine, layout=layout):
                    v = Volume(layout)
                    result = self.run_image(v, engine)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, v.read(0, v.size))

    def test_modern_metadata_and_full_inline_payload(self):
        # Linux C readers live in the dynamic format module; standalone 7zz
        # retains the independent 7-Zip LGPL classic reader.
        for engine in ("7z",):
            for inline, seed, descriptor in ((False, False, 64), (False, True, 128),
                                              (True, False, 64), (True, True, 128)):
                with self.subTest(engine=engine, inline=inline, seed=seed, descriptor=descriptor):
                    volume = ModernVolume(inline, seed, descriptor)
                    result = self.run_image(volume, engine)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    expected = volume.payload if inline else volume.read(0, volume.size)
                    self.assertEqual(result.stdout, expected)
                    if inline:
                        result = self.run_image(volume, engine, name="nested/alias.bin")
                        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                        self.assertEqual(result.stdout, expected)

    def test_metadata_corruption_is_rejected(self):
        for location in ("super", "group", "inode-map", "block-map", "inode", "dir", "extent"):
            with self.subTest(location=location):
                volume = ModernVolume()
                offsets = {"super": 1024 + 120, "group": volume.gdt * 1024 + 12,
                           "inode-map": volume.inode_map * 1024 + 1,
                           "block-map": volume.block_map * 1024 + 4,
                           "inode": volume.file_inode + 8, "dir": volume.root_block * 1024 + 40,
                           "extent": volume.extent_blocks[0] * 1024 + 8}
                volume.data[offsets[location]] ^= 1
                result = self.run_image(volume)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_indexed_directory_checksums_and_count_bounds(self):
        volume = ModernVolume(indexed=True)
        result = self.run_image(volume)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(result.stdout, volume.read(0, volume.size))
        for fault in ("root-checksum", "index-checksum", "count", "limit", "leaf-checksum"):
            with self.subTest(fault=fault):
                volume = ModernVolume(indexed=True)
                if fault == "root-checksum":
                    volume.data[volume.root_block * 1024 + 1020] ^= 1
                elif fault == "index-checksum":
                    volume.data[volume.index_blocks[1][0] * 1024 + 1020] ^= 1
                elif fault == "leaf-checksum":
                    volume.data[volume.dir_leaves[0] * 1024 + 1020] ^= 1
                else:
                    struct.pack_into("<H", volume.data, volume.root_block * 1024 +
                                     (34 if fault == "count" else 32), 65535)
                result = self.run_image(volume)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_linux_creator_inline_bigalloc_and_distributed_descriptors(self):
        fixtures = Path(__file__).parent / "fixtures" / "ext4"
        for case in ("inline", "bigalloc", "meta-bg"):
            with self.subTest(case=case):
                class Image:
                    data = zlib.decompress((fixtures / (case + ".ext4.zlib")).read_bytes())
                for name, expected in ((NAME, bytes(range(100))), ("big.bin", bytes(range(256)) * 257)):
                    result = self.run_image(Image, name=name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

    def test_inline_xattr_and_directory_corruption(self):
        for fault in ("overlap", "value-size", "external", "missing-data", "record", "inode-number"):
            with self.subTest(fault=fault):
                volume = ModernVolume(inline=True)
                if fault == "overlap":
                    struct.pack_into("<H", volume.data, volume.file_inode + 166, 0)
                elif fault == "value-size":
                    volume.put32(volume.file_inode + 172, 1 << 25)
                elif fault == "external":
                    volume.put32(volume.file_inode + 168, 2)
                elif fault == "missing-data":
                    volume.data[volume.file_inode + 180] = ord("X")
                elif fault == "record":
                    struct.pack_into("<H", volume.data, volume.inline_dir_inode + 48, 55)
                else:
                    volume.put32(volume.inline_dir_inode + 44, 1000)
                volume.refresh()
                result = self.run_image(volume)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_unknown_layout_and_encrypted_content_cannot_succeed(self):
        for encrypted in (False, True):
            with self.subTest(encrypted=encrypted):
                volume = ModernVolume()
                features = struct.unpack_from("<I", volume.data, 1024 + 96)[0]
                volume.put32(1024 + 96, features | (0x10000 if encrypted else 1 << 31))
                if encrypted:
                    volume.put32(volume.file_inode + 32, (1 << 19) | 0x800)
                volume.refresh()
                result = self.run_image(volume)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_external_xattr_checksum_and_value_bounds(self):
        volume = ModernVolume()
        volume.add_external_xattr()
        result = self.run_image(volume)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(result.stdout, volume.read(0, volume.size))
        for fault in ("checksum", "header", "overlap"):
            with self.subTest(fault=fault):
                volume = ModernVolume()
                volume.add_external_xattr()
                start = volume.xattr_block * 1024
                if fault == "checksum":
                    volume.data[start + 16] ^= 1
                elif fault == "header":
                    volume.put32(start + 8, 2)
                    volume.refresh()
                else:
                    struct.pack_into("<BBHIII", volume.data, start + 32, 4, 1, 8, 0, 64, 0)
                    volume.data[start + 48:start + 52] = b"name"
                    volume.refresh()
                result = self.run_image(volume)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_complete_group_and_fast_symlink_with_xattr(self):
        for engine in ("7z", "7zz"):
            v = Volume("direct", full_group=True)
            result = self.run_image(v, engine)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
            self.assertEqual(result.stdout, v.read(0, v.size))
            v = Volume("fastlink-xattr")
            result = self.run_image(v, engine, "l", None)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
            self.assertIn("Symbolic Link = " + NAME, result.stdout.decode("utf-8"))

    def test_invalid_extent_headers_ranges_and_parent_boundaries(self):
        for engine in ("7z", "7zz"):
            for fault in ("zero-max", "too-large-max", "entries-over-max", "zero-length", "overlap",
                          "physical-range", "child-first", "index-order", "child-overrun",
                          "conflicting-flags", "acl-range", "empty-child"):
                with self.subTest(engine=engine, fault=fault):
                    v = Volume("extent-tree" if fault.startswith("child") or fault == "empty-child" else "extent")
                    root = v.file_inode + 40
                    if fault == "zero-max":
                        struct.pack_into("<H", v.data, root + 4, 0)
                    elif fault == "too-large-max":
                        struct.pack_into("<H", v.data, root + 4, 5)
                    elif fault == "entries-over-max":
                        struct.pack_into("<H", v.data, root + 4, 1)
                    elif fault == "zero-length":
                        struct.pack_into("<H", v.data, root + 16, 0)
                    elif fault == "overlap":
                        struct.pack_into("<I", v.data, root + 24, 2)
                    elif fault == "physical-range":
                        struct.pack_into("<I", v.data, root + 20, v.blocks)
                    elif fault == "child-first":
                        struct.pack_into("<I", v.data, root + 12, 1)
                    elif fault == "conflicting-flags":
                        v.put32(v.file_inode + 32, (1 << 19) | (1 << 28))
                    elif fault == "acl-range":
                        v.put32(v.file_inode + 104, v.blocks)
                    elif fault == "empty-child":
                        struct.pack_into("<H", v.data, v.extent_blocks[0] * v.block_size + 2, 0)
                    elif fault == "index-order":
                        block = v.allocate(v.extent_header(0, [(2, 1, 8)], 4))
                        v.data[root:root + 60] = v.extent_header(1, [(2, block), (2, block)], 4)
                    else:
                        block = v.extent_blocks[0]
                        v.data[root:root + 60] = v.extent_header(1, [(2, block), (3, block)], 4)
                    result = self.run_image(v, engine)
                    self.assertIn(result.returncode, (1, 2), result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, b"")

    def test_invalid_directory_records_terminate(self):
        for engine in ("7z", "7zz"):
            for value in (0, 4, 10, 1020):
                with self.subTest(engine=engine, record_length=value):
                    v = Volume("direct")
                    pos = v.root_block * v.block_size + 24
                    struct.pack_into("<I", v.data, pos, 0)
                    struct.pack_into("<H", v.data, pos + 4, value)
                    result = self.run_image(v, engine, "l", None)
                    self.assertIn(result.returncode, (1, 2), result.stderr)

    def test_directory_records_cannot_cross_blocks_or_use_unaligned_lengths(self):
        for engine in ("7z", "7zz"):
            for crossing in (False, True):
                with self.subTest(engine=engine, crossing=crossing):
                    v = Volume("direct")
                    start = v.root_block * v.block_size
                    directory_size = v.block_size * (2 if crossing else 1)
                    if crossing:
                        v.put32(v.root_inode + 4, directory_size)
                        v.put32(v.root_inode + 28, 4)
                        v.put32(v.root_inode + 44, v.root_block + 1)
                    v.data[start + 24:start + directory_size] = bytes(directory_size - 24)
                    first_length = 1020 if crossing else 10
                    struct.pack_into("<H", v.data, start + 28, first_length)
                    struct.pack_into("<H", v.data, start + 24 + first_length + 4,
                                     directory_size - 24 - first_length)
                    result = self.run_image(v, engine, "l", None)
                    self.assertIn(result.returncode, (1, 2), result.stderr)

    def test_large_block_directory_empty_record_encodings(self):
        for engine in ("7z", "7zz"):
            for encoded in (0, 0xFFFF):
                with self.subTest(engine=engine, encoded=encoded):
                    v = Volume("direct", block_bits=16)
                    block = v.allocate()
                    struct.pack_into("<H", v.data, block * v.block_size + 4, encoded)
                    v.put32(v.root_inode + 4, 2 * v.block_size)
                    v.put32(v.root_inode + 28, 2 * v.block_size // 512)
                    v.put32(v.root_inode + 44, block)
                    v.superblock()
                    result = self.run_image(v, engine)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, v.read(0, v.size))


if __name__ == "__main__":
    unittest.main()
