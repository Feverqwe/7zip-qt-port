"""XZ framing, filters, blocks, padding and integrity failures."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[3]))
from pathlib import Path
from tests.archive.support.runtime_fixture import copy_codec_runtime
import lzma
import shutil
import subprocess
import sys
import tempfile
import unittest
from tests.formats.xz.support.xz_fixture import CASES, BAD, volume

RUNTIME = Path(sys.argv.pop(1)).resolve()
SUFFIX = ".exe" if sys.platform == "win32" else ""


class XzTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-xz-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        copy_codec_runtime(RUNTIME, cls.runtime)

    def command(self, engine, path, command, *args, forced=True):
        return subprocess.run(
            [str(self.runtime / (engine + SUFFIX)), command, "-sccUTF-8", "-bsp0",
             "-txz:s0" if forced else "-t*:s0", *args, "--", str(path)],
            capture_output=True, timeout=10)

    def test_read_and_discover(self):
        for kind in CASES:
            image, expected = volume(kind, self.runtime / ("7z" + SUFFIX))
            with tempfile.TemporaryDirectory(prefix="sevenzip-xz-") as temporary:
                path = Path(temporary) / "архив 空.bin"
                path.write_bytes(image)
                for engine in ("7z",):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "l", "-slt", forced=False)
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertIn(b"Type = xz", result.stdout)
                        for threads in (1, 2):
                            result = self.command(engine, path, "x", "-so", "-bso0", "-bse2",
                                                  "-mmt=" + str(threads))
                            self.assertEqual(result.returncode, 0, result.stderr)
                            self.assertEqual(result.stdout, expected)
                        result = self.command(engine, path, "t")
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)

    def test_malformed_input_fails_normally(self):
        for kind in BAD:
            image, _ = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-xz-invalid-") as temporary:
                path = Path(temporary) / "bad.bin"
                path.write_bytes(image)
                for engine in ("7z",):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "t")
                        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)

    def test_crc64_write_interoperability(self):
        payload = bytes(range(256)) * 257
        with tempfile.TemporaryDirectory(prefix="sevenzip-xz-write-") as temporary:
            root = Path(temporary)
            source = root / "данные 空.bin"
            archive = root / "архив 空.xz"
            source.write_bytes(payload)
            result = subprocess.run(
                [str(self.runtime / ("7z" + SUFFIX)), "a", "-txz", "-mcrc=8", "-mmt=1",
                 "--", str(archive), str(source)], capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            decoder = lzma.LZMADecompressor()
            self.assertEqual(decoder.decompress(archive.read_bytes()), payload)
            self.assertTrue(decoder.eof)
            self.assertEqual(decoder.check, lzma.CHECK_CRC64)
            self.assertEqual(source.read_bytes(), payload)


if __name__ == "__main__":
    unittest.main()
