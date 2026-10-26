"""Intel HEX address records, settings maps and malformed input."""
from pathlib import Path
from runtime_fixture import copy_codec_runtime
import shutil
import subprocess
import sys
import tempfile
import unittest
from ihex_fixture import CASES, BAD, volume

RUNTIME = Path(sys.argv.pop(1)).resolve()
SUFFIX = ".exe" if sys.platform == "win32" else ""


class IhexTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-ihex-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        copy_codec_runtime(RUNTIME, cls.runtime)

    def command(self, engine, path, command, *args, fmt="IHex", forced=True):
        return subprocess.run(
            [str(self.runtime / (engine + SUFFIX)), command, "-sccUTF-8", "-bsp0",
             "-t" + fmt + ":s0" if forced else "-t*:s0", *args, "--", str(path)],
            capture_output=True, timeout=10)

    def test_read_and_discover(self):
        for kind in CASES:
            image, metadata = volume(kind)
            fmt = "IHex"
            options = ["-miqs5xx=on"] if metadata["profile"] else []
            with tempfile.TemporaryDirectory(prefix="sevenzip-ihex-") as temporary:
                path = Path(temporary) / "образ 空.bin"
                path.write_bytes(image)
                for engine in ("7z",):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "l", "-slt", fmt=fmt, *options, forced=metadata["profile"])
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertIn(b"Type = " + fmt.encode(), result.stdout)
                        selection = [] if metadata["target"] is None else ["-i!" + metadata["target"]]
                        result = self.command(engine, path, "x", "-so", "-bso0", "-bse2", *selection, *options, fmt=fmt)
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(result.stdout, bytes.fromhex(metadata["payload"]))
                        result = self.command(engine, path, "t", *options, fmt=fmt)
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)

    def test_malformed_input_fails_normally(self):
        for kind in BAD:
            image, metadata = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-ihex-invalid-") as temporary:
                path = Path(temporary) / "bad.ihex"
                path.write_bytes(image)
                for engine in ("7z",):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "t", *(["-miqs5xx=on"] if metadata["profile"] else []))
                        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)

    def test_vendor_checksum_policy_requires_explicit_profile(self):
        image, _ = volume("iqs-settings")
        with tempfile.TemporaryDirectory(prefix="sevenzip-ihex-profile-") as temporary:
            path = Path(temporary) / "settings.ihex"
            path.write_bytes(image)
            for engine in ("7z",):
                result = self.command(engine, path, "t")
                self.assertEqual(result.returncode, 2, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
