"""GPT discovery, validated table selection and bounded partition extraction."""
from pathlib import Path
import shutil
import subprocess
import struct
import sys
import tempfile
import unittest
from gpt_fixture import CASES, BAD, volume

RUNTIME = Path(sys.argv.pop(1)).resolve()
SUFFIX = ".exe" if sys.platform == "win32" else ""


class GptTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-gpt-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        for name in ("7z" + SUFFIX, "7zz" + SUFFIX,
                     "7z.dll" if sys.platform == "win32" else "7z.so"):
            shutil.copy2(RUNTIME / name, cls.runtime / name)

    def run_image(self, engine, image, *, listing=False, forced=True, format_name="GPT"):
        with tempfile.TemporaryDirectory(prefix="sevenzip-gpt-") as temporary:
            path = Path(temporary) / "разделы 空.bin"
            path.write_bytes(image)
            args = [str(self.runtime / (engine + SUFFIX)), "l" if listing else "x", "-sccUTF-8", "-bsp0"]
            if forced:
                args += ["-t" + format_name]
            args += ["-slt"] if listing else ["-so", "-bso0", "-bse2"]
            result = subprocess.run(args + ["--", str(path)], capture_output=True, timeout=10)
            self.assertEqual(path.read_bytes(), image)
            return result

    def test_partition_payload_and_discovery(self):
        for kind in CASES:
            image, expected, sector = volume(kind)
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    result = self.run_image(engine, image)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, expected)
                    result = self.run_image(engine, image, listing=True, forced=False)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    self.assertIn(b"Type = GPT", result.stdout)
                    self.assertIn(("Sector Size = " + str(sector)).encode(), result.stdout)
                    self.assertIn("данные 空".encode(), result.stdout)

    def test_invalid_tables_fail_normally(self):
        for kind in BAD:
            image, _, _ = volume(kind)
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    self.assertEqual(self.run_image(engine, image).returncode, 2)

    def test_virtual_disk_inner_gpt_name(self):
        for kind in ("sector-1024", "sector-2048", "backup-signature"):
            image, _, _ = volume(kind)
            footer = bytearray(512)
            footer[:8] = b"conectix"
            struct.pack_into(">IIQ", footer, 8, 2, 0x10000, 0xffffffffffffffff)
            struct.pack_into(">QQ", footer, 40, len(image), len(image))
            struct.pack_into(">I", footer, 60, 2)
            struct.pack_into(">I", footer, 64, (~sum(footer)) & 0xffffffff)
            for engine in ("7z", "7zz"):
                with self.subTest(kind=kind, engine=engine):
                    wrapped = image + footer
                    result = self.run_image(engine, wrapped, listing=True, format_name="VHD")
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    self.assertIn(b"Type = VHD", result.stdout)
                    result = self.run_image(engine, wrapped, format_name="VHD")
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(result.stdout, image)
                    with tempfile.TemporaryDirectory(prefix="sevenzip-gpt-vhd-") as temporary:
                        source = Path(temporary) / "disk.vhd"
                        source.write_bytes(wrapped)
                        output = Path(temporary) / "output"
                        result = subprocess.run(
                            [str(self.runtime / (engine + SUFFIX)), "x", "-tVHD", "-y",
                             "-o" + str(output), "--", str(source)],
                            capture_output=True, timeout=10)
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        files = list(output.iterdir())
                        self.assertEqual(len(files), 1)
                        self.assertEqual(files[0].suffix, ".gpt")
                        self.assertEqual(files[0].read_bytes(), image)
                        self.assertEqual(source.read_bytes(), wrapped)


if __name__ == "__main__":
    unittest.main()
