"""Read results and rejection of malformed SquashFS through both delivered engines."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from squashfs_fixture import volume, legacy_volume

RUNTIME = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""


class SquashfsTests(unittest.TestCase):
    def run_image(self, engine, data, command="t", name=None):
        with tempfile.TemporaryDirectory(prefix="sevenzip-squashfs-") as temporary:
            path = Path(temporary) / "образ 空.squashfs"
            path.write_bytes(data)
            args = [str(RUNTIME / (engine + SUFFIX)), command, "-tSquashFS", "-sccUTF-8", "-bsp0"]
            if command == "x":
                args += ["-so", "-bso0", "-bse2"]
            args += ["--", str(path)]
            if name:
                args.append(name)
            result = subprocess.run(args, capture_output=True, timeout=10)
            self.assertEqual(path.read_bytes(), data)
            return result

    def test_blocks_fragments_sparse_and_unicode(self):
        for engine in ("7z", "7zz"):
            for compressed in (False, True):
                data, files = volume(compressed=compressed)
                for name, expected in files.items():
                    with self.subTest(engine=engine, compressed=compressed, name=name):
                        result = self.run_image(engine, data, "x", name)
                        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                        self.assertEqual(result.stdout, expected)

    def test_empty_filesystem_without_fragment_index(self):
        for engine in ("7z", "7zz"):
            result = self.run_image(engine, volume(no_fragments=True)[0])
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))

    def test_legacy_v3_little_and_big_endian_read_extension(self):
        for engine in ("7z", "7zz"):
            for endian in ("<", ">"):
                for compressed in (False, True):
                    with self.subTest(engine=engine, endian=endian, compressed=compressed):
                        data, expected = legacy_volume(endian, compressed)
                        result = self.run_image(engine, data, "x", "alpha")
                        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                        self.assertEqual(result.stdout, expected)

    def test_corrupt_metadata_and_data_boundaries(self):
        for engine in ("7z", "7zz"):
            for fault in ("fragment-large", "fragment-outside", "data-outside", "cross-block-inode",
                          "cycle", "count-wrap", "name-nul", "name-slash", "root-not-directory",
                          "uid-index", "negative-size", "metadata-bomb"):
                with self.subTest(engine=engine, fault=fault):
                    result = self.run_image(engine, volume(fault)[0])
                    self.assertNotEqual(result.returncode, 0, result.stdout.decode(errors="replace"))

    def test_shared_fragment_with_nonzero_offset_is_validated_before_read(self):
        for engine in ("7z", "7zz"):
            for fault in ("fragment-large", "fragment-outside"):
                result = self.run_image(engine, volume(fault)[0], "x", "данные 空")
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    @unittest.skipUnless(shutil.which("mksquashfs"), "requires official squashfs-tools")
    def test_official_images_with_fragments_exports_xattrs_and_hardlinks(self):
        with tempfile.TemporaryDirectory(prefix="sevenzip-squashfs-native-") as temporary:
            root = Path(temporary)
            source = root / "source"
            source.mkdir()
            payload = bytes(range(256)) * 1000 + bytes(131072) + b"tail"
            (source / "данные 空").write_bytes(payload)
            (source / "hardlink").hardlink_to(source / "данные 空")
            (source / "small").write_bytes(b"fragment")
            (source / "empty").mkdir()
            # Read native xattr tables when the host filesystem permits their creation.
            import os
            try:
                os.setxattr(source / "small", b"user.sevenzip", b"archive fixture")
            except (OSError, AttributeError):
                pass
            for codec in ("gzip", "xz", "lzo", "zstd"):
                image = root / "native.squashfs"
                created = subprocess.run(["mksquashfs", str(source), str(image), "-noappend",
                                          "-processors", "1", "-comp", codec, "-no-progress"],
                                         capture_output=True, timeout=15)
                self.assertEqual(created.returncode, 0, created.stderr.decode(errors="replace"))
                for engine in ("7z", "7zz"):
                    for name, expected in (("данные 空", payload), ("hardlink", payload),
                                           ("small", b"fragment")):
                        with self.subTest(codec=codec, engine=engine, name=name):
                            result = self.run_image(engine, image.read_bytes(), "x", name)
                            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                            self.assertEqual(result.stdout, expected)


if __name__ == "__main__":
    unittest.main()
