"""APM geometry/discovery, exact extraction and damaged-map failures."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from apm_fixture import CASES, BAD, volume

RUNTIME = Path(sys.argv.pop(1)).resolve()
SUFFIX = ".exe" if sys.platform == "win32" else ""


class ApmTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-apm-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        for name in ("7z" + SUFFIX, "7zz" + SUFFIX,
                     "7z.dll" if sys.platform == "win32" else "7z.so"):
            shutil.copy2(RUNTIME / name, cls.runtime / name)

    def run_image(self, engine, image, listing=False, forced=True):
        with tempfile.TemporaryDirectory(prefix="sevenzip-apm-") as temporary:
            path = Path(temporary) / "карта 空.bin"
            path.write_bytes(image)
            args = [str(self.runtime / (engine + SUFFIX)), "l" if listing else "x",
                    "-sccUTF-8", "-bsp0"]
            args += (["-t*", "-slt"] if listing else
                     ["-tAPM" if forced else "-t*", "-so", "-bso0", "-bse2"])
            args += ["--", str(path)]
            if not listing:
                args += ["data.hfs"]
            result = subprocess.run(args, capture_output=True, timeout=10)
            self.assertEqual(path.read_bytes(), image)
            return result

    def test_geometry_and_payload(self):
        for kind in CASES:
            image, expected, block = volume(kind)
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    result = self.run_image(engine, image)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, expected)
                    result = self.run_image(engine, image, listing=True)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    self.assertIn(b"Type = APM", result.stdout)
                    self.assertIn(("Cluster Size = " + str(block)).encode(), result.stdout)
                    if expected:
                        self.assertIn(b"Path = data.hfs", result.stdout)
                    else:
                        self.assertNotIn(b"Path = data.hfs", result.stdout)

    def test_automatic_partition_extraction(self):
        image, expected, _ = volume("block-512")
        for engine in ("7z", "7zz"):
            with self.subTest(engine=engine):
                result = self.run_image(engine, image, forced=False)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(result.stdout, expected)

    def test_invalid_maps_fail_normally(self):
        for kind in BAD:
            image, _, _ = volume(kind)
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    self.assertEqual(self.run_image(engine, image).returncode, 2)


if __name__ == "__main__":
    unittest.main()
