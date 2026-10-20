"""UDF allocation data, sparse holes and EOF through both shipped engines."""
from pathlib import Path
import os
import stat
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
from udf_walk_fixture import CASES as WALK_CASES, MODULE_ONLY as WALK_MODULE, BAD as WALK_BAD
from udf_discovery_fixture import CASES as DISCOVERY_CASES, MODULE_ONLY as DISCOVERY_MODULE, BAD as DISCOVERY_BAD
from udf_names_fixture import CASES as NAME_CASES, BAD as NAME_BAD, COMMENTS
from udf_inode_fixture import CASES as INODE_CASES, BAD as INODE_BAD, MODULE_BAD as INODE_MODULE_BAD, METADATA_ONLY, attributes
from udf_directory_fixture import CASES as DIRECTORY_CASES, BAD as DIRECTORY_BAD, MODULE_BAD as DIRECTORY_MODULE_BAD
from udf_maps_fixture import CASES as MAP_CASES, BAD as MAP_BAD, MODULE_BAD as MAP_MODULE_BAD
from udf_cache_fixture import CASES as CACHE_CASES, BAD as CACHE_BAD, MODULE_BAD as CACHE_MODULE_BAD, METADATA_ONLY as CACHE_METADATA

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

    def run_image(self, engine, image, name=None, listing=False, forced=True):
        with tempfile.TemporaryDirectory(prefix="sevenzip-udf-") as temporary:
            path = Path(temporary) / ("образ 空.udf" if forced else "образ 空.bin")
            path.write_bytes(image)
            args = [str(self.runtime / (engine + SUFFIX)), "l" if listing else "x" if name else "t", "-sccUTF-8", "-bsp0"]
            if forced:
                args += ["-tUdf"]
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
        for kind in (*CONTINUATIONS, *ICB_CASES, *SPARING_CASES, *VAT_CASES, *METADATA_CASES,
                     *WALK_CASES, *WALK_MODULE, *DISCOVERY_CASES, *DISCOVERY_MODULE, *NAME_CASES,
                     *INODE_CASES, *DIRECTORY_CASES, *MAP_CASES, *CACHE_CASES):
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
                     *METADATA_BAD, *DISCOVERY_BAD, *NAME_BAD, *INODE_BAD, *INODE_MODULE_BAD,
                     *DIRECTORY_BAD, *MAP_BAD, *CACHE_BAD):
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

    def test_unallocated_inode_metadata(self):
        for kind in CACHE_METADATA:
            image, _ = volume(kind)
            with self.subTest(kind=kind):
                result = self.run_image("7z", image, listing=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                alpha = result.stdout.split(b"Path = alpha.bin", 1)[1].split(b"Path = ", 1)[0]
                for property in (b"Size = 0", b"Mode = 0---------", b"Links = 1",
                                 b"User ID = 0", b"Group ID = 0"):
                    self.assertIn(property, alpha)
                result = self.run_image("7z", image, "alpha.bin")
                self.assertEqual(result.returncode, 2)
                self.assertEqual(result.stdout, b"")

    def test_inode_properties(self):
        for kind in (*INODE_CASES, *METADATA_ONLY):
            image, _ = volume(kind)
            with self.subTest(kind=kind):
                result = self.run_image("7z", image, listing=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                records = result.stdout.decode("utf-8").replace("\r\n", "\n").split("----------\n", 1)[1].split("\n\n")
                for name, expected in attributes(kind).items():
                    record = next(record for record in records if record.startswith("Path = " + name + "\n"))
                    self.assertIn("Mode = " + stat.filemode(expected["mode"]), record)
                    for key, label in (("uid", "User ID"), ("gid", "Group ID"), ("links", "Links"),
                                       ("major", "Device Major"), ("minor", "Device Minor")):
                        if key in expected:
                            self.assertIn(label + " = " + str(expected[key]) + "\n", record + "\n")
                if kind in METADATA_ONLY:
                    result = self.run_image("7z", image, "alpha.bin")
                    self.assertEqual(result.returncode, 2)
                    self.assertEqual(result.stdout, b"")

    def test_allocation_cycles_fail_normally(self):
        for kind in (*WALK_BAD, *DIRECTORY_MODULE_BAD):
            image, _ = volume(kind)
            with self.subTest(kind=kind):
                # 7-Zip's normal archive error, rather than timeout, crash or out-of-memory.
                self.assertEqual(self.run_image("7z", image).returncode, 2)

    def test_unsafe_inputs_fail_normally(self):
        for kind in (*MAP_MODULE_BAD, *CACHE_MODULE_BAD):
            image, _ = volume(kind)
            with self.subTest(kind=kind):
                self.assertEqual(self.run_image("7z", image).returncode, 2)

    def test_namespace_listing(self):
        for kind in NAME_CASES:
            image, files = volume(kind)
            with self.subTest(kind=kind):
                result = self.run_image("7z", image, listing=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                text = result.stdout.decode("utf-8").replace("\r\n", "\n")
                records = text.split("----------\n", 1)[1].split("\n\n")
                paths = {line[7:].replace("\\", "/") for record in records
                         for line in record.splitlines() if line.startswith("Path = ")}
                self.assertEqual(paths - {"nested"}, set(files))
                if kind in COMMENTS:
                    self.assertIn("VolumeId: " + COMMENTS[kind] + "\n", text)
                if kind.startswith("ns-link-"):
                    alpha = next(record for record in records if "Path = alpha.bin\n" in record)
                    self.assertIn("Symbolic Link = " + files["alpha.bin"].decode(), alpha)
                    self.assertIn("Size = " + str(len(files["alpha.bin"])), alpha)

    def test_directory_listing(self):
        for kind in DIRECTORY_CASES:
            image, files = volume(kind)
            with self.subTest(kind=kind):
                result = self.run_image("7z", image, listing=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                text = result.stdout.decode("utf-8").replace("\r\n", "\n")
                records = text.partition("----------\n")[2].split("\n\n")
                paths = {line[7:].replace("\\", "/") for record in records
                         for line in record.splitlines() if line.startswith("Path = ")}
                self.assertEqual(paths - {"nested"}, set(files))
                if "nested/tail.bin" in files:
                    nested = next(record for record in records if record.startswith("Path = nested\n"))
                    self.assertIn("Folder = +", nested)

    @unittest.skipIf(sys.platform == "win32", "native symlink creation requires Windows privilege")
    def test_symlink_extraction(self):
        for kind in ("ns-link-inline", "ns-link-external", "ns-link-chain"):
            image, _ = volume(kind)
            with self.subTest(kind=kind), tempfile.TemporaryDirectory(prefix="sevenzip-udf-links-") as td:
                root = Path(td)
                source = root / "образ 空.udf"
                source.write_bytes(image)
                output = root / "выход 空"
                result = subprocess.run([str(self.runtime / ("7z" + SUFFIX)), "x", "-tUdf", "-y",
                                         "-o" + str(output), "--", str(source)], capture_output=True, timeout=10)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                link = output / "alpha.bin"
                self.assertTrue(link.is_symlink())
                self.assertEqual(os.readlink(link), "nested/tail.bin")
                self.assertEqual(link.read_bytes(), b"directory payload")
                self.assertEqual(source.read_bytes(), image)

    def test_discovery_without_format_or_extension_hint(self):
        for kind in ("ds-block-512", "ds-block-1024", "ds-block-4096", "ds-block-4096-spaced",
                     "ds-backup-last", "ds-backup-middle", "ds-open-512", "ds-nsr03",
                     "ds-direct-nsr", "ds-vrs-type-version", "ds-main-reserve", "ds-prevailing-lvd"):
            image, files = volume(kind)
            with self.subTest(kind=kind):
                result = self.run_image("7z", image, listing=True, forced=False)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn(b"Type = Udf", result.stdout)
                result = self.run_image("7z", image, "alpha.bin", forced=False)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout, files["alpha.bin"])


if __name__ == "__main__":
    unittest.main()
