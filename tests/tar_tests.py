"""TAR interoperability, physical record bounds and update consumers."""
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import tempfile
import unittest
from tar_fixture import CASES, BAD, volume

RUNTIME = Path(sys.argv.pop(1)).resolve()
SUFFIX = ".exe" if sys.platform == "win32" else ""


class TarTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-tar-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        for name in ("7z" + SUFFIX, "7zz" + SUFFIX,
                     "7z.dll" if sys.platform == "win32" else "7z.so"):
            shutil.copy2(RUNTIME / name, cls.runtime / name)

    def command(self, engine, path, command, *args, forced=True, files=()):
        return subprocess.run(
            [str(self.runtime / (engine + SUFFIX)), command, "-sccUTF-8", "-bsp0",
             "-tTar:s0" if forced else "-t*:s0", *args, "--", str(path), *files],
            capture_output=True, timeout=10)

    def test_read_and_discover(self):
        for kind in CASES:
            image, meta = volume(kind)
            options = ["-mlinkdata=on"] if meta["linkdata"] else []
            with tempfile.TemporaryDirectory(prefix="sevenzip-tar-") as temporary:
                path = Path(temporary) / "образ 空.bin"
                path.write_bytes(image)
                for engine in ("7z", "7zz"):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "l", "-slt", *options,
                                              forced=meta["linkdata"])
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertIn(b"Type = tar", result.stdout)
                        result = self.command(engine, path, "x", "-so", "-bso0", "-bse2",
                                              "-i!" + meta["target"], *options)
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(result.stdout, bytes.fromhex(meta["payload"]))
                        result = self.command(engine, path, "t", *options)
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)

    def test_malformed_input(self):
        for kind in BAD:
            image, _ = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-tar-invalid-") as temporary:
                path = Path(temporary) / "bad.tar"
                path.write_bytes(image)
                for engine in ("7z", "7zz"):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "t")
                        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)

    def test_update_preserves_existing_extensions_and_data(self):
        for kind in ("gnu", "pax", "sparse-extended", "linkdata-directory"):
            original, meta = volume(kind)
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine), tempfile.TemporaryDirectory(
                        prefix="sevenzip-tar-update-") as temporary:
                    path = Path(temporary) / "update.tar"
                    path.write_bytes(original)
                    source = Path(temporary) / "new 空.txt"
                    source.write_bytes(b"new content")
                    options = ["-mlinkdata=on"] if meta["linkdata"] else []
                    result = self.command(engine, path, "a", *options, files=(str(source),))
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    result = self.command(engine, path, "x", "-so", "-bso0", "-bse2",
                                          "-i!" + meta["target"], *options)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, bytes.fromhex(meta["payload"]))
                    result = self.command(engine, path, "x", "-so", "-bso0", "-bse2",
                                          "-i!" + source.name, *options)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, source.read_bytes())
                    if not meta["linkdata"]:
                        with tarfile.open(path) as archive:
                            self.assertEqual(archive.extractfile(meta["target"]).read(),
                                             bytes.fromhex(meta["payload"]))


if __name__ == "__main__":
    unittest.main()
