"""Native encrypted ext4 reads through the dynamic engine and standalone refusal."""
from pathlib import Path
import json
import shutil
import subprocess
import struct
import sys
import tempfile
import unittest
import zlib
from ext_modern_fixture import crc32c


class FscryptCliTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-fscrypt-cli-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.root = Path(cls.temporary.name)
        core = "7z.dll" if sys.platform == "win32" else "7z.so"
        cls.suffix = ".exe" if sys.platform == "win32" else ""
        for name in ("7z" + cls.suffix, "7zz" + cls.suffix, core):
            shutil.copy2(RUNTIME / name, cls.root / name)
        (cls.root / "Formats").mkdir()
        shutil.copy2(MODULE, cls.root / "Formats" / MODULE.name)
        fixtures = Path(__file__).parent / "fixtures" / "fscrypt"
        cls.record = json.loads((fixtures / "native.json").read_text())
        cls.image_bytes = zlib.decompress((fixtures / "native.ext4.zlib").read_bytes())
        cls.image = cls.root / "образ encrypted 空.ext4"
        cls.image.write_bytes(cls.image_bytes)
        keys = []
        for case in cls.record["cases"]:
            prefix = "v1:" + case["descriptor"] + ":" if case["key_type"] == 1 else "v2:"
            keys.append(prefix + case["raw_key"])
        cls.keys = "fscrypt:" + ";".join(keys)

    def invoke(self, engine, command, key, name=None):
        args = [str(self.root / (engine + self.suffix)), command, "-tExt", "-sns-",
                "-sccUTF-8", "-bsp0", "-p" + key]
        args += ["-so", "-bso0", "-bse2"] if command == "x" else ["-slt"]
        args += ["--", str(self.image)]
        if name is not None:
            args.append(name)
        result = subprocess.run(args, stdin=subprocess.DEVNULL, capture_output=True, timeout=10)
        self.assertEqual(self.image.read_bytes(), self.image_bytes)
        return result

    def test_native_bytes_and_symlinks(self):
        for engine in ("7z",):
            listing = self.invoke(engine, "l", self.keys)
            self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
            self.assertIn(b"Encrypted = +", listing.stdout)
            for case in self.record["cases"]:
                with self.subTest(engine=engine, policy=case["name"]):
                    path = case["name"] + "/" + case["filename"]
                    result = self.invoke(engine, "x", self.keys, path)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    expected = bytes.fromhex(case["plain_first"]) + bytes(4096) + b"end"
                    self.assertEqual(result.stdout, expected)
                    result = self.invoke(engine, "x", self.keys, case["name"] + "/link")
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, case["symlink_target"].encode())

    def test_no_key_listing_and_regular_read_failure(self):
        for engine in ("7z",):
            listing = self.invoke(engine, "l", "fscrypt:none")
            self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
            for case in self.record["cases"]:
                self.assertIn(case["nokey_name"].encode(), listing.stdout)
            case = self.record["cases"][0]
            result = self.invoke(engine, "x", "fscrypt:none", case["name"] + "/" + case["nokey_name"])
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(result.stdout, b"")

    def test_standalone_without_gpl_module_cannot_publish_ciphertext(self):
        case = self.record["cases"][0]
        result = self.invoke("7zz", "x", self.keys, case["name"] + "/" + case["filename"])
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, b"")

    def test_foreign_policy_with_available_key_is_denied(self):
        # Preserve a valid inode checksum and give the file a different known
        # master key. Without the policy gate the engine could return garbage
        # with success, even though Linux forbids access through this parent.
        cases = {case["name"]: case for case in self.record["cases"]}
        for name, other in (("v1-essiv", "v1-aes"), ("v2-essiv", "v2-aes")):
            with self.subTest(policy=name):
                case, foreign = cases[name], cases[other]
                data = bytearray(self.image_bytes)
                sb = 1024
                block = 1024 << struct.unpack_from("<I", data, sb + 24)[0]
                per_group = struct.unpack_from("<I", data, sb + 40)[0]
                inode_size = struct.unpack_from("<H", data, sb + 88)[0]
                descriptor_size = struct.unpack_from("<H", data, sb + 254)[0]
                group, index = divmod(case["file_inode"] - 1, per_group)
                descriptor = block + group * descriptor_size
                table = struct.unpack_from("<I", data, descriptor + 8)[0]
                table |= struct.unpack_from("<I", data, descriptor + 40)[0] << 32
                offset = table * block + index * inode_size
                inode = bytearray(data[offset:offset + inode_size])
                context = bytes.fromhex(case["file_context"])
                where = inode.find(context)
                self.assertGreaterEqual(where, 160)
                key_offset = 4 if case["key_type"] == 1 else 8
                identity = bytes.fromhex(foreign["identity"])
                inode[where + key_offset:where + key_offset + len(identity)] = identity
                inode[124:126] = b"\0\0"
                inode[130:132] = b"\0\0"
                seed = crc32c(0xffffffff, bytes(data[sb + 104:sb + 120]))
                seed = crc32c(seed, struct.pack("<I", case["file_inode"]))
                seed = crc32c(seed, bytes(inode[100:104]))
                checksum = crc32c(seed, inode)
                struct.pack_into("<H", inode, 124, checksum & 0xffff)
                struct.pack_into("<H", inode, 130, checksum >> 16)
                data[offset:offset + inode_size] = inode
                original = self.image_bytes
                self.image_bytes = bytes(data)
                self.image.write_bytes(self.image_bytes)
                try:
                    listing = self.invoke("7z", "l", self.keys)
                    self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
                    result = self.invoke("7z", "x", self.keys, name + "/" + case["filename"])
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")
                finally:
                    self.image_bytes = original
                    self.image.write_bytes(original)


if __name__ == "__main__":
    RUNTIME, MODULE = map(Path, sys.argv[1:3])
    unittest.main(argv=[sys.argv[0]])
