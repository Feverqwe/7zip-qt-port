"""ELF headers, byte orders, sections, payload bounds and native Linux loading."""
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tempfile
import unittest
from elf_fixture import CASES, BAD, NATIVE, volume

RUNTIME = Path(sys.argv.pop(1)).resolve()
SUFFIX = ".exe" if sys.platform == "win32" else ""


class ElfTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-elf-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        for name in ("7z" + SUFFIX, "7zz" + SUFFIX,
                     "7z.dll" if sys.platform == "win32" else "7z.so"):
            shutil.copy2(RUNTIME / name, cls.runtime / name)

    def command(self, engine, path, command, *args, forced=True):
        return subprocess.run(
            [str(self.runtime / (engine + SUFFIX)), command, "-sccUTF-8", "-bsp0",
             "-tELF:s0" if forced else "-t*:s0", *args, "--", str(path)],
            capture_output=True, timeout=10)

    def test_read_and_discover(self):
        for kind in CASES + NATIVE:
            image, metadata = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-elf-") as temporary:
                path = Path(temporary) / "образ 空.bin"
                path.write_bytes(image)
                for engine in ("7z", "7zz"):
                    with self.subTest(kind=kind, engine=engine):
                        # PreArc auto-selection reserves overlays for nested archives.
                        result = self.command(engine, path, "l", "-slt", forced=kind == "tail")
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertIn(b"Type = ELF", result.stdout)
                        selection = [] if metadata["target"] is None else ["-i!" + metadata["target"]]
                        result = self.command(engine, path, "x", "-so", "-bso0", "-bse2", *selection)
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(result.stdout, bytes.fromhex(metadata["payload"]))
                        result = self.command(engine, path, "t")
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)

    def test_malformed_input_fails_normally(self):
        for kind in BAD:
            image, _ = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-elf-invalid-") as temporary:
                path = Path(temporary) / "bad.elf"
                path.write_bytes(image)
                for engine in ("7z", "7zz"):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "t")
                        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)

    @unittest.skipUnless(sys.platform.startswith("linux") and platform.machine() == "x86_64",
                         "Native ELF execution requires the Linux x86-64 runner")
    def test_native_linux_loader(self):
        with tempfile.TemporaryDirectory(prefix="sevenzip-elf-native-") as temporary:
            for kind in NATIVE:
                with self.subTest(kind=kind):
                    image, _ = volume(kind)
                    path = Path(temporary) / kind
                    path.write_bytes(image)
                    path.chmod(0o700)
                    result = subprocess.run([str(path)], capture_output=True, timeout=5)
                    self.assertEqual(result.returncode, 42, result.stderr)
                    self.assertEqual(path.read_bytes(), image)

    @unittest.skipUnless(shutil.which("readelf"), "An independent readelf is unavailable")
    def test_extended_numbering(self):
        with tempfile.TemporaryDirectory(prefix="sevenzip-elf-extended-") as temporary:
            for kind in ("core-xnum", "sections-xnum"):
                image, metadata = volume(kind)
                path = Path(temporary) / (kind + ".elf")
                path.write_bytes(image)
                result = subprocess.run(["readelf", "-W", "-h", str(path)],
                                        capture_output=True, timeout=10)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn(b"(" + str(metadata["count"]).encode() + b")", result.stdout)


if __name__ == "__main__":
    unittest.main()
