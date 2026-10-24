"""LZMA headers, dictionaries, member boundaries and normal error results."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from lzma_fixture import CASES, BAD, volume

RUNTIME = Path(sys.argv.pop(1)).resolve()
SUFFIX = ".exe" if sys.platform == "win32" else ""


class LzmaTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-lzma-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        for name in ("7z" + SUFFIX,
                     "7z.dll" if sys.platform == "win32" else "7z.so"):
            shutil.copy2(RUNTIME / name, cls.runtime / name)

    def command(self, engine, path, command, format_name, *args, forced=True):
        return subprocess.run(
            [str(self.runtime / (engine + SUFFIX)), command, "-sccUTF-8", "-bsp0",
             "-t" + format_name + ":s0" if forced else "-t*:s0", *args, "--", str(path)],
            capture_output=True, timeout=10)

    def test_read_and_discover(self):
        for kind in CASES:
            image, expected, format_name = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-lzma-") as temporary:
                path = Path(temporary) / "архив 空.bin"
                path.write_bytes(image)
                for engine in ("7z",):
                    with self.subTest(kind=kind, engine=engine):
                        # LZMA86 has no identifying signature; explicit type is its public entry point.
                        result = self.command(engine, path, "l", format_name, "-slt",
                                              forced=format_name == "lzma86")
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertIn(("Type = " + format_name).encode(), result.stdout)
                        result = self.command(engine, path, "x", format_name, "-so", "-bso0", "-bse2")
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(result.stdout, expected)
                        result = self.command(engine, path, "t", format_name)
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)

    def test_malformed_input_fails_normally(self):
        for kind in BAD:
            image, _, format_name = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-lzma-invalid-") as temporary:
                path = Path(temporary) / "bad.bin"
                path.write_bytes(image)
                for engine in ("7z",):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "t", format_name)
                        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)


if __name__ == "__main__":
    unittest.main()
