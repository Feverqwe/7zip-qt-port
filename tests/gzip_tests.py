"""Gzip recognition, long headers, concatenation and payload integrity."""
from pathlib import Path
from runtime_fixture import copy_codec_runtime
import shutil
import subprocess
import sys
import tempfile
import unittest
from gzip_fixture import CASES, BAD, volume

RUNTIME = Path(sys.argv.pop(1)).resolve()
SUFFIX = ".exe" if sys.platform == "win32" else ""


class GzipTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-gzip-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        copy_codec_runtime(RUNTIME, cls.runtime)

    def command(self, engine, path, command, *args, forced=True):
        return subprocess.run(
            [str(self.runtime / (engine + SUFFIX)), command, "-sccUTF-8", "-bsp0",
             *(["-tgzip:s0"] if forced else ["-t*:s0"]), *args, "--", str(path)],
            capture_output=True, timeout=10)

    def test_read_and_discover(self):
        for kind in CASES:
            image, expected, name = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-gzip-") as temporary:
                path = Path(temporary) / "архив 空.bin"
                path.write_bytes(image)
                for engine in ("7z",):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "l", "-slt", forced=False)
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertIn(b"Type = gzip", result.stdout)
                        self.assertIn(b"Path = " + name + b"\n", result.stdout.replace(b"\r\n", b"\n"))
                        result = self.command(engine, path, "x", "-so", "-bso0", "-bse2")
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(result.stdout, expected)
                        result = self.command(engine, path, "t")
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)

    def test_malformed_input_fails_normally(self):
        for kind in BAD:
            image, _, _ = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-gzip-invalid-") as temporary:
                path = Path(temporary) / "bad.bin"
                path.write_bytes(image)
                for engine in ("7z",):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "t")
                        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)


if __name__ == "__main__":
    unittest.main()
