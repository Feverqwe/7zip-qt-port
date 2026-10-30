"""Read results and rejection of malformed NTFS through the delivered dynamic engine."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[3]))
from pathlib import Path
from tests.archive.support.runtime_fixture import copy_codec_runtime
import shutil
import subprocess
import sys
import tempfile
import unittest
import zlib
from tests.formats.ntfs.support.ntfs_fixture import volume
from tests.formats.ntfs.support.ntfs_ea_fixture import VALID as EA_VALID, BAD as EA_BAD
from tests.formats.ntfs.support.ntfs_link_fixture import VALID as LINK_VALID, UNSUPPORTED as LINK_UNSUPPORTED, path as link_path
from tests.formats.ntfs.support.ntfs_standard_fixture import VALID as STANDARD_VALID, BAD as STANDARD_BAD

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
        copy_codec_runtime(RUNTIME, cls.runtime)
        (cls.runtime / "Formats").mkdir()
        shutil.copy2(MODULE, cls.runtime / "Formats" / MODULE.name)

    def run_image(self, engine, data, command="t", name=None, streams=False, case_sensitive=False, deleted=False):
        with tempfile.TemporaryDirectory(dir=self.runtime) as temporary:
            path = Path(temporary) / "образ 空.ntfs"
            path.write_bytes(data)
            args = [str(self.runtime / (engine + SUFFIX)), command, "-tNTFS", "-sccUTF-8", "-bsp0"]
            if deleted:
                args += ["-mld=on"]
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
        from tests.formats.ntfs.support.ntfs_bitmap_fixture import expected
        from tests.formats.ntfs.support.ntfs_bitmap_fixture import VALID, BAD
        for kind in (*VALID, *BAD):
            with self.subTest(kind=kind):
                image, files = volume(kind)
                listing = self.run_image("7z", image, "l")
                self.assertEqual(listing.returncode, 0, listing.stderr)
                free = expected(kind, image)
                if free is None:
                    self.assertFalse(any(line.startswith(b"Free Space = ")
                                         for line in listing.stdout.splitlines()))
                    result = self.run_image("7z", image, "x", "alpha")
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, files["alpha"])
                else:
                    self.assertIn(("Free Space = " + str(free)).encode(), listing.stdout.splitlines())

    def test_directory_indexes_and_unverified_mft_fallback(self):
        from tests.formats.ntfs.support.ntfs_directory_fixture import CASES, LIMITED
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
        from tests.formats.ntfs.support.ntfs_volume_fixture import CASES, limitation
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
                    from tests.formats.ntfs.support.ntfs_volume_list_fixture import label
                    if label(kind):
                        self.assertIn(label(kind).encode(), listing.stdout)
                result = self.run_image("7z", image, "x", "alpha")
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout, files["alpha"])

    def test_standard_information_layout_and_presence_in_both_engines(self):
        for kind in STANDARD_VALID:
            image, files = volume(kind)
            for engine in ("7z",):
                with self.subTest(kind=kind, engine=engine):
                    name = "gamma" if "gamma" in files else "alpha"
                    result = self.run_image(engine, image, "x", name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, files[name])
        for kind in STANDARD_BAD:
            image, files = volume(kind)
            for engine in ("7z",):
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

    def test_mft_record_failures_do_not_hide_independent_files(self):
        from tests.formats.ntfs.support.ntfs_record_fixture import CASES, unavailable
        for kind in CASES:
            with self.subTest(kind=kind):
                image, files = volume(kind)
                listing = self.run_image("7z", image, "l")
                self.assertEqual(listing.returncode, 0, listing.stderr)
                self.assertEqual(b"NTFS MFT records" in listing.stdout, kind != "record-base")
                for name, expected in files.items():
                    result = self.run_image("7z", image, "x", name, case_sensitive=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, expected)
                for name in unavailable(kind):
                    result = self.run_image("7z", image, "x", name, case_sensitive=True)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")

    def test_unavailable_security_metadata_keeps_file_data_readable(self):
        for kind in ("security-root-offset", "security-sd-short", "security-partial-fixup", "security-list-bad-id", "security-sds-map-missing-tail"):
            with self.subTest(kind=kind):
                image, files = volume(kind)
                listing = self.run_image("7z", image, "l")
                self.assertEqual(listing.returncode, 0, listing.stderr)
                self.assertIn(b"NTFS security descriptors", listing.stdout)
                for name in ("alpha", "gamma"):
                    result = self.run_image("7z", image, "x", name)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, files[name])

    def test_short_sources_and_boot_copies_keep_usable_files(self):
        from tests.formats.ntfs.support.ntfs_source_fixture import CASES, FATAL
        _, complete = volume()
        for kind in CASES:
            with self.subTest(kind=kind):
                image, files = volume(kind)
                listing = self.run_image("7z", image, "l")
                if kind in FATAL:
                    self.assertNotEqual(listing.returncode, 0)
                    continue
                self.assertEqual(listing.returncode, 0, listing.stderr)
                self.assertEqual(b"Read-only limitations = " in listing.stdout, kind != "source-no-backup")
                for name, expected in files.items():
                    result = self.run_image("7z", image, "x", name)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, expected)
                for name in ("alpha", "gamma"):
                    if name in files:
                        continue
                    result = self.run_image("7z", image, "x", name)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertTrue(complete[name].startswith(result.stdout))
                    self.assertLess(len(result.stdout), len(complete[name]))

    def test_deleted_metadata_does_not_hide_live_files(self):
        from tests.formats.ntfs.support.ntfs_deleted_fixture import CASES, UNREADABLE, volume as deleted_volume
        for kind in CASES:
            with self.subTest(kind=kind):
                image, active, recovered = deleted_volume(kind)
                for name, expected in active.items():
                    result = self.run_image("7z", image, "x", name)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, expected)
                listing = self.run_image("7z", image, "l", deleted=True)
                self.assertEqual(listing.returncode, 0, listing.stderr)
                if kind != "deleted-orphan-extension":
                    self.assertIn(b"Deleted NTFS records", listing.stdout)
                for name, expected in recovered.items():
                    # The common 7-Zip opening layer prefixes recovery paths.
                    result = self.run_image("7z", image, "x", "[DELETED]/" + name, deleted=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, expected)
                if kind in UNREADABLE:
                    result = self.run_image("7z", image, "x", "[DELETED]/gamma", deleted=True)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")

    def test_classic_read_paths(self):
        for kind in ("base", "sparse", "hole-first", "initialized", "compressed", "sector4k", "boot-reserved", "backup-boot"):
            image, files = volume(kind)
            for engine in ("7z",):
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

    def test_runlist_sentinel_overflow_and_signed_deltas(self):
        for kind in ("run-sentinel-overflow", "run-sentinel", "run-negative", "hole-first"):
            image, files = volume(kind)
            for engine in ("7z",):
                with self.subTest(kind=kind, engine=engine):
                    result = self.run_image(engine, image, "x", "alpha")
                    if "alpha" in files:
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(result.stdout, files["alpha"])
                    else:
                        self.assertNotEqual(result.returncode, 0)
                        self.assertEqual(result.stdout, b"")
                    for name in ("gamma", "данные 空"):
                        result = self.run_image(engine, image, "x", name)
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(result.stdout, files[name])

    def test_mft_bootstrap_uses_only_unnamed_data(self):
        for kind in ("mft-list-named-only", "mft-list-nul-name", "mft-list-named-decoy",
                     "mft-list-nul-decoy", "mft-list-nul-extension"):
            image, files = volume(kind)
            for engine in ("7z",):
                with self.subTest(kind=kind, engine=engine):
                    result = self.run_image(engine, image, "x", "alpha")
                    if kind.endswith("decoy") or engine == "7z":
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(result.stdout, files["alpha"])
                    else:
                        self.assertNotEqual(result.returncode, 0)
                        self.assertEqual(result.stdout, b"")

    def test_bootstrap_recovery_preserves_readable_files(self):
        from tests.formats.ntfs.support.ntfs_recovery_fixture import CASES, FATAL
        for kind in CASES:
            if kind.endswith("-io"):
                continue  # Range-specific read errors use the archive ABI input fixture.
            with self.subTest(kind=kind):
                image, files = volume(kind)
                listing = self.run_image("7z", image, "l")
                if kind in FATAL:
                    self.assertNotEqual(listing.returncode, 0)
                    continue
                self.assertEqual(listing.returncode, 0, listing.stderr)
                self.assertEqual(b"Read-only limitations" in listing.stdout,
                                 kind != "recovery-primary-control")
                for name, expected in files.items():
                    result = self.run_image("7z", image, "x", name, case_sensitive=True)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, expected)
        image, _ = volume("recovery-list-partial")
        result = self.run_image("7z", image)
        self.assertEqual(result.returncode, 2)
        self.assertIn(b"Headers Error", result.stdout + result.stderr)

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
        from tests.formats.ntfs.support.ntfs_ea_list_fixture import CASES, LIMITED, LIMITATION
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
        from tests.formats.ntfs.support.ntfs_data_list_fixture import CASES, LIMITED, PARTIAL, UNAVAILABLE, LIMITATION
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
        from tests.formats.ntfs.support.ntfs_file_metadata_fixture import CASES, LIMITED, LIMITATION
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
                     "wof-xpress16k", "wof-lzx32k", "wof-xpress4k-resident", "wof-lzx32k-resident",
                     "wof-xpress4k-reparse-padding", "wof-xpress4k-reparse-padding-unmapped-tail"):
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
        from tests.formats.ntfs.support.ntfs_reparse_list_fixture import CASES, BAD, UNREADABLE, LIMITATION
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
            for engine in ("7z",):
                with self.subTest(kind=kind, engine=engine):
                    result = self.run_image(engine, image, "x", "gamma" if kind.startswith("wof") else "alpha")
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")


if __name__ == "__main__":
    unittest.main()
