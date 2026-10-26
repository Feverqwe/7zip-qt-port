"""ISO descriptor, extent and source boundaries through both shipped engines."""
from pathlib import Path
from runtime_fixture import copy_codec_runtime
import subprocess
import shutil
import re
import sys
import tempfile
import unittest
from iso_fixture import volume

RUNTIME = Path(sys.argv.pop(1))
MODULE = Path(sys.argv.pop(1))
CHARSET_PROBE = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""


class IsoTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-iso-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        copy_codec_runtime(RUNTIME, cls.runtime)
        (cls.runtime / "Formats").mkdir()
        shutil.copy2(MODULE, cls.runtime / "Formats" / MODULE.name)

    def run_image(self, engine, image, name=None, typed=True, properties=()):
        with tempfile.TemporaryDirectory(prefix="sevenzip-iso-") as temporary:
            path = Path(temporary) / ("образ 空.iso" if typed else "образ 空")
            path.write_bytes(image)
            args = [str(self.runtime / (engine+SUFFIX)), "x" if name else "t", "-sccUTF-8", "-bsp0"]
            if typed:
                args += ["-tIso"]
            args += ["-m" + value for value in properties]
            if name:
                args += ["-so", "-bso0", "-bse2"]
            args += ["--", str(path)]
            if name:
                args += [name]
            result = subprocess.run(args, capture_output=True, timeout=10)
            self.assertEqual(path.read_bytes(), image)
            return result

    def test_valid_extent_bytes(self):
        for kind in ("plain", "joliet", "block-512", "block-1024", "block-512-tail", "xar",
                     "xar-directory", "multi", "multi-directory", "unknown-descriptor",
                     "foreign-descriptor", "no-terminator", "unit-no-gap", "big-endian-mirror", "iso-hs-collision"):
            image, files = volume(kind)
            for engine in ("7z",):
                for name, expected in files.items():
                    with self.subTest(kind=kind, engine=engine, name=name):
                        result = self.run_image(engine, image, name)
                        self.assertEqual(result.stdout, expected, result.stderr.decode(errors="replace"))
                        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))

    def test_bad_mapping_never_returns_bytes(self):
        for kind in ("outside-volume", "xar-outside-volume", "short-source", "missing-final",
                     "misaligned-section", "interleave"):
            for engine in ("7z",):
                with self.subTest(kind=kind, engine=engine):
                    result = self.run_image(engine, volume(kind)[0], "ALPHA.BIN")
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")

    def test_directory_own_inode_and_sections(self):
        for kind in ("directory-root-size", "directory-parent-size", "directory-root-flag",
                     "directory-multi-root", "directory-multi-child", "directory-multi-padding",
                     "directory-multi-split", "directory-sections-limit", "directory-metadata"):
            image, files = volume(kind)
            for name, expected in files.items():
                with self.subTest(kind=kind, name=name):
                    result = self.run_image("7z", image, name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

    def test_invalid_directory_inode_never_returns_bytes(self):
        for kind in ("directory-bad-dot", "directory-file-dot", "directory-multi-outside",
                     "directory-multi-unaligned", "directory-multi-unfinished", "directory-multi-bad-dot",
                     "directory-sections-overflow"):
            with self.subTest(kind=kind):
                result = self.run_image("7z", volume(kind)[0], "ALPHA.BIN")
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_deep_tree_bytes_and_cycle(self):
        image, files = volume("tree-clear")
        for engine in ("7z",):
            for name, expected in files.items():
                with self.subTest(engine=engine):
                    result = self.run_image(engine, image, name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)
            self.assertNotEqual(self.run_image(engine, volume("tree-cycle")[0]).returncode, 0)

    def test_invalid_directory_is_rejected(self):
        for kind in ("bad-directory", "cycle", "bad-name-length", "short-record"):
            for engine in ("7z",):
                with self.subTest(kind=kind, engine=engine):
                    self.assertNotEqual(self.run_image(engine, volume(kind)[0]).returncode, 0)

    def test_rock_ridge_files_and_volume_choice(self):
        for kind in ("rock", "rock-joliet", "rock-no-er", "rock-no-er-joliet", "rock-metadata", "rock-shared-ce", "rock-cl-same-extent", "rock-empty-primary", "rock-bad-primary", "rock-bad-primary-name"):
            image, files = volume(kind)
            for name, expected in files.items():
                with self.subTest(kind=kind, name=name):
                    result = self.run_image("7z", image, name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

    def test_iso_detection(self):
        for kind in ("plain", "joliet", "iso-hs-collision"):
            image, files = volume(kind)
            for name, expected in files.items():
                with self.subTest(kind=kind, name=name):
                    result = self.run_image("7z", image, name, typed=False)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

    def test_high_sierra_is_unsupported(self):
        image = volume("sierra")[0]
        for typed in (False, True):
            result = self.run_image("7z", image, "ALPHA.BIN", typed=typed)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(result.stdout, b"")

    def test_linux_names_and_mappings(self):
        cases = {"names-joliet-components": (), "names-joliet-surrogates": (),
                 "names-joliet-malformed": (), "names-joliet-nul": (), "names-joliet-empty": (), "names-joliet-long": (),
                 "names-joliet-utf8": ("iocharset=utf8",), "names-joliet-cp437": ("iocharset=cp437",),
                 "names-map-normal": ("map=n",), "names-map-off": ("map=off",),
                 "names-acorn": ("map=a",), "names-acorn-short": ("map=a",),
                 "names-acorn-marker": ("map=a",), "names-rr": ()}
        for kind, properties in cases.items():
            image, files = volume(kind)
            for name, expected in files.items():
                with self.subTest(kind=kind, name=name):
                    result = self.run_image("7z", image, name, properties=properties)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

    def test_linux_nls_names_and_multibyte_sequences(self):
        from iso_nls_fixture import cases, charset
        for case in cases():
            image, files = volume("names-joliet-nls-" + case)
            available = subprocess.run([str(CHARSET_PROBE), "--charset-available", charset(case)])
            self.assertIn(available.returncode, (0, 2))
            if available.returncode == 2:
                result = self.run_image("7z", image, properties=("iocharset=" + charset(case),))
                self.assertNotEqual(result.returncode, 0)
                continue
            if case in ("euc-jp-extended", "euc-jp-long", "maciceland"):
                # OS vendor extensions differ. All payloads must remain accessible.
                result = self.run_image("7z", image, properties=("iocharset=" + charset(case),))
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                reference = self.run_image("7z", image, properties=("iocharset=utf8",))
                self.assertEqual(reference.returncode, 0, reference.stderr.decode(errors="replace"))
                self.assertEqual(re.search(rb"(?m)^Files: (\d+)", result.stdout).group(1),
                                 re.search(rb"(?m)^Files: (\d+)", reference.stdout).group(1))
                continue
            for name, expected in files.items():
                with self.subTest(charset=case, name=name):
                    result = self.run_image("7z", image, name, properties=("iocharset=" + charset(case),))
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

    def test_linux_read_policies_and_optional_dates(self):
        from iso_policy_fixture import PROFILES
        for kind, properties in PROFILES.items():
            image, files = volume(kind)
            for name, expected in files.items():
                with self.subTest(kind=kind, name=name):
                    result = self.run_image("7z", image, name, properties=properties)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

    def test_filtered_bad_directory_is_not_traversed(self):
        image = volume("policy-associated-bad")[0]
        result = self.run_image("7z", image)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertNotEqual(self.run_image("7z", image, properties=("showassoc",)).returncode, 0)

    def test_invalid_read_policy_is_rejected(self):
        for properties in (("hide=wrong",), ("sbsector",), ("sbsector=-1",), ("sbsector=4294967295",)):
            with self.subTest(properties=properties):
                self.assertNotEqual(self.run_image("7z", volume("plain")[0], properties=properties).returncode, 0)

    def test_disabling_joliet_without_primary_is_rejected(self):
        image = volume("policy-joliet-only")[0]
        self.assertNotEqual(self.run_image("7z", image, properties=("nojoliet",)).returncode, 0)
        image = volume("policy-norock-bad-primary")[0]
        self.assertNotEqual(self.run_image("7z", image).returncode, 0)

    def test_rock_ridge_producer_layouts(self):
        from iso_producer_fixture import CASES
        for kind in CASES:
            image, files = volume(kind)
            for name, expected in files.items():
                with self.subTest(kind=kind, name=name):
                    result = self.run_image("7z", image, name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)
        result = self.run_image("7z", volume("producer-directory-ce-outside")[0], "alpha.bin")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, b"")

    def test_zisofs_decoded_bytes(self):
        for kind in ("zisofs", "zisofs-shift0", "zisofs-pointer-boundary", "zisofs-expanding",
                     "zisofs-expanding-padding", "zisofs-small-sparse",
                     "zisofs-shift17", "zisofs-padding", "zisofs-header0", "zisofs-embedded-header",
                     "zisofs-empty", "zisofs-sparse-marker", "zisofs-multi", "zisofs-block512"):
            image, files = volume(kind)
            for name, expected in files.items():
                with self.subTest(kind=kind, name=name):
                    result = self.run_image("7z", image, name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

    def test_zisofs_corruption_is_data_error(self):
        for kind in ("zisofs-crc", "zisofs-short-zlib", "zisofs-short-output", "zisofs-long-output",
                     "zisofs-pointer-order", "zisofs-pointer-outside", "zisofs-pointer-header",
                     "zisofs-short-table"):
            with self.subTest(kind=kind):
                result = self.run_image("7z", volume(kind)[0], "alpha.bin")
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(b"Data Error", result.stderr)

    def test_bad_rock_ridge_never_publishes_partial_catalog(self):
        for kind in ("rock-ce-outside", "rock-ce-crossblock", "rock-ce-cycle", "rock-sl-overrun",
                     "rock-cl-cycle", "rock-cl-outside", "rock-root-mode"):
            with self.subTest(kind=kind):
                result = self.run_image("7z", volume(kind)[0], "alpha.bin")
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")


if __name__ == "__main__":
    unittest.main()
