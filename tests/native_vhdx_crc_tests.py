"""Read a small VHDX and reject damaged header/region CRC32C fields."""
from pathlib import Path
from runtime_fixture import copy_codec_runtime
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest

RUNTIME = Path(sys.argv.pop(1)).resolve()
SUFFIX = ".exe" if sys.platform == "win32" else ""
MIB = 1 << 20


def checksum(data):
    # Independent bitwise Castagnoli oracle for synthetic on-disk structures.
    state = 0xffffffff
    for value in data:
        state ^= value
        for _ in range(8):
            state = (state >> 1) ^ (0x82f63b78 if state & 1 else 0)
    return state ^ 0xffffffff


def image():
    result = bytearray(4 * MIB)
    result[:8] = b"vhdxfile"
    creator = "Synthetic CRC fixture".encode("utf-16le")
    result[8:8 + len(creator)] = creator
    for index in range(2):
        header = bytearray(4096)
        header[:4] = b"head"
        struct.pack_into("<Q", header, 8, index + 1)
        struct.pack_into("<H", header, 0x42, 1)
        struct.pack_into("<I", header, 4, checksum(header))
        start = (index + 1) << 16
        result[start:start + len(header)] = header

    region = bytearray(1 << 16)
    region[:4] = b"regi"
    struct.pack_into("<I", region, 8, 2)
    regions = (("6677c22d23f600429d64115e9bfd4a08", MIB),
               ("06a27c8b90479a4bb8fe575f050f886e", 2 * MIB))
    for i, (guid, offset) in enumerate(regions):
        pos = 16 + i * 32
        region[pos:pos + 16] = bytes.fromhex(guid)
        struct.pack_into("<QII", region, pos + 16, offset, MIB, 1)
    struct.pack_into("<I", region, 4, checksum(region))
    for start in (3 << 16, 4 << 16):
        result[start:start + len(region)] = region

    metadata = bytearray(MIB)
    metadata[:8] = b"metadata"
    entries = (("3767a1ca36fa434db3b633f0aa44e76b", struct.pack("<II", MIB, 1)),
               ("2442a52f1bcd7648b2115dbed83bf4b8", struct.pack("<Q", 4096)),
               ("1dbf41816fa90947ba47f233a8faab5f", struct.pack("<I", 512)))
    struct.pack_into("<H", metadata, 10, len(entries))
    for i, (guid, value) in enumerate(entries):
        pos, offset = 32 + i * 32, (1 << 16) + i * 32
        metadata[pos:pos + 16] = bytes.fromhex(guid)
        struct.pack_into("<III", metadata, pos + 16, offset, len(value), 4)
        metadata[offset:offset + len(value)] = value
    result[2 * MIB:3 * MIB] = metadata
    struct.pack_into("<Q", result, MIB, (3 * MIB) | 6)
    payload = bytes((i * 37 + 11) & 255 for i in range(4096))
    result[3 * MIB:3 * MIB + len(payload)] = payload
    return result, payload


class VhdxCrcTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-vhdx-crc-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        copy_codec_runtime(RUNTIME, cls.runtime)
        cls.valid, cls.payload = image()

    def extract(self, data):
        path = self.runtime / "диск 空.vhdx"
        path.write_bytes(data)
        return subprocess.run([str(self.runtime / ("7z" + SUFFIX)), "x", "-tVHDX:s0",
                               "-so", "-bso0", "-bse2", "--", str(path)],
                              capture_output=True, timeout=15)

    def test_valid_image(self):
        result = self.extract(self.valid)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout, self.payload)

    def test_corrupt_metadata_checksums(self):
        for offsets in ((1 << 16,), (2 << 16,), (3 << 16, 4 << 16)):
            with self.subTest(offsets=offsets):
                data = self.valid.copy()
                for start in offsets:
                    data[start + 100] ^= 1
                result = self.extract(data)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")


if __name__ == "__main__":
    unittest.main()
