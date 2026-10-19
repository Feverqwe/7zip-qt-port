"""UDF allocation data, sparse holes and EOF through both shipped engines."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from udf_fixture import CASES, volume

RUNTIME = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""


class UdfTests(unittest.TestCase):
    def run_image(self, engine, image, name=None):
        with tempfile.TemporaryDirectory(prefix="sevenzip-udf-") as temporary:
            path = Path(temporary) / "образ 空.udf"
            path.write_bytes(image)
            args = [str(RUNTIME / (engine + SUFFIX)), "x" if name else "t", "-tUdf", "-sccUTF-8", "-bsp0"]
            if name:
                args += ["-so", "-bso0", "-bse2"]
            result = subprocess.run(args + ["--", str(path)] + ([name] if name else []),
                                    capture_output=True, timeout=10)
            self.assertEqual(path.read_bytes(), image)
            return result

    def test_allocation_bytes(self):
        for kind in CASES:
            image, files = volume(kind)
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    result = self.run_image(engine, image)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    for name, expected in files.items():
                        result = self.run_image(engine, image, name)
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(result.stdout, expected, name)

    def test_malformed_allocations(self):
        for kind in ("recorded-outside", "bad-partition", "continuation",
                     "inline-length-mismatch", "metadata-hole", "metadata-outside"):
            image, _ = volume(kind)
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    self.assertNotEqual(self.run_image(engine, image).returncode, 0)


if __name__ == "__main__":
    unittest.main()
