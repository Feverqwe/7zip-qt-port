"""ISO descriptor, extent and source boundaries through both shipped engines."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from iso_fixture import volume

RUNTIME = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""


class IsoTests(unittest.TestCase):
    def run_image(self, engine, image, name=None):
        with tempfile.TemporaryDirectory(prefix="sevenzip-iso-") as temporary:
            path = Path(temporary) / "образ 空.iso"
            path.write_bytes(image)
            args = [str(RUNTIME / (engine+SUFFIX)), "x" if name else "t", "-tIso", "-sccUTF-8", "-bsp0"]
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


if __name__ == "__main__":
    unittest.main()
