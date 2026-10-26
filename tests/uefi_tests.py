"""EFI capsules, firmware volumes, exact extraction and malformed spans."""
from pathlib import Path
from runtime_fixture import copy_codec_runtime
import shutil
import subprocess
import sys
import tempfile
import unittest
from uefi_fixture import CASES, BAD, volume

RUNTIME = Path(sys.argv.pop(1)).resolve()
SUFFIX = ".exe" if sys.platform == "win32" else ""


class UefiTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-uefi-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        copy_codec_runtime(RUNTIME, cls.runtime)

    def command(self, engine, path, command, *args, fmt="UEFIc", forced=True):
        return subprocess.run(
            [str(self.runtime / (engine + SUFFIX)), command, "-sccUTF-8", "-bsp0",
             "-t" + fmt + ":s0" if forced else "-t*:s0", *args, "--", str(path)],
            capture_output=True, timeout=10)

    def test_read_and_discover(self):
        for kind in CASES:
            image, metadata = volume(kind)
            fmt = metadata["format"]
            with tempfile.TemporaryDirectory(prefix="sevenzip-uefi-") as temporary:
                known = kind in ("standard", "legacy1", "legacy2", "quark", "quark-fv",
                                 "fv-capsule", "lzma-capsule", "me-capsule", "fv")
                path = Path(temporary) / ("образ 空.bin" if known else
                                          "образ 空.uefif" if fmt == "UEFIf" else "образ 空.cap")
                path.write_bytes(image)
                for engine in ("7z",):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "l", "-slt", fmt=fmt, forced=False)
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertIn(b"Type = " + fmt.encode(), result.stdout)
                        selection = [] if metadata["target"] is None else ["-i!" + metadata["target"]]
                        result = self.command(engine, path, "x", "-so", "-bso0", "-bse2", *selection, fmt=fmt)
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(result.stdout, bytes.fromhex(metadata["payload"]))
                        result = self.command(engine, path, "t", fmt=fmt)
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)

    def test_malformed_input_fails_normally(self):
        for kind in BAD:
            image, _ = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-uefi-invalid-") as temporary:
                path = Path(temporary) / "bad.cap"
                path.write_bytes(image)
                for engine in ("7z",):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.command(engine, path, "t")
                        self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                        self.assertEqual(path.read_bytes(), image)


if __name__ == "__main__":
    unittest.main()
