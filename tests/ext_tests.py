"""Production ext sparse reads, extent boundaries and malformed directories."""
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from ext_fixture import Volume, NAME

RUNTIME = Path(sys.argv.pop(1))
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
