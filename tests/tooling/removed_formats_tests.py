"""Retired filesystem inputs must not reopen through the bundled engine."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zlib

RUNTIME = Path(sys.argv.pop(1))
ENGINE = RUNTIME / ("7z.exe" if sys.platform == "win32" else "7z")


class RemovedFormatsTests(unittest.TestCase):
    def test_cramfs_is_unsupported(self):
        image = zlib.decompress((Path(__file__).parents[1] /
                                "fixtures/retired/cramfs.img.zlib").read_bytes())
        with tempfile.TemporaryDirectory(prefix="sevenzip-retired-") as directory:
            path = Path(directory) / "образ 空.cramfs"
            path.write_bytes(image)
            for forced in ([], ["-tCramFS"]):
                with self.subTest(forced=forced):
                    result = subprocess.run([str(ENGINE), "x", "-so", "-bso0", "-bse2",
                                             *forced, "--", str(path)],
                                            capture_output=True, timeout=10)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")
                    self.assertEqual(path.read_bytes(), image)


if __name__ == "__main__":
    unittest.main()
