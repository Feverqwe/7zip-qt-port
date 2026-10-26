"""Fork bytes, metadata failures and volume boundaries through the dynamic engine."""
from pathlib import Path
from runtime_fixture import copy_codec_runtime
import shutil
import subprocess
import sys
import tempfile
import unittest
from hfs_fixture import volume

RUNTIME = Path(sys.argv.pop(1))
MODULE = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""


class HfsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-hfs-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        core = "7z.dll" if sys.platform == "win32" else "7z.so"
        copy_codec_runtime(RUNTIME, cls.runtime)
        (cls.runtime / "Formats").mkdir()
        shutil.copy2(MODULE, cls.runtime / "Formats" / MODULE.name)

    def run_image(self, engine, data, name=None, listing=False, start_only=False):
        with tempfile.TemporaryDirectory(prefix="sevenzip-hfs-") as temporary:
            path = Path(temporary) / "образ 空.hfs"
            path.write_bytes(data)
            args = [str(self.runtime / (engine + SUFFIX)), "l" if listing else "x" if name else "t",
                    "-tHFS:s0" if start_only else "-tHFS", "-sccUTF-8", "-bsp0"]
            if listing:
                args += ["-slt", "-ba"]
            if name:
                args += ["-so", "-bso0", "-bse2", "-sns" if ":" in name else "-sns-"]
            args += ["--", str(path)]
            if name:
                args += [name]
            result = subprocess.run(args, capture_output=True, timeout=10)
            self.assertEqual(path.read_bytes(), data)
            return result

    def test_linux_inode_metadata_listing(self):
        for kind in ("modern-times", "modern-special"):
            result = self.run_image("7z", volume(kind)[0], listing=True)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
            entries = {}
            for block in result.stdout.decode("utf-8").replace("\r\n", "\n").split("\n\n"):
                fields = dict(line.split(" = ", 1) for line in block.splitlines() if " = " in line)
                if "Path" in fields:
                    entries[fields["Path"].replace('\\', '/')] = fields
            if kind == "modern-times":
                for name in ("alpha", "hard-alpha", "links/second-link", "links"):
                    self.assertTrue(entries["HfsFixture/" + name]["Modified"].startswith("2106-"))
                self.assertTrue(entries["HfsFixture/null-name"]["Modified"].startswith("2040-"))
            else:
                for name, major, minor in (("null-name", 0xabc, 0x54321),
                                            ("slash_name", 0x123, 0xabcde)):
                    self.assertEqual(int(entries["HfsFixture/" + name]["Device Major"]), major)
                    self.assertEqual(int(entries["HfsFixture/" + name]["Device Minor"]), minor)
                for name in ("café", "각"):
                    self.assertFalse(entries["HfsFixture/" + name].get("Device Major"))
                    self.assertFalse(entries["HfsFixture/" + name].get("Device Minor"))

    def test_page_backed_symlink_targets(self):
        from hfs_symlink_fixture import PAYLOADS, target
        for suffix in PAYLOADS:
            kind = "modern-symlink-" + suffix
            with self.subTest(kind=kind):
                result = self.run_image("7z", volume(kind)[0], listing=True)
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                entries = {}
                for block in result.stdout.decode("utf-8").replace("\r\n", "\n").split("\n\n"):
                    fields = dict(line.split(" = ", 1) for line in block.splitlines() if " = " in line)
                    if "Path" in fields:
                        entries[fields["Path"].replace('\\', '/')] = fields
                self.assertEqual(entries["HfsFixture/symlink"].get("Symbolic Link", ""),
                                 target(kind).decode())

    def test_fork_contents(self):
        for kind in ("base", "preallocated", "empty-preallocated", "overflow-leaves", "boot-blocks",
                     "hfsx", "wrapped", "wrapped-hfsx"):
            image, files = volume(kind)
            for engine in ("7z",):
                for name, expected in files.items():
                    with self.subTest(kind=kind, engine=engine, name=name):
                        result = self.run_image(engine, image, name)
                        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                        self.assertEqual(result.stdout, expected)

    def test_linux_embedded_wrappers(self):
        for kind in ("wrapper-nested", "wrapper-nested-hfsx", "wrapper-non-power",
                     "wrapper-backup-version", "wrapper-version-hplus5", "wrapper-version-hfsx4",
                     "wrapper-no-slock", "wrapper-no-spared", "unclean", "free-overflow"):
            image, files = volume(kind)
            for name, expected in files.items():
                with self.subTest(kind=kind, name=name):
                    result = self.run_image("7z", image, name, start_only=True)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)
        for kind in ("wrapper-zero-step",
                     "wrapper-nested-short", "wrapper-backup-signature",
                     "wrapper-version-old", "wrapper-version-new"):
            with self.subTest(kind=kind):
                result = self.run_image("7z", volume(kind)[0], "HfsFixture/alpha", start_only=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_classic_hfs_is_unsupported(self):
        data = volume("classic")[0]
        result = self.run_image("7z", data, "HfsFixture/alpha", start_only=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, b"")

    def test_linux_inline_xattr_records(self):
        name = "HfsFixture/alpha:com.sevenzip.fixture"
        for kind in ("xattr-reserved", "xattr-padding", "xattr-start", "xattr-empty", "xattr-limit",
                     "xattr-large"):
            with self.subTest(kind=kind):
                image, files = volume(kind)
                result = self.run_image("7z", image, name)
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(result.stdout, files[name])
        for kind in ("xattr-short", "xattr-unknown"):
            with self.subTest(kind=kind):
                result = self.run_image("7z", volume(kind)[0], name)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_bad_forks_never_read_appended_bytes(self):
        for kind in ("extent-over-volume", "allocation-tail-over-volume"):
            for engine in ("7z",):
                with self.subTest(kind=kind, engine=engine):
                    result = self.run_image(engine, volume(kind)[0], "HfsFixture/alpha")
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")

    def test_linux_unicode_and_hardlink_bytes(self):
        for kind in ("modern", "modern-nul", "modern-date", "modern-chain", "modern-hfsx",
                     "modern-case", "modern-xattr", "modern-metadata", "modern-binary",
                     "modern-zlib", "modern-lzvn", "modern-hidden-case", "modern-permissions"):
            image, files = volume(kind)
            for name, expected in files.items():
                with self.subTest(kind=kind, name=name):
                    result = self.run_image("7z", image, name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

    def test_missing_and_corrupt_backing_inode_never_succeed(self):
        for kind in ("modern-missing", "modern-bounds", "modern-binary-case"):
            with self.subTest(kind=kind):
                result = self.run_image("7z", volume(kind)[0], "HfsFixture/alpha")
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")



    def test_linux_metadata_and_thread_failures(self):
        for kind in ("modern-invalid-file-mode", "modern-invalid-dir-mode", "modern-thread-parent",
                     "modern-thread-name", "modern-thread-type", "modern-thread-length"):
            with self.subTest(kind=kind):
                self.assertNotEqual(self.run_image("7z", volume(kind)[0]).returncode, 0)

    def test_corrupt_metadata_is_rejected(self):
        for kind in ("metadata-short-map", "leaf-height", "leaf-backlink", "leaf-offset",
                     "leaf-tail", "leaf-count", "root-depth", "key-format",
                     "parent-cycle", "volume-truncated"):
            for engine in ("7z",):
                with self.subTest(kind=kind, engine=engine):
                    self.assertNotEqual(self.run_image(engine, volume(kind)[0]).returncode, 0)


if __name__ == "__main__":
    unittest.main()
