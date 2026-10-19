"""UDF allocation data, sparse holes and EOF through both shipped engines."""
from pathlib import Path
import subprocess
import shutil
import sys
import tempfile
import unittest
from udf_fixture import CASES, CONTINUATIONS, volume
from udf_icb_fixture import CASES as ICB_CASES, BAD as ICB_BAD
from udf_sparing_fixture import CASES as SPARING_CASES, BAD as SPARING_BAD
from udf_vat_fixture import CASES as VAT_CASES, BAD as VAT_BAD
from udf_metadata_fixture import CASES as METADATA_CASES, BAD as METADATA_BAD

RUNTIME = Path(sys.argv.pop(1))
MODULE = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""


class UdfTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-udf-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        for name in ("7z" + SUFFIX, "7zz" + SUFFIX,
                     "7z.dll" if sys.platform == "win32" else "7z.so"):
            shutil.copy2(RUNTIME / name, cls.runtime / name)
        (cls.runtime / "Formats").mkdir()
        shutil.copy2(MODULE, cls.runtime / "Formats" / MODULE.name)

    def run_image(self, engine, image, name=None, listing=False):
        with tempfile.TemporaryDirectory(prefix="sevenzip-udf-") as temporary:
            path = Path(temporary) / "образ 空.udf"
            path.write_bytes(image)
            args = [str(self.runtime / (engine + SUFFIX)), "l" if listing else "x" if name else "t", "-tUdf", "-sccUTF-8", "-bsp0"]
            if listing:
                args += ["-slt"]
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
                     "inline-length-mismatch"):
            image, _ = volume(kind)
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    self.assertNotEqual(self.run_image(engine, image).returncode, 0)

    def test_continuations(self):
        for kind in (*CONTINUATIONS, *ICB_CASES, *SPARING_CASES, *VAT_CASES, *METADATA_CASES):
            image, files = volume(kind)
            with self.subTest(kind=kind):
                result = self.run_image("7z", image)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                for name, expected in files.items():
                    result = self.run_image("7z", image, name)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, expected, name)

    def test_standalone_metadata_bootstrap_bounds(self):
        for kind in ("metadata-hole", "metadata-outside"):
            image, _ = volume(kind)
            with self.subTest(kind=kind):
                self.assertNotEqual(self.run_image("7zz", image).returncode, 0)

    def test_damaged_continuation_chains(self):
        for kind in ("ad-cycle", "ad-limit-17", "ad-outside", "ad-bad-partition",
                     "ad-overflow", "ad-over-block", *ICB_BAD, *SPARING_BAD, *VAT_BAD,
                     *METADATA_BAD):
            image, _ = volume(kind)
            with self.subTest(kind=kind):
                self.assertNotEqual(self.run_image("7z", image).returncode, 0)

    def test_selected_inode_metadata(self):
        for kind in ("icb-file", "icb-efe", "icb-cross-partition"):
            image, _ = volume(kind)
            with self.subTest(kind=kind):
                result = self.run_image("7z", image, listing=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                alpha = result.stdout.split(b"Path = alpha.bin", 1)[1].split(b"Path = ", 1)[0]
                self.assertIn(b"Modified = 2041-", alpha)


if __name__ == "__main__":
    unittest.main()
