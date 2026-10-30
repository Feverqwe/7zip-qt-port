"""Read regular Compound streams through both shipped cluster-stream consumers."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

RUNTIME = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""
FREE, END, FAT = 0xffffffff, 0xfffffffe, 0xfffffffd


def volume():
    header = bytearray(512)
    header[:8] = bytes.fromhex("d0cf11e0a1b11ae1")
    struct.pack_into("<5H", header, 24, 0x3e, 3, 0xfffe, 9, 6)
    struct.pack_into("<9I", header, 40, 0, 1, 1, 0, 4096, END, 0, END, 0)
    struct.pack_into("<109I", header, 76, 0, *([FREE] * 108))
    sectors = [bytearray(512) for _ in range(18)]
    fat = [FREE] * 128
    fat[0], fat[1] = FAT, END
    files = {"First": bytes(range(256)) * 16,
             "Second": bytes((i * 7 + 23) & 255 for i in range(4096))}
    for start, payload in zip((2, 10), files.values()):
        for i in range(8):
            fat[start + i] = start + i + 1 if i < 7 else END
            sectors[start + i][:] = payload[i * 512:(i + 1) * 512]
    sectors[0][:] = struct.pack("<128I", *fat)
    for index, name, kind, right, child, start, size in (
            (0, "Root Entry", 5, FREE, 1, END, 0),
            (1, "First", 2, 2, FREE, 2, 4096),
            (2, "Second", 2, FREE, FREE, 10, 4096)):
        record = bytearray(128)
        encoded = (name + "\0").encode("utf-16le")
        record[:len(encoded)] = encoded
        struct.pack_into("<HBBIII", record, 64, len(encoded), kind, int(index != 2),
                         FREE, right, child)
        struct.pack_into("<IQ", record, 116, start, size)
        sectors[1][index * 128:(index + 1) * 128] = record
    return bytes(header) + b"".join(sectors), files


class CompoundStreamTests(unittest.TestCase):
    def test_regular_streams_and_truncated_source(self):
        image, files = volume()
        with tempfile.TemporaryDirectory(prefix="sevenzip-compound-") as temporary:
            path = Path(temporary) / "потоки 空.doc"
            for engine in ("7z",):
                for truncated in (False, True):
                    source = image[:-211] if truncated else image
                    path.write_bytes(source)
                    for name, expected in files.items():
                        with self.subTest(engine=engine, truncated=truncated, name=name):
                            result = subprocess.run(
                                [str(RUNTIME / (engine + SUFFIX)), "x", "-so", "-tCompound",
                                 "-bsp0", str(path), name], capture_output=True, timeout=15)
                            if truncated:
                                self.assertNotEqual(result.returncode, 0)
                                if name == "Second":
                                    self.assertEqual(result.stdout, expected[:-211])
                                    self.assertIn(b"Data Error", result.stderr)
                                else:
                                    self.assertEqual(result.stdout, expected)
                            else:
                                self.assertEqual(result.returncode, 0, result.stderr)
                                self.assertEqual(result.stdout, expected)
                            self.assertEqual(path.read_bytes(), source)


if __name__ == "__main__":
    unittest.main()
