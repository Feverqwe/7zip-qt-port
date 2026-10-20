"""Raw CPIO members, inode aliases, checksums and legacy compatibility."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from cpio_fixture import CASES, BAD, volume

RUNTIME = Path(sys.argv.pop(1)).resolve()
SUFFIX = ".exe" if sys.platform == "win32" else ""


class CpioTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-cpio-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        for name in ("7z" + SUFFIX, "7zz" + SUFFIX,
                     "7z.dll" if sys.platform == "win32" else "7z.so"):
            shutil.copy2(RUNTIME / name, cls.runtime / name)

    def command(self, engine, path, command, *args, forced=True):
        return subprocess.run(
            [str(self.runtime / (engine + SUFFIX)), command, "-sccUTF-8", "-bsp0",
             *(["-tCpio:s0"] if forced else ["-t*"]), *args, "--", str(path)],
            capture_output=True, timeout=10)

    def test_read_and_extract(self):
        for kind in CASES:
            image, files, links = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-cpio-") as temporary:
                path = Path(temporary) / "архив 空.cpio"
                path.write_bytes(image)
                for engine in ("7z", "7zz"):
                    with self.subTest(kind=kind, engine=engine):
                        listing = self.command(engine, path, "l", "-slt", forced=False)
                        self.assertEqual(listing.returncode, 0, listing.stdout + listing.stderr)
                        self.assertIn(b"Type = Cpio", listing.stdout)
                        tested = self.command(engine, path, "t")
                        self.assertEqual(tested.returncode, 0, tested.stdout + tested.stderr)
                        for name, expected in files.items():
                            result = subprocess.run(
                                [str(self.runtime / (engine + SUFFIX)), "x", "-tCpio", "-so",
                                 "-bso0", "-bse2", "--", str(path), name],
                                capture_output=True, timeout=10)
                            self.assertEqual(result.returncode, 0, result.stderr)
                            self.assertEqual(result.stdout, expected, name)
                        for name, target in links.items():
                            self.assertIn(("Path = " + name).encode(), listing.stdout)
                            self.assertIn(("Symbolic Link = " + target).encode(), listing.stdout)
                        if links:
                            continue  # Symlink metadata is checked without requiring host privileges.
                        output = Path(temporary) / engine
                        result = self.command(engine, path, "x", "-y", "-o" + str(output))
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        for name, expected in files.items():
                            self.assertEqual((output / name).read_bytes(), expected, name)
                        self.assertEqual(path.read_bytes(), image)

    def test_malformed_input_fails_normally(self):
        for kind in BAD:
            image, _, _ = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-cpio-invalid-") as temporary:
                path = Path(temporary) / "bad.cpio"
                path.write_bytes(image)
                for engine in ("7z", "7zz"):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "t")
                        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)


if __name__ == "__main__":
    unittest.main()
