"""Read results and rejection of malformed NTFS through both delivered engines."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
import zlib
from ntfs_fixture import volume
from ntfs_ea_fixture import VALID as EA_VALID, BAD as EA_BAD
from ntfs_link_fixture import VALID as LINK_VALID, UNSUPPORTED as LINK_UNSUPPORTED, path as link_path
from ntfs_standard_fixture import VALID as STANDARD_VALID, BAD as STANDARD_BAD

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

    def run_image(self, engine, data, command="t", name=None, streams=False):
        with tempfile.TemporaryDirectory(dir=self.runtime) as temporary:
            path = Path(temporary) / "образ 空.ntfs"
            path.write_bytes(data)
            args = [str(self.runtime / (engine + SUFFIX)), command, "-tNTFS", "-sccUTF-8", "-bsp0"]
            if streams or (name and ":" in name):
                args += ["-sns"]
            if command == "x":
                args += ["-so", "-bso0", "-bse2"]
                if name and ":" not in name:
                    args += ["-x!" + name + ":*"]  # Keep the selected unnamed stream separate from ADS.
            args += ["--", str(path)]
            if name:
                args.append(name)
            result = subprocess.run(args, capture_output=True, timeout=10)
            self.assertEqual(path.read_bytes(), data)
            return result

    def test_standard_information_layout_and_presence_in_both_engines(self):
        for kind in STANDARD_VALID:
            image, files = volume(kind)
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    name = "gamma" if "gamma" in files else "alpha"
                    result = self.run_image(engine, image, "x", name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, files[name])
        for kind in STANDARD_BAD:
            image, _ = volume(kind)
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    result = self.run_image(engine, image, "x", "gamma")
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")

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

    def test_linux_ea_values_and_ads_namespaces(self):
        for kind in ("ea-xattrs", "ea-xattrs-fragmented", "ea-xattrs-extension", "ea-xattrs-collision"):
            image, files = volume(kind)
            listing = self.run_image("7z", image, "l", streams=True)
            self.assertEqual(listing.returncode, 0, listing.stderr)
            for name, expected in files.items():
                if not name.startswith("gamma:"):
                    continue
                with self.subTest(kind=kind, name=name):
                    self.assertIn(name.encode(), listing.stdout)
                    result = self.run_image("7z", image, "x", name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

    def test_linux_ea_metadata_and_payload(self):
        for kind in EA_VALID:
            with self.subTest(kind=kind):
                image, files = volume(kind)
                listing = self.run_image("7z", image, "l")
                self.assertEqual(listing.returncode, 0, listing.stderr)
                result = self.run_image("7z", image, "x", "gamma")
                if "gamma" in files:
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, files["gamma"])
                else:
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")

    def test_corrupt_ea_rejects_open(self):
        for kind in EA_BAD:
            with self.subTest(kind=kind):
                result = self.run_image("7z", volume(kind)[0], "l")
                self.assertNotEqual(result.returncode, 0, result.stdout)

    def test_linux_wof_and_fragmented_mft(self):
        for kind in ("mft-list", "mft-list-nonresident", "wof-xpress4k", "wof-xpress8k",
                     "wof-xpress16k", "wof-lzx32k", "wof-xpress4k-resident", "wof-lzx32k-resident"):
            image, files = volume(kind)
            for name in ("alpha", "данные 空", "gamma"):
                with self.subTest(kind=kind, name=name):
                    result = self.run_image("7z", image, "x", name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, files[name])

    def test_wof_corrupt_reads_report_failure(self):
        for kind in ("wof-xpress4k-bad-frame", "wof-lzx32k-bad-frame", "wof-lzx32k-bad-table",
                     "wof-xpress4k-short-ads"):
            with self.subTest(kind=kind):
                result = self.run_image("7z", volume(kind)[0], "x", "gamma")
                self.assertNotEqual(result.returncode, 0)

    def test_linux_reparse_links(self):
        for kind in LINK_VALID:
            with self.subTest(kind=kind):
                image, files = volume(kind)
                name = link_path(kind)
                result = self.run_image("7z", image, "x", name)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout, files[name])
                checked = self.run_image("7z", image, name=name)
                self.assertEqual(checked.returncode, 0, checked.stderr)
                if kind == "link-ads":
                    ads = self.run_image("7z", image, "x", "gamma:side")
                    self.assertEqual(ads.returncode, 0, ads.stderr)
                    self.assertEqual(ads.stdout, files["gamma:side"])

    def test_unsupported_reparse_never_returns_unnamed_data(self):
        for kind in LINK_UNSUPPORTED:
            with self.subTest(kind=kind):
                result = self.run_image("7z", volume(kind)[0], "x", "gamma")
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    @unittest.skipIf(sys.platform == "win32", "Native Windows link creation requires a privileged token")
    def test_reparse_extraction_creates_a_link(self):
        with tempfile.TemporaryDirectory(dir=self.runtime) as temporary:
            root = Path(temporary)
            image = root / "образ 空.ntfs"
            data, _ = volume("link-symlink")
            image.write_bytes(data)
            result = subprocess.run([str(self.runtime / ("7z" + SUFFIX)), "x", "-tNTFS", "-y",
                                     "-o" + str(root / "output"), "--", str(image), "gamma"],
                                    capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr)
            link = root / "output" / "gamma"
            self.assertTrue(link.is_symlink())
            self.assertEqual(str(link.readlink()), "alpha")
            self.assertEqual(image.read_bytes(), data)

    def test_standalone_refuses_missing_linux_read_features(self):
        for kind in ("mft-list", "wof-xpress4k", "wof-lzx32k"):
            with self.subTest(kind=kind):
                result = self.run_image("7zz", volume(kind)[0], "x", "gamma")
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_wof_unknown_initialized_data_is_unsupported(self):
        for kind in ("wof-xpress4k-unknown-initialized", "wof-xpress4k-unknown-high-initialized",
                     "wof-xpress4k-unknown-max-initialized", "wof-xpress4k-initialized-missing-ads",
                     "wof-xpress4k-initialized-bad-map", "wof-lzx32k-initialized-encrypted-ads"):
            with self.subTest(kind=kind):
                result = self.run_image("7z", volume(kind)[0], "x", "gamma")
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")
        image, files = volume("wof-xpress4k-unknown-uninitialized")
        result = self.run_image("7z", image, "x", "gamma")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, files["gamma"])

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
