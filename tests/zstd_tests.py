"""Zstandard frames, skippable metadata, blocks and integrity failures."""
from pathlib import Path
from runtime_fixture import copy_codec_runtime
import shutil
import subprocess
import sys
import tempfile
import unittest
from zstd_fixture import CASES, BAD, volume

RUNTIME = Path(sys.argv.pop(1)).resolve()
SUFFIX = ".exe" if sys.platform == "win32" else ""


class ZstdTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-zstd-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        copy_codec_runtime(RUNTIME, cls.runtime)

    def command(self, engine, path, command, *args, forced=True):
        return subprocess.run(
            [str(self.runtime / (engine + SUFFIX)), command, "-sccUTF-8", "-bsp0",
             "-tzstd:s0" if forced else "-t*:s0", *args, "--", str(path)],
            capture_output=True, timeout=10)

    def test_read_and_discover(self):
        for kind in CASES:
            image, expected = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-zstd-") as temporary:
                path = Path(temporary) / "архив 空.bin"
                path.write_bytes(image)
                for engine in ("7z",):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "l", "-slt", forced=False)
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertIn(b"Type = zstd", result.stdout)
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
            with tempfile.TemporaryDirectory(prefix="sevenzip-zstd-invalid-") as temporary:
                path = Path(temporary) / "bad.bin"
                path.write_bytes(image)
                for engine in ("7z",):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "t")
                        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)

    def test_error_preserves_available_data(self):
        checksum_payload = volume("checksum")[1]
        raw_payload = volume("raw")[1]
        cases = {
            "checksum-bad": checksum_payload,
            "late-crc": raw_payload + checksum_payload,
            "body-short": raw_payload[:-1],
            "size-large": raw_payload,
        }
        with tempfile.TemporaryDirectory(prefix="sevenzip-zstd-recovery-") as temporary:
            path = Path(temporary) / "повреждённый 空.zst"
            for kind, expected in cases.items():
                with self.subTest(kind=kind):
                    image, _ = volume(kind)
                    path.write_bytes(image)
                    result = self.command("7z", path, "x", "-so", "-bso0", "-bse2")
                    self.assertEqual(result.returncode, 2, result.stderr)
                    self.assertEqual(result.stdout, expected)
                    self.assertEqual(path.read_bytes(), image)


if __name__ == "__main__":
    unittest.main()
