"""The production dynamic engine reads boot frames through the GPL module."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from linux_frames_fixture import CASES, BAD, volume

RUNTIME = Path(sys.argv.pop(1)).resolve()
MODULE = Path(sys.argv.pop(1)).resolve()
SUFFIX = ".exe" if sys.platform == "win32" else ""


class LinuxFramesTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-linux-frames-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        for name in ("7z" + SUFFIX,
                     "7z.dll" if sys.platform == "win32" else "7z.so"):
            shutil.copy2(RUNTIME / name, cls.runtime / name)
        (cls.runtime / "Formats").mkdir()
        shutil.copy2(MODULE, cls.runtime / "Formats" / MODULE.name)

    def run_frame(self, engine, path, fmt, command, *args, forced=True):
        return subprocess.run([str(self.runtime / (engine + SUFFIX)), command,
                               "-sccUTF-8", "-bsp0", "-t" + fmt + ":s0" if forced else "-t*:s0",
                               *args, "--", str(path)], capture_output=True, timeout=10)

    def test_valid_frames(self):
        for kind in CASES:
            image, meta = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-linux-frames-") as temporary:
                path = Path(temporary) / "образ 空.bin"
                path.write_bytes(image)
                for engine in ("7z",):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.run_frame(engine, path, meta["format"], "l", "-slt", forced=False)
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertIn(("Type = " + meta["format"]).encode(), result.stdout)
                        result = self.run_frame(engine, path, meta["format"], "x", "-so", "-bso0", "-bse2")
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(result.stdout, bytes.fromhex(meta["payload"]))
                        result = self.run_frame(engine, path, meta["format"], "t")
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)

    def test_corrupt_frames(self):
        for kind in BAD:
            image, meta = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-linux-frames-invalid-") as temporary:
                path = Path(temporary) / "bad.bin"
                path.write_bytes(image)
                for engine in ("7z",):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.run_frame(engine, path, meta["format"], "t")
                        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)

    def test_nested_cpio_extraction(self):
        for kind in ("lz4-cpio", "lzo-cpio"):
            image, meta = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-linux-frames-cpio-") as temporary:
                path = Path(temporary) / "образ 空.bin"
                path.write_bytes(image)
                result = self.run_frame("7z", path, meta["format"], "x", "-so", "-bso0", "-bse2")
                self.assertEqual(result.returncode, 0, result.stderr)
                inner = Path(temporary) / "inner.cpio"
                inner.write_bytes(result.stdout)
                result = self.run_frame("7z", inner, "Cpio", "x", "-so",
                                        "-bso0", "-bse2", "-i!данные 空.txt")
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout, b"nested cpio payload")
                self.assertEqual(path.read_bytes(), image)


if __name__ == "__main__":
    unittest.main()
