"""XZ framing, filters, blocks, padding and integrity failures."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from xz_fixture import CASES, BAD, volume

RUNTIME = Path(sys.argv.pop(1)).resolve()
SUFFIX = ".exe" if sys.platform == "win32" else ""


class XzTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-xz-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        for name in ("7z" + SUFFIX, "7zz" + SUFFIX,
                     "7z.dll" if sys.platform == "win32" else "7z.so"):
            shutil.copy2(RUNTIME / name, cls.runtime / name)

    def command(self, engine, path, command, *args, forced=True):
        return subprocess.run(
            [str(self.runtime / (engine + SUFFIX)), command, "-sccUTF-8", "-bsp0",
             "-txz:s0" if forced else "-t*:s0", *args, "--", str(path)],
            capture_output=True, timeout=10)

    def test_read_and_discover(self):
        for kind in CASES:
            image, expected = volume(kind, self.runtime / ("7zz" + SUFFIX))
            with tempfile.TemporaryDirectory(prefix="sevenzip-xz-") as temporary:
                path = Path(temporary) / "архив 空.bin"
                path.write_bytes(image)
                for engine in ("7z", "7zz"):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "l", "-slt", forced=False)
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertIn(b"Type = xz", result.stdout)
                        for threads in (1, 2):
                            result = self.command(engine, path, "x", "-so", "-bso0", "-bse2",
                                                  "-mmt=" + str(threads))
                            self.assertEqual(result.returncode, 0, result.stderr)
                            self.assertEqual(result.stdout, expected)
                        result = self.command(engine, path, "t")
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)

    def test_malformed_input_fails_normally(self):
        for kind in BAD:
            image, _ = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-xz-invalid-") as temporary:
                path = Path(temporary) / "bad.bin"
                path.write_bytes(image)
                for engine in ("7z", "7zz"):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "t")
                        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)


if __name__ == "__main__":
    unittest.main()
