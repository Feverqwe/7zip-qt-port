"""Fork bytes, metadata failures and volume boundaries through both engines."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from hfs_fixture import volume

RUNTIME = Path(sys.argv.pop(1))
MODULE = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""


class HfsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-hfs-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        core = "7z.dll" if sys.platform == "win32" else "7z.so"
        for name in ("7z" + SUFFIX, "7zz" + SUFFIX, core):
            shutil.copy2(RUNTIME / name, cls.runtime / name)
        (cls.runtime / "Formats").mkdir()
        shutil.copy2(MODULE, cls.runtime / "Formats" / MODULE.name)

    def run_image(self, engine, data, name=None):
        with tempfile.TemporaryDirectory(prefix="sevenzip-hfs-") as temporary:
            path = Path(temporary) / "образ 空.hfs"
            path.write_bytes(data)
            args = [str(self.runtime / (engine + SUFFIX)), "x" if name else "t",
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

    def test_linux_unicode_and_hardlink_bytes(self):
        for kind in ("modern", "modern-nul", "modern-date", "modern-chain", "modern-hfsx",
                     "modern-case", "modern-xattr", "modern-metadata", "modern-binary",
                     "modern-zlib", "modern-lzvn", "modern-hidden-case", "modern-permissions"):
            image, files = volume(kind)
            for name, expected in files.items():
                with self.subTest(kind=kind, name=name):
                    result = self.run_image("7z", image, name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

    def test_missing_and_corrupt_backing_inode_never_succeed(self):
        for kind in ("modern-missing", "modern-bounds", "modern-binary-case"):
            with self.subTest(kind=kind):
                result = self.run_image("7z", volume(kind)[0], "HfsFixture/alpha")
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_standalone_does_not_extract_hardlink_stubs_as_empty_success(self):
        result = self.run_image("7zz", volume("modern")[0], "HfsFixture/alpha")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, b"")

    def test_linux_metadata_and_thread_failures(self):
        for kind in ("modern-invalid-file-mode", "modern-invalid-dir-mode", "modern-thread-parent",
                     "modern-thread-name", "modern-thread-type", "modern-thread-length"):
            with self.subTest(kind=kind):
                self.assertNotEqual(self.run_image("7z", volume(kind)[0]).returncode, 0)

    def test_corrupt_metadata_is_rejected(self):
        for kind in ("metadata-short-map", "leaf-height", "leaf-backlink", "leaf-offset",
                     "leaf-tail", "leaf-count", "root-depth", "key-format",
                     "parent-cycle", "volume-truncated"):
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    self.assertNotEqual(self.run_image(engine, volume(kind)[0]).returncode, 0)


if __name__ == "__main__":
    unittest.main()
