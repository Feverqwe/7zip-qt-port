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

    def run_image(self, engine, data, command="t", name=None, streams=False, case_sensitive=False):
        with tempfile.TemporaryDirectory(dir=self.runtime) as temporary:
            path = Path(temporary) / "образ 空.ntfs"
            path.write_bytes(data)
            args = [str(self.runtime / (engine + SUFFIX)), command, "-tNTFS", "-sccUTF-8", "-bsp0"]
            if case_sensitive:
                args += ["-ssc"]
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

    def test_volume_free_space_in_dynamic_engine(self):
        from ntfs_bitmap_fixture import expected
        for kind in ("bitmap-base", "bitmap-last", "bitmap-fragmented"):
            with self.subTest(kind=kind):
                image, _ = volume(kind)
                listing = self.run_image("7z", image, "l")
                self.assertEqual(listing.returncode, 0, listing.stderr)
                self.assertIn(("Free Space = " + str(expected(kind, image))).encode(),
                              listing.stdout.splitlines())

    def test_directory_indexes_and_unverified_mft_fallback(self):
        from ntfs_directory_fixture import CASES, LIMITED
        for kind in CASES:
            with self.subTest(kind=kind):
                image, files = volume(kind)
                listing = self.run_image("7z", image, "l")
                self.assertEqual(listing.returncode, 0, listing.stderr)
                self.assertEqual(b"Some NTFS file metadata is unavailable or unverified." in
                                 listing.stdout, kind in LIMITED)
                for name, expected in files.items():
                    result = self.run_image("7z", image, "x", name, case_sensitive=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, expected)

    def test_volume_state_and_readable_files(self):
        from ntfs_volume_fixture import CASES, limitation
        for kind in CASES:
            image, files = volume(kind)
            with self.subTest(kind=kind):
                listing = self.run_image("7z", image, "l")
                self.assertEqual(listing.returncode, 0, listing.stderr)
                reason = limitation(kind)
                if reason:
                    self.assertIn(reason.encode(), listing.stdout)
                else:
                    self.assertNotIn(b"Read-only limitations = ", listing.stdout)
                if kind.startswith("volume-list-"):
                    from ntfs_volume_list_fixture import label
                    if label(kind):
                        self.assertIn(label(kind).encode(), listing.stdout)
                result = self.run_image("7z", image, "x", "alpha")
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout, files["alpha"])

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
            image, files = volume(kind)
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    result = self.run_image(engine, image, "x", "gamma")
                    if engine == "7z" and kind != "standard-record-total":
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(result.stdout, files["gamma"])
                        listing = self.run_image(engine, image, "l")
                        self.assertIn(b"Some NTFS file metadata is unavailable or unverified.", listing.stdout)
                    else:
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

    def test_partial_ea_preserves_readable_data(self):
        from ntfs_ea_list_fixture import CASES, LIMITED, LIMITATION
        for kind in (*EA_BAD, *CASES):
            with self.subTest(kind=kind):
                image, files = volume(kind)
                result = self.run_image("7z", image, "l", streams=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                if kind in EA_BAD or kind in LIMITED:
                    self.assertIn(LIMITATION.encode(), result.stdout)
                else:
                    self.assertNotIn(LIMITATION.encode(), result.stdout)
                for name in ("alpha", "gamma"):
                    extracted = self.run_image("7z", image, "x", name)
                    self.assertEqual(extracted.returncode, 0, extracted.stderr)
                    self.assertEqual(extracted.stdout, files[name])

    def test_data_list_authority_and_partial_copy_results(self):
        from ntfs_data_list_fixture import CASES, LIMITED, PARTIAL, UNAVAILABLE, LIMITATION
        for kind in CASES:
            with self.subTest(kind=kind):
                image, files = volume(kind)
                listing = self.run_image("7z", image, "l", streams=True)
                self.assertEqual(listing.returncode, 0, listing.stderr)
                self.assertEqual(LIMITATION.encode() in listing.stdout, kind in LIMITED)
                for name, expected in files.items():
                    if name != "alpha" and not name.startswith("gamma"):
                        continue
                    result = self.run_image("7z", image, "x", name, case_sensitive=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, expected)
                if kind in PARTIAL or kind in UNAVAILABLE:
                    result = self.run_image("7z", image, "x", "gamma")
                    self.assertNotEqual(result.returncode, 0)
                    prefix = (bytes(4096) if kind == "data-list-sparse-middle" else
                              b"A" * 4096 if kind in ("data-list-missing-middle", "data-list-no-list-tail") else b"")
                    self.assertEqual(result.stdout, prefix)
                if kind in ("data-list-ads-bad-id", "data-list-nul-attribute"):
                    result = self.run_image("7z", image, "x", "gamma:side", case_sensitive=True)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")

    def test_file_metadata_membership_and_recovery_names(self):
        from ntfs_file_metadata_fixture import CASES, LIMITED, LIMITATION
        for kind in CASES:
            with self.subTest(kind=kind):
                image, files = volume(kind)
                listing = self.run_image("7z", image, "l")
                self.assertEqual(listing.returncode, 0, listing.stderr)
                self.assertEqual(LIMITATION.encode() in listing.stdout, kind in LIMITED)
                for name, expected in files.items():
                    result = self.run_image("7z", image, "x", name, case_sensitive=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, expected)

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

    def test_reparse_attribute_list_selection_and_partial_read(self):
        from ntfs_reparse_list_fixture import CASES, BAD, UNREADABLE, LIMITATION
        for kind in CASES:
            with self.subTest(kind=kind):
                image, files = volume(kind)
                listed = self.run_image("7z", image, "l")
                self.assertEqual(listed.returncode, 0, listed.stderr)
                self.assertEqual(LIMITATION.encode() in listed.stdout, kind in BAD)
                gamma = self.run_image("7z", image, "x", "gamma")
                if kind in UNREADABLE:
                    self.assertNotEqual(gamma.returncode, 0)
                    self.assertEqual(gamma.stdout, b"")
                else:
                    self.assertEqual(gamma.returncode, 0, gamma.stderr)
                    self.assertEqual(gamma.stdout, files["gamma"])
                alpha = self.run_image("7z", image, "x", "alpha")
                self.assertEqual(alpha.returncode, 0, alpha.stderr)
                self.assertEqual(alpha.stdout, files["alpha"])
                side = self.run_image("7z", image, "x", "gamma:side")
                self.assertEqual(side.returncode, 0, side.stderr)
                self.assertEqual(side.stdout, files["gamma:side"])

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
