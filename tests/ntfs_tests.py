"""Read results and rejection of malformed NTFS through both delivered engines."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
import zlib
from ntfs_fixture import volume

RUNTIME = Path(sys.argv.pop(1))
MODULE = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""


class NtfsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-ntfs-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        core = "7z.dll" if sys.platform == "win32" else "7z.so"
        for name in ("7z" + SUFFIX, "7zz" + SUFFIX, core):
            shutil.copy2(RUNTIME / name, cls.runtime / name)
        (cls.runtime / "Formats").mkdir()
        shutil.copy2(MODULE, cls.runtime / "Formats" / MODULE.name)

    def run_image(self, engine, data, command="t", name=None):
        with tempfile.TemporaryDirectory(dir=self.runtime) as temporary:
            path = Path(temporary) / "образ 空.ntfs"
            path.write_bytes(data)
            args = [str(self.runtime / (engine + SUFFIX)), command, "-tNTFS", "-sccUTF-8", "-bsp0"]
            if command == "x":
                args += ["-so", "-bso0", "-bse2"]
            args += ["--", str(path)]
            if name:
                args.append(name)
            result = subprocess.run(args, capture_output=True, timeout=10)
            self.assertEqual(path.read_bytes(), data)
            return result

    def test_classic_read_paths_in_both_engines(self):
        for kind in ("base", "sparse", "hole-first", "initialized", "compressed", "sector4k", "boot-reserved", "backup-boot"):
            image, files = volume(kind)
            for engine in ("7z", "7zz"):
                for name, expected in files.items():
                    with self.subTest(kind=kind, engine=engine, name=name):
                        result = self.run_image(engine, image, "x", name)
                        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                        self.assertEqual(result.stdout, expected)

    def test_linux_partial_frame_is_zero_filled(self):
        image, files = volume("short-compressed")
        result = self.run_image("7z", image, "x", "alpha")
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(result.stdout, files["alpha"])

    def test_corrupt_or_encrypted_data_never_reports_success(self):
        for kind in ("run-over-volume", "run-underflow", "normal-virtual-over-volume", "invalid-initialized", "encrypted", "wof", "wof-empty", "bad-compressed"):
            image, _ = volume(kind)
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    result = self.run_image(engine, image, "x", "gamma" if kind.startswith("wof") else "alpha")
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")


if __name__ == "__main__":
    unittest.main()
