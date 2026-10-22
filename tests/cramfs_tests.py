"""Production-process reads of Linux cramfs layouts and corrupt inputs."""
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from cramfs_fixture import image, checksum, inode, partial_directory, declare_size

RUNTIME = Path(sys.argv.pop(1))
MODULE = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""


class CramfsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-cramfs-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        core = "7z.dll" if sys.platform == "win32" else "7z.so"
        for name in ("7z" + SUFFIX, "7zz" + SUFFIX, core):
            shutil.copy2(RUNTIME / name, cls.runtime / name)
        (cls.runtime / "Formats").mkdir()
        shutil.copy2(MODULE, cls.runtime / "Formats" / MODULE.name)

    def run_image(self, data, engine="7z", command="x", forced=True, members=()):
        with tempfile.TemporaryDirectory(dir=self.runtime) as temporary:
            path = Path(temporary) / "образ 空.img"
            path.write_bytes(data)
            options = ["-slt", "-sccUTF-8", "-bsp0", "-bse2"] if command == "l" else ["-so", "-bso0", "-bsp0", "-bse2"]
            completed = subprocess.run([str(self.runtime / (engine + SUFFIX)), command,
                                        *options, *(["-tCramFS"] if forced else []), "--", str(path), *members],
                                       capture_output=True, timeout=10)
            self.assertEqual(path.read_bytes(), data)
            return completed

    def assertContents(self, data, expected, engine="7z"):
        result = self.run_image(data, engine)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(result.stdout, expected)

    def test_sparse_contents(self):
        for engine in ("7z", "7zz"):
            for endian in ("<", ">"):
                for blocks in ([bytes(4096), b"A" * 4096, bytes(73)],
                               [b"A" * 4096, bytes(4096), b"tail"], [bytes(4096), bytes(73)]):
                    with self.subTest(engine=engine, endian=endian, sizes=list(map(len, blocks))):
                        self.assertContents(image(blocks, endian), b"".join(blocks), engine)

    def test_retains_explicit_legacy_lzma_and_larger_blocks(self):
        for engine in ("7z", "7zz"):
            self.assertContents(image([b"A" * 8192, b"tail"], legacy_block_bits=13), b"A" * 8192 + b"tail", engine)
            self.assertContents(image([b"A" * 4096, b"tail"], storage=["lzma"] * 2,
                                      legacy_block_bits=12), b"A" * 4096 + b"tail", engine)

    def test_extended_pointers_and_mixed_blocks(self):
        for endian in ("<", ">"):
            for prefix, shifted in ((0, 0), (512, 128)):
                for storage in (["direct-raw", "zlib", "direct-zlib", "raw"],
                                ["direct-zlib", "raw", "zlib", "direct-raw"]):
                    blocks = [b"A" * 4096, b"B" * 4096, bytes(4096), b"tail"]
                    with self.subTest(endian=endian, prefix=prefix, storage=storage):
                        self.assertContents(image(blocks, endian, storage, prefix, shifted), b"".join(blocks))

    def test_discontiguous_direct_blocks_and_aligned_eof(self):
        for endian in ("<", ">"):
            blocks = [b"A" * 4096, b"B" * 4096]
            self.assertContents(image(blocks, endian, ["direct-raw"] * 2, reverse=True), b"".join(blocks))
            self.assertContents(image(blocks, endian, ["direct-zlib"] * 2, reverse=True), b"".join(blocks))

    def test_prefix_shifted_root_and_old_superblock(self):
        for version2 in (False, True):
            for prefix in (0, 512):
                for shifted in (0, 128):
                    with self.subTest(version2=version2, prefix=prefix, shifted=shifted):
                        self.assertContents(image([b"tail"], prefix=prefix, shifted=shifted,
                                                  version2=version2), b"tail")
            self.assertContents(image([b"tail"], storage=["direct-raw"], version2=version2), b"tail")

    def test_global_volume_coordinates_do_not_create_false_tails(self):
        for engine in ("7z", "7zz"):
            for boot_prefix, outer_prefix in ((512, 0), (0, 1024)):
                volume = image([b"volume payload"], prefix=boot_prefix)
                data = b"X" * outer_prefix + volume
                for forced in (False, True):
                    with self.subTest(engine=engine, boot=boot_prefix, outer=outer_prefix, forced=forced):
                        listing = self.run_image(data, engine, "l", forced)
                        self.assertIn(listing.returncode, (0, 1), listing.stderr)
                        header = listing.stdout.partition(b"----------")[0]
                        properties = dict(line.split(b" = ", 1) for line in header.splitlines() if b" = " in line)
                        self.assertEqual(int(properties.get(b"Offset", b"0")), outer_prefix)
                        self.assertEqual(int(properties[b"Physical Size"]), len(volume))
                        self.assertEqual(int(properties.get(b"Tail Size", b"0")), 0)
                        extracted = self.run_image(data, engine, forced=forced)
                        self.assertIn(extracted.returncode, (0, 1), extracted.stderr)
                        self.assertEqual(extracted.stdout, b"volume payload")

    def test_underdeclared_size_keeps_referenced_data_with_explicit_limitations(self):
        for endian in ("<", ">"):
            for prefix in (0, 512):
                for kind in ("zlib", "raw", "direct-zlib", "direct-raw"):
                    payload = b"A" * 4096 + b"tail"
                    original = image([payload[:4096], payload[4096:]], endian,
                                     [kind, kind], prefix=prefix)
                    for size in (0, prefix + 76, len(original) - 1):
                        with self.subTest(endian=endian, prefix=prefix, kind=kind, size=size):
                            data = declare_size(original, size, endian, prefix)
                            listing = self.run_image(data, command="l", forced=False)
                            self.assertIn(listing.returncode, (0, 1), listing.stderr)
                            self.assertIn(b"declared CramFS volume size", listing.stdout)
                            self.assertIn(b"Read-only limitations =", listing.stdout)
                            extracted = self.run_image(data, forced=False)
                            self.assertIn(extracted.returncode, (0, 1), extracted.stderr)
                            self.assertEqual(extracted.stdout, payload)

    def test_trailing_source_does_not_enable_limited_reading_or_extend_checksum(self):
        for prefix in (0, 512):
            original = image([b"payload"], storage=["direct-raw"], prefix=prefix)
            listing = self.run_image(original + b"unrelated trailing data", command="l")
            self.assertEqual(listing.returncode, 0, listing.stderr)
            self.assertNotIn(b"Read-only limitations =", listing.stdout)
            self.assertNotIn(b"declared CramFS volume size", listing.stdout)
            extracted = self.run_image(original + b"unrelated trailing data")
            self.assertEqual(extracted.returncode, 0, extracted.stderr)
            self.assertEqual(extracted.stdout, b"payload")

    def test_overdeclared_size_does_not_refuse_readable_file(self):
        for prefix in (0, 512):
            data = bytearray(image([b"readable prefix"], storage=["direct-raw"], prefix=prefix))
            struct.pack_into("<I", data, prefix + 4, len(data) + 16)
            for forced in (False, True):
                with self.subTest(prefix=prefix, forced=forced):
                    listing = self.run_image(data, command="l", forced=forced)
                    self.assertIn(listing.returncode, (0, 1), listing.stderr)
                    self.assertNotIn(b"ERRORS:", listing.stdout)
                    self.assertIn(b"source is shorter than the declared volume", listing.stdout)
                    extracted = self.run_image(data, forced=forced)
                    self.assertIn(extracted.returncode, (0, 1), extracted.stderr)
                    self.assertEqual(extracted.stdout, b"readable prefix")

    def test_shared_direct_data_and_short_uncompressed_tail(self):
        data = bytearray(image([b"A" * 4096] * 2, storage=["direct-raw"] * 2))
        data[100:104] = data[96:100]
        checksum(data)
        self.assertContents(data, b"A" * 8192)
        self.assertContents(image([b"tail" + bytes(4092)], short_zlib=True), b"tail" + bytes(4092))
        self.assertContents(image([b"tail" + bytes(4092)], storage=["raw"]), b"tail" + bytes(4092))

    def test_empty_files_empty_root_and_symlink_data(self):
        self.assertContents(image([]), b"")
        self.assertContents(image([b"target"], mode=0o120777), b"target")
        data = bytearray(image([])[:76])
        data[64:76] = inode(0o40755, 0, 0)
        checksum(data)
        self.assertContents(data, b"")
        data[64:76] = inode(0o40755, 0, 0x0ffffffc)
        struct.pack_into("<I", data, 8, struct.unpack_from("<I", data, 8)[0] | 0x400)
        checksum(data)
        self.assertContents(data, b"")

    def test_nested_unicode_names_and_ownership_metadata(self):
        for endian in ("<", ">"):
            data = image([b"tail"], endian, ["direct-raw"], prefix=512,
                         name="файл 空.bin".encode(), directory="папка 空".encode())
            self.assertContents(data, b"tail")
            listing = self.run_image(data, command="l")
            self.assertEqual(listing.returncode, 0, listing.stderr)
            text = listing.stdout.decode("utf-8").replace("\\", "/")
            self.assertIn("Path = папка 空/файл 空.bin", text)
            self.assertIn("User ID = 1234", text)
            self.assertIn("Group ID = 42", text)

    def test_empty_inode_streams_do_not_require_unused_offsets(self):
        for engine in ("7z", "7zz"):
            for endian in ("<", ">"):
                for mode, size in ((0o100644, 0), (0o120777, 0), (0o20644, 0x1234)):
                    data = bytearray(image([], endian, mode=mode))
                    data[76:88] = inode(mode, size, 0x0ffffffc, 8, endian)
                    checksum(data, endian)
                    with self.subTest(engine=engine, endian=endian, mode=mode):
                        self.assertContents(data, b"", engine)
                        listing = self.run_image(data, engine, "l")
                        self.assertEqual(listing.returncode, 0, listing.stderr)
                        self.assertIn(b"Packed Size = 0", listing.stdout)
                        if mode == 0o20644:
                            self.assertIn(b"Device Major = 18", listing.stdout)
                            self.assertIn(b"Device Minor = 52", listing.stdout)

    def test_fsid_counters_do_not_control_readable_file_access(self):
        for engine in ("7z", "7zz"):
            for endian in ("<", ">"):
                for count in (0, 0xffffffff):
                    data = bytearray(image([b"readable data"], endian))
                    struct.pack_into(endian + "II", data, 40, count, count)
                    checksum(data, endian)
                    with self.subTest(engine=engine, endian=endian, count=count):
                        self.assertContents(data, b"readable data", engine)

    def test_zero_offset_directory_has_no_payload_despite_raw_size(self):
        for engine in ("7z", "7zz"):
            for endian in ("<", ">"):
                data = bytearray(image([b"unreferenced data"], endian))
                data[64:76] = inode(0o40755, 17, 0, endian=endian)
                checksum(data, endian)
                with self.subTest(engine=engine, endian=endian):
                    self.assertContents(data, b"", engine)
                    listing = self.run_image(data, engine, "l")
                    self.assertEqual(listing.returncode, 0, listing.stderr)
                    self.assertNotIn(b"Some CramFS directories", listing.stdout)

    def test_magic_detection_with_reserved_signature(self):
        for prefix in (0, 512):
            data = bytearray(image([b"tail"], storage=["direct-raw"], prefix=prefix))
            data[prefix + 16:prefix + 32] = bytes(16)
            checksum(data, prefix=prefix)
            result = self.run_image(data, forced=False)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stdout, b"tail")

    def test_invalid_metadata_never_publishes_unsafe_entries(self):
        for fault in ("root-type", "file-type", "empty-name", "traversal", "unsupported-feature"):
            data = bytearray(image([b"tail"]))
            if fault == "root-type":
                struct.pack_into("<H", data, 64, 0o100644)
            if fault == "file-type":
                struct.pack_into("<H", data, 76, 0)
            if fault == "empty-name":
                data[88:96] = bytes(8)
            if fault == "traversal":
                data[88:96] = b"../x\0\0\0\0"
            if fault == "unsupported-feature":
                struct.pack_into("<I", data, 8, 1 << 20)
            checksum(data)
            with self.subTest(fault=fault):
                result = self.run_image(data)
                self.assertEqual(result.stdout, b"")
                if fault in ("root-type", "unsupported-feature"):
                    self.assertNotEqual(result.returncode, 0)
                else:
                    listing = self.run_image(data, command="l")
                    self.assertIn(listing.returncode, (0, 1), listing.stderr)
                    self.assertIn(b"Some CramFS directories", listing.stdout)
                    self.assertNotIn(b"Path = ../x", listing.stdout)

    def test_checksum_warning_preserves_readable_payload(self):
        for engine in ("7z", "7zz"):
            for endian in ("<", ">"):
                data = bytearray(image([b"readable payload"], endian, prefix=512))
                data[512 + 32] ^= 1
                with self.subTest(engine=engine, endian=endian):
                    listing = self.run_image(data, engine, "l")
                    self.assertIn(listing.returncode, (0, 1), listing.stderr)
                    self.assertIn(b"checksum does not match", listing.stdout)
                    extracted = self.run_image(data, engine)
                    self.assertIn(extracted.returncode, (0, 1), extracted.stderr)
                    self.assertEqual(extracted.stdout, b"readable payload")

    def test_partial_directory_preserves_prefix_and_siblings(self):
        for endian in ("<", ">"):
            for fault in ("directory", "directory-cycle"):
                data, files, _ = partial_directory(fault, endian)
                with self.subTest(endian=endian, fault=fault):
                    listing = self.run_image(data, command="l")
                    self.assertIn(listing.returncode, (0, 1), listing.stderr)
                    self.assertIn(b"Some CramFS directories", listing.stdout)
                    self.assertIn(b"The CramFS directory could not be read completely", listing.stdout)
                    for name, payload in files.items():
                        result = self.run_image(data, members=(name,))
                        self.assertIn(result.returncode, (0, 1), result.stderr)
                        self.assertEqual(result.stdout, payload)

    def test_bad_data_ranges_and_streams(self):
        for fault in ("direct-range", "direct-length", "oversized-raw", "broken-zlib", "truncated"):
            data = bytearray(image([b"A" * 4096], storage=["direct-zlib"]))
            if fault == "direct-range":
                struct.pack_into("<I", data, 96, 0xFFFFFFFF)
            if fault == "direct-length":
                struct.pack_into("<H", data, 100, 9000)
            if fault == "oversized-raw":
                data = bytearray(image([b"A" * 8192], storage=["raw"]))
                struct.pack_into("<I", data, 80, (42 << 24) | 4096)
            if fault == "broken-zlib":
                data[103] ^= 1
            if fault == "truncated":
                data = data[:103]
            checksum(data)
            with self.subTest(fault=fault):
                result = self.run_image(data)
                self.assertNotEqual(result.returncode, 0)


if __name__ == "__main__":
    unittest.main()
