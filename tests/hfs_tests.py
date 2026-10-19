"""Fork bytes, metadata failures and volume boundaries through both engines."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from hfs_fixture import volume

RUNTIME = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""


class HfsTests(unittest.TestCase):
    def run_image(self, engine, data, name=None):
        with tempfile.TemporaryDirectory(prefix="sevenzip-hfs-") as temporary:
            path = Path(temporary) / "образ 空.hfs"
            path.write_bytes(data)
            args = [str(RUNTIME / (engine + SUFFIX)), "x" if name else "t",
                    "-tHFS", "-sccUTF-8", "-bsp0"]
            if name:
                args += ["-so", "-bso0", "-bse2", "-sns" if ":" in name else "-sns-"]
            args += ["--", str(path)]
            if name:
                args += [name]
            result = subprocess.run(args, capture_output=True, timeout=10)
            self.assertEqual(path.read_bytes(), data)
            return result

    def test_fork_contents(self):
        for kind in ("base", "preallocated", "empty-preallocated", "overflow-leaves", "boot-blocks",
                     "hfsx", "wrapped", "wrapped-hfsx"):
            image, files = volume(kind)
            for engine in ("7z", "7zz"):
                for name, expected in files.items():
                    with self.subTest(kind=kind, engine=engine, name=name):
                        result = self.run_image(engine, image, name)
                        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                        self.assertEqual(result.stdout, expected)

    def test_bad_forks_never_read_appended_bytes(self):
        for kind in ("extent-over-volume", "allocation-tail-over-volume"):
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    result = self.run_image(engine, volume(kind)[0], "HfsFixture/alpha")
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")

    def test_corrupt_metadata_is_rejected(self):
        for kind in ("metadata-short-map", "leaf-height", "leaf-backlink", "leaf-offset",
                     "leaf-tail", "leaf-count", "root-depth", "key-format",
                     "parent-cycle", "volume-truncated"):
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    self.assertNotEqual(self.run_image(engine, volume(kind)[0]).returncode, 0)


if __name__ == "__main__":
    unittest.main()
