"""ISO descriptor, extent and source boundaries through both shipped engines."""
from pathlib import Path
import subprocess
import shutil
import sys
import tempfile
import unittest
from iso_fixture import volume

RUNTIME = Path(sys.argv.pop(1))
MODULE = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""


class IsoTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-iso-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        for name in ("7z" + SUFFIX, "7zz" + SUFFIX,
                     "7z.dll" if sys.platform == "win32" else "7z.so"):
            shutil.copy2(RUNTIME / name, cls.runtime / name)
        (cls.runtime / "Formats").mkdir()
        shutil.copy2(MODULE, cls.runtime / "Formats" / MODULE.name)

    def run_image(self, engine, image, name=None):
        with tempfile.TemporaryDirectory(prefix="sevenzip-iso-") as temporary:
            path = Path(temporary) / "образ 空.iso"
            path.write_bytes(image)
            args = [str(self.runtime / (engine+SUFFIX)), "x" if name else "t", "-tIso", "-sccUTF-8", "-bsp0"]
            if name:
                args += ["-so", "-bso0", "-bse2"]
            args += ["--", str(path)]
            if name:
                args += [name]
            result = subprocess.run(args, capture_output=True, timeout=10)
            self.assertEqual(path.read_bytes(), image)
            return result

    def test_valid_extent_bytes(self):
        for kind in ("plain", "joliet", "block-512", "block-1024", "block-512-tail", "xar",
                     "xar-directory", "multi", "multi-directory", "unknown-descriptor",
                     "foreign-descriptor", "no-terminator", "unit-no-gap", "big-endian-mirror"):
            image, files = volume(kind)
            for engine in ("7z", "7zz"):
                for name, expected in files.items():
                    with self.subTest(kind=kind, engine=engine, name=name):
                        result = self.run_image(engine, image, name)
                        self.assertEqual(result.stdout, expected, result.stderr.decode(errors="replace"))
                        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))

    def test_bad_mapping_never_returns_bytes(self):
        for kind in ("outside-volume", "xar-outside-volume", "short-source", "missing-final",
                     "misaligned-section", "interleave"):
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    result = self.run_image(engine, volume(kind)[0], "ALPHA.BIN")
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")

    def test_invalid_directory_is_rejected(self):
        for kind in ("bad-directory", "cycle", "bad-name-length", "short-record"):
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    self.assertNotEqual(self.run_image(engine, volume(kind)[0]).returncode, 0)

    def test_rock_ridge_files_and_volume_choice(self):
        for kind in ("rock", "rock-joliet", "rock-no-er", "rock-no-er-joliet", "rock-metadata", "rock-shared-ce", "rock-cl-same-extent", "rock-empty-primary", "rock-bad-primary", "rock-bad-primary-name"):
            image, files = volume(kind)
            for name, expected in files.items():
                with self.subTest(kind=kind, name=name):
                    result = self.run_image("7z", image, name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

    def test_zisofs_decoded_bytes(self):
        for kind in ("zisofs", "zisofs-shift0", "zisofs-pointer-boundary", "zisofs-expanding", "zisofs-small-sparse",
                     "zisofs-shift17", "zisofs-padding", "zisofs-header0", "zisofs-embedded-header",
                     "zisofs-empty", "zisofs-sparse-marker", "zisofs-multi", "zisofs-block512"):
            image, files = volume(kind)
            for name, expected in files.items():
                with self.subTest(kind=kind, name=name):
                    result = self.run_image("7z", image, name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

    def test_zisofs_corruption_is_data_error(self):
        for kind in ("zisofs-crc", "zisofs-short-zlib", "zisofs-short-output", "zisofs-long-output",
                     "zisofs-pointer-order", "zisofs-pointer-outside", "zisofs-pointer-header",
                     "zisofs-short-table"):
            with self.subTest(kind=kind):
                result = self.run_image("7z", volume(kind)[0], "alpha.bin")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(b"Data Error", result.stderr)

    def test_bad_rock_ridge_never_publishes_partial_catalog(self):
        for kind in ("rock-ce-outside", "rock-ce-crossblock", "rock-ce-cycle", "rock-sl-overrun",
                     "rock-cl-cycle", "rock-cl-outside", "rock-root-mode"):
            with self.subTest(kind=kind):
                result = self.run_image("7z", volume(kind)[0], "alpha.bin")
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")


if __name__ == "__main__":
    unittest.main()
