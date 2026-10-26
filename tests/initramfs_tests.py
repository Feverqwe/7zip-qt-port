"""Production dynamic engine: complete initramfs images and strict part boundaries."""
from pathlib import Path
from runtime_fixture import copy_codec_runtime
import shutil
import subprocess
import sys
import tempfile
import unittest
from initramfs_fixture import CASES, BAD, volume

RUNTIME, MODULE = (Path(sys.argv.pop(1)).resolve() for _ in range(2))
SUFFIX = ".exe" if sys.platform == "win32" else ""


class InitramfsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-initramfs-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.root = Path(cls.temporary.name)
        copy_codec_runtime(RUNTIME, cls.root)
        (cls.root / "Formats").mkdir()
        shutil.copy2(MODULE, cls.root / "Formats" / MODULE.name)

    def command(self, path, *args, selected=()):
        return subprocess.run([str(self.root / ("7z" + SUFFIX)), *args, "-sccUTF-8", "-bsp0",
                               "--", str(path), *selected], capture_output=True, timeout=10)

    def test_complete_images(self):
        for kind in CASES:
            with self.subTest(kind=kind), tempfile.TemporaryDirectory(dir=self.root) as temporary:
                root = Path(temporary)
                path = root / "образ 空.img"
                image, files = volume(kind)
                path.write_bytes(image)
                selected = list(files) if kind.endswith("symlink-trailer") else []
                result = self.command(path, "x", "-tCpio:s0", "-y", "-o" + str(root / "out"),
                                      selected=selected)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                for name, expected in files.items():
                    self.assertEqual((root / "out" / name).read_bytes(), expected)
                self.assertEqual(path.read_bytes(), image)
                result = self.command(path, "l", "-t*:s0", "-slt")
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn(b"Type = Cpio", result.stdout)
                if kind.endswith("symlink-trailer"):
                    self.assertIn(b"Path = TRAILER!!!", result.stdout)
                    self.assertIn(b"Symbolic Link = target", result.stdout)
                for name in files:
                    self.assertIn(name.encode(), result.stdout)

    def test_part_boundaries(self):
        for kind in BAD:
            with self.subTest(kind=kind):
                path = self.root / "invalid.img"
                path.write_bytes(volume(kind)[0])
                result = self.command(path, "t", "-tCpio:s0")
                self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
                path.unlink()


if __name__ == "__main__":
    unittest.main()
