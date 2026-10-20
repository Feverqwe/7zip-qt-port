"""Read official-creator EROFS images through the production dynamic engine."""
from pathlib import Path
import shutil
import json
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib
import urllib.parse
from erofs_fixture import FILES, PAYLOAD_NAME, mutation_locations, xattr_locations

RUNTIME = Path(sys.argv.pop(1))
MODULE = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""
FIXTURES = Path(__file__).parent / "fixtures" / "erofs"


class ErofsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-erofs-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        core = "7z.dll" if sys.platform == "win32" else "7z.so"
        for name in ("7z" + SUFFIX, core):
            shutil.copy2(RUNTIME / name, cls.runtime / name)
        (cls.runtime / "Formats").mkdir()
        shutil.copy2(MODULE, cls.runtime / "Formats" / MODULE.name)

    def run_engine(self, arguments):
        try:
            return subprocess.run(arguments, stdin=subprocess.DEVNULL, capture_output=True, timeout=20)
        except subprocess.TimeoutExpired as error:
            self.fail("Engine timed out: " + repr(arguments) + "\n" +
                      (error.stdout or b"").decode(errors="replace") +
                      (error.stderr or b"").decode(errors="replace"))

    def test_malformed_metadata_and_data_are_rejected(self):
        original = zlib.decompress((FIXTURES / "plain.erofs.zlib").read_bytes())
        locations = mutation_locations(original)
        cases = {}
        for case in ("feature", "block-size", "root-type", "negative-size", "traversal", "cycle", "extent", "timestamp-overflow"):
            data = bytearray(original)
            struct.pack_into("<I", data, 1032, 0)  # Metadata mutations bypass the optional checksum.
            node = locations["file"]
            if case == "feature":
                struct.pack_into("<I", data, 1104, 1 << 31)
            elif case == "block-size":
                data[1036] = 8
            elif case == "root-type":
                struct.pack_into("<H", data, locations["root"] + 4, 0o100644)
            elif case == "negative-size":
                fmt = struct.unpack_from("<H", data, node)[0]
                struct.pack_into("<H", data, node, fmt | 1)
                struct.pack_into("<Q", data, node + 8, 1 << 63)
            elif case == "traversal":
                data[locations["name"]:locations["name"] + 3] = b"../"
            elif case == "cycle":
                struct.pack_into("<Q", data, locations["entry"], locations["root_nid"])
            elif case == "extent":
                struct.pack_into("<I", data, node + 16, len(data) // locations["block_size"] + 1)
            elif case == "timestamp-overflow":
                if struct.unpack_from("<H", data, locations["root"])[0] & 1:
                    raise ValueError("Timestamp overflow fixture needs a compact root inode")
                struct.pack_into("<Q", data, 1048, (1 << 63) - 1)
                struct.pack_into("<I", data, locations["root"] + 12, 1)
            cases[case] = data
        checksum = bytearray(original)
        checksum[1028] ^= 1
        cases["checksum"] = checksum
        cases["truncation"] = original[:-17]
        for algorithm, invalid in (("lz4", 0xff), ("lzma", 1), ("deflate", 7), ("zstd", 0xff)):
            compressed = bytearray(zlib.decompress((FIXTURES / (algorithm + "-full.erofs.zlib")).read_bytes()))
            node = mutation_locations(compressed)["file"]
            fmt, xcount = struct.unpack_from("<HH", compressed, node)
            if (fmt >> 1) & 7 != 1:
                raise ValueError("Corruption fixture needs full compression indexes: " + algorithm)
            header_end = node + (64 if fmt & 1 else 32) + (12 + (xcount - 1) * 4 if xcount else 0)
            index = (header_end + 7) // 8 * 8 + 16
            physical = struct.unpack_from("<I", compressed, index + 4)[0] * 4096
            compressed[physical:physical + 4096] = bytes([0xff]) * 4096
            compressed[physical] = invalid
            cases[algorithm + "-compressed-block"] = compressed
        for case, data in cases.items():
            with self.subTest(corruption=case):
                path = self.runtime / "повреждённый 空.img"
                path.write_bytes(data)
                result = self.run_engine([str(self.runtime / ("7z" + SUFFIX)), "x", "-tEROFS", "-so",
                                          "-bso0", "-bsp0", "-bse2", "--", str(path), PAYLOAD_NAME])
                self.assertIn(result.returncode, (1, 2), result.stderr.decode(errors="replace"))
                self.assertEqual(path.read_bytes(), data)
                path.unlink()

    def test_xattr_values_shared_inline_prefix_and_metabox(self):
        for variant, record in json.loads((FIXTURES / "manifest.json").read_text())["images"].items():
            if "attributes" not in record:
                continue
            with self.subTest(variant=variant):
                data = zlib.decompress((FIXTURES / (variant + ".erofs.zlib")).read_bytes())
                image = self.runtime / "значения 空.erofs"
                image.write_bytes(data)
                engine = str(self.runtime / ("7z" + SUFFIX))
                listing = self.run_engine([engine, "l", "-slt", "-sns", "-sccUTF-8", "--", str(image)])
                self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
                paths = {line[7:] for line in listing.stdout.decode("utf-8").splitlines()
                         if line.startswith("Path = ")}
                plain = self.run_engine([engine, "l", "-slt", "-sns-", "--", str(image)])
                self.assertEqual(plain.returncode, 0, plain.stderr.decode(errors="replace"))
                self.assertNotIn(b"Alternate Stream = +", plain.stdout)
                for path, attributes in record["attributes"].items():
                    for name, value in attributes.items():
                        base = (path or "[ROOT]_") + ":$XATTR." + urllib.parse.quote(name, safe="._-")
                        candidates = [p for p in paths if p == base or p.startswith(base + "#")]
                        self.assertEqual(len(candidates), 1, base)
                        extracted = self.run_engine([engine, "x", "-so", "-sns", "-bso0", "-bsp0", "-bse2",
                                                     "--", str(image), candidates[0]])
                        self.assertEqual(extracted.returncode, 0, extracted.stderr.decode(errors="replace"))
                        self.assertEqual(extracted.stdout, bytes.fromhex(value), candidates[0])
                self.assertEqual(image.read_bytes(), data)
                image.unlink()

    def test_xattr_corruption_and_reserved_filter(self):
        for fault in ("header-only", "entry-size", "shared-count", "shared-id", "acl-version",
                      "acl-id", "filter", "reserved-filter"):
            with self.subTest(fault=fault):
                variant = "shared" if fault.startswith("shared") else "inline"
                data = bytearray(zlib.decompress((FIXTURES / ("xattr-values-" + variant + ".erofs.zlib")).read_bytes()))
                loc = xattr_locations(data)
                struct.pack_into("<I", data, 1032, 4)  # Keep name filtering, omit optional checksum.
                if fault == "header-only":
                    struct.pack_into("<H", data, loc["node"] + 2, 1)
                elif fault == "entry-size":
                    struct.pack_into("<H", data, next(iter(loc["entries"].values())) + 2, 65535)
                elif fault == "shared-count":
                    data[loc["header"] + 4] = 255
                elif fault == "shared-id":
                    struct.pack_into("<I", data, loc["header"] + 12, 0xffffffff)
                elif fault.startswith("acl"):
                    value = loc["entries"][(2, b"")] + 4
                    if fault == "acl-version":
                        struct.pack_into("<I", data, value, 0)
                    else:
                        struct.pack_into("<I", data, value + 4 + 8 + 4, 0xffffffff)
                else:
                    struct.pack_into("<I", data, loc["header"], 0xffffffff)
                    if fault == "reserved-filter":
                        data[1128] = 1
                image = self.runtime / "xattr corrupt.img"
                image.write_bytes(data)
                result = self.run_engine([str(self.runtime / ("7z" + SUFFIX)), "x", "-so",
                                          "-sns", "-bso0", "-bsp0", "-bse2", "--", str(image),
                                          PAYLOAD_NAME + ":$XATTR.user.large"])
                if fault == "reserved-filter":
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, bytes(i % 251 for i in range(9000)))
                else:
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")
                self.assertEqual(image.read_bytes(), data)
                image.unlink()

    def test_creator_images_exact_contents_and_metadata(self):
        variants = json.loads((FIXTURES / "manifest.json").read_text())["images"]
        for variant in variants:
            fixture = FIXTURES / (variant + ".erofs.zlib")
            with self.subTest(layout=variant):
                data = zlib.decompress(fixture.read_bytes())
                image = self.runtime / "образ 空.img"
                image.write_bytes(data)
                devices = variants[variant].get("devices", {})
                for name, record in devices.items():
                    (self.runtime / name).write_bytes(zlib.decompress((FIXTURES / record["file"]).read_bytes()))
                engine = str(self.runtime / ("7z" + SUFFIX))
                listing = self.run_engine([engine, "l", "-slt", "-sccUTF-8", "--", str(image)])
                self.assertEqual(listing.returncode, 0, listing.stdout.decode(errors="replace") +
                                 listing.stderr.decode(errors="replace"))
                text = listing.stdout.decode("utf-8")
                self.assertIn("Type = EROFS", text)
                self.assertIn("Path = " + PAYLOAD_NAME, text)
                self.assertIn("User ID = 1234", text)
                self.assertIn("Group ID = 5678", text)
                self.assertIn("Symbolic Link = " + PAYLOAD_NAME, text)
                for name, expected in FILES.items():
                    result = self.run_engine([engine, "x", "-so", "-sns-", "-bso0", "-bsp0", "-bse2", "--",
                                              str(image), name])
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected, name)
                self.assertEqual(image.read_bytes(), data)
                image.unlink()
                for name in devices:
                    (self.runtime / name).unlink()


if __name__ == "__main__":
    unittest.main()
