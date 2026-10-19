"""Read sparse cramfs files through both production engine variants."""

from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

RUNTIME = Path(sys.argv.pop(1))


def image(blocks, endian="<"):
    """Linux cramfs layout: file blocks are delimited by end pointers."""
    name = b"data.bin"
    size = sum(len(block) for block in blocks)
    def inode(mode, length, offset, name_length=0):
        if endian == "<":
            fields = (mode, length, (offset // 4 << 6) | (name_length // 4))
        else:
            fields = (mode << 16, length << 8, (name_length // 4 << 26) | (offset // 4))
        return struct.pack(endian + "III", *fields)
    directory = inode(0o100644, size, 96, len(name)) + name
    # Root is at byte 64, directory at 76; block table starts at 96.
    result = bytearray(96 + 4 * len(blocks))
    result[:32] = struct.pack(endian + "IIII16s", 0x28CD3D45, 0, 0x101, 0,
                              b"Compressed ROMFS")
    result[64:76] = inode(0o40755, len(directory), 76)
    result[76:96] = directory
    for index, block in enumerate(blocks):
        if any(block):
            result.extend(zlib.compress(block))
        struct.pack_into(endian + "I", result, 96 + 4 * index, len(result))
    struct.pack_into(endian + "I", result, 4, len(result))
    struct.pack_into(endian + "IIII", result, 32, 0, 0, len(blocks), 2)
    struct.pack_into(endian + "I", result, 32, zlib.crc32(result))
    return bytes(result)


class CramfsTests(unittest.TestCase):
    def run_image(self, data, engine):
        with tempfile.TemporaryDirectory(prefix="sevenzip-cramfs-") as temporary:
            path = Path(temporary) / "образ 空.cramfs"
            path.write_bytes(data)
            completed = subprocess.run([str(engine), "x", "-so", "-bso0", "-bsp0", "-bse2",
                                        "-tCramFS", "--", str(path)], capture_output=True, timeout=10)
            self.assertEqual(path.read_bytes(), data)
            return completed

    def test_sparse_contents(self):
        suffix = ".exe" if sys.platform == "win32" else ""
        for name in ("7z", "7zz"):
            for endian in ("<", ">"):
                for blocks in ([bytes(4096), b"A" * 4096, bytes(73)],
                               [b"A" * 4096, bytes(4096), b"tail"],
                               [bytes(4096), bytes(73)]):
                    with self.subTest(engine=name, endian=endian, sizes=[len(b) for b in blocks]):
                        result = self.run_image(image(blocks, endian), RUNTIME / (name + suffix))
                        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                        self.assertEqual(result.stdout, b"".join(blocks))

if __name__ == "__main__":
    unittest.main()
