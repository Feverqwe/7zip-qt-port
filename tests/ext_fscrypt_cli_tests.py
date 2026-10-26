"""Native encrypted ext4 reads through the dynamic engine."""
from pathlib import Path
from runtime_fixture import copy_codec_runtime
import base64
import json
import shutil
import subprocess
import struct
import sys
import tempfile
import unittest
import zlib
from ext_modern_fixture import crc32c
from ext_readlink_fixture import encrypted_link_size, encrypted_link_payload_length, encrypted_link_indirect
from ext_partial_fixture import encrypted_partial_map, encrypted_indirect_map


class FscryptCliTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-fscrypt-cli-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.root = Path(cls.temporary.name)
        core = "7z.dll" if sys.platform == "win32" else "7z.so"
        cls.suffix = ".exe" if sys.platform == "win32" else ""
        copy_codec_runtime(RUNTIME, cls.root)
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

    def test_indirect_file_transport_preserves_crypto_units_holes_and_tail(self):
        original = self.image_bytes
        try:
            cases = {case["name"]: case for case in self.record["cases"]}
            for name in ("v1-aes", "v2-subblock"):
                case = cases[name]
                with self.subTest(policy=case["name"]):
                    self.image_bytes = encrypted_indirect_map(original, case)
                    self.image.write_bytes(self.image_bytes)
                    result = self.invoke("7z", "x", self.keys, case["name"] + "/" + case["filename"])
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout,
                                     bytes.fromhex(case["plain_first"]) + bytes(4096) + b"end")
        finally:
            self.image_bytes = original
            self.image.write_bytes(original)

    def test_partial_encrypted_maps_keep_only_verified_prefixes(self):
        original = self.image_bytes
        try:
            cases = self.record["cases"]
            for i, case in enumerate(cases):
                with self.subTest(policy=case["name"]):
                    self.image_bytes = encrypted_partial_map(original, case)
                    self.image.write_bytes(self.image_bytes)
                    result = self.invoke("7z", "x", self.keys, case["name"] + "/" + case["filename"])
                    self.assertNotEqual(result.returncode, 0)
                    # A complete encryption unit can be decrypted from the
                    # first leaf. fs-verity additionally needs its missing map
                    # and must never publish these bytes without verification.
                    expected = b"" if case["verity"] else bytes.fromhex(case["plain_first"])
                    self.assertEqual(result.stdout, expected)
                    healthy = cases[(i + 1) % len(cases)]
                    result = self.invoke("7z", "x", self.keys,
                                         healthy["name"] + "/" + healthy["filename"])
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, bytes.fromhex(healthy["plain_first"]) + bytes(4096) + b"end")
        finally:
            self.image_bytes = original
            self.image.write_bytes(original)

    def test_unsigned_default_hash_versions_read_encrypted_names(self):
        original = self.image_bytes
        try:
            for version in (3, 4, 5):
                with self.subTest(version=version):
                    data = bytearray(original)
                    data[1024 + 252] = version
                    struct.pack_into("<I", data, 2044, crc32c(0xFFFFFFFF, data[1024:2044]))
                    self.image_bytes = bytes(data)
                    self.image.write_bytes(self.image_bytes)
                    listing = self.invoke("7z", "l", self.keys)
                    self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
                    for case in self.record["cases"]:
                        self.assertIn(case["filename"].encode(), listing.stdout)
                    case = self.record["cases"][0]
                    result = self.invoke("7z", "x", self.keys, case["name"] + "/" + case["filename"])
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, bytes.fromhex(case["plain_first"]) + bytes(4096) + b"end")
        finally:
            self.image_bytes = original
            self.image.write_bytes(original)

    def test_htree_fallback_uses_linear_name_hashes(self):
        original = self.image_bytes
        case = self.record["cases"][0]
        data = bytearray(original)
        block_size = 1024 << struct.unpack_from("<I", data, 1024 + 24)[0]
        inode_size = struct.unpack_from("<H", data, 1024 + 88)[0]
        table = struct.unpack_from("<I", data, block_size + 8)[0]
        offset = table * block_size + (case["directory_inode"] - 1) * inode_size
        # This native fixture has one group. Make its legacy bitmap/descriptor
        # mode explicit, then request htree on the ordinary encrypted leaf.
        ro = struct.unpack_from("<I", data, 1024 + 100)[0]
        struct.pack_into("<I", data, 1024 + 100, ro & ~(0x400 | 0x10))
        flags = struct.unpack_from("<I", data, offset + 32)[0]
        struct.pack_into("<I", data, offset + 32, flags | 0x1000)
        # Without metadata_csum, casefolded encrypted records cannot leave a
        # 12-byte checksum tail: their ordinary minimum record is larger.
        for folded in (entry for entry in self.record["cases"] if entry["casefolded"]):
            inode = table * block_size + (folded["directory_inode"] - 1) * inode_size
            if struct.unpack_from("<I", data, inode + 32)[0] & 0x80000:
                self.assertEqual(struct.unpack_from("<H", data, inode + 46)[0], 0)
                physical = struct.unpack_from("<I", data, inode + 60)[0]
            else:
                physical = struct.unpack_from("<I", data, inode + 40)[0]
            start = physical * block_size
            position = start
            while position < start + block_size - 12:
                length = struct.unpack_from("<H", data, position + 4)[0]
                self.assertGreaterEqual(length, 12)
                if position + length == start + block_size - 12:
                    struct.pack_into("<H", data, position + 4, length + 12)
                    break
                position += length
        encoded = case["nokey_name"]
        raw_name = base64.urlsafe_b64decode(encoded + "=" * (-len(encoded) % 4))
        linear_name = base64.urlsafe_b64encode(bytes(8) + raw_name[8:]).rstrip(b"=")
        try:
            self.image_bytes = bytes(data)
            self.image.write_bytes(self.image_bytes)
            listing = self.invoke("7z", "l", self.keys)
            self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
            self.assertIn(case["filename"].encode(), listing.stdout)
            listing = self.invoke("7z", "l", "fscrypt:none")
            self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
            self.assertIn(linear_name, listing.stdout)
            result = self.invoke("7z", "x", self.keys, case["name"] + "/" + case["filename"])
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
            self.assertEqual(result.stdout, bytes.fromhex(case["plain_first"]) + bytes(4096) + b"end")
        finally:
            self.image_bytes = original
            self.image.write_bytes(original)

    def test_symlink_ciphertext_length_uses_storage_bounds_not_inode_size(self):
        original = self.image_bytes
        case = self.record["cases"][0]
        try:
            self.image_bytes = encrypted_link_size(original, case, 1)
            self.image.write_bytes(self.image_bytes)
            for key, target in ((self.keys, case["symlink_target"]),
                                ("fscrypt:none", case["nokey_symlink_target"])):
                path = case["name"] + "/link"
                if key == "fscrypt:none":
                    listing = self.invoke("7z", "l", key)
                    self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
                    chunks = listing.stdout.decode().replace("\r\n", "\n").split("\n\n")
                    chunk = next(part for part in chunks if "Symbolic Link = " + target in part)
                    path = next(line[7:] for line in chunk.splitlines() if line.startswith("Path = "))
                result = self.invoke("7z", "x", key, path)
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(result.stdout, target.encode())
        finally:
            self.image_bytes = original
            self.image.write_bytes(original)

    def test_bad_encrypted_link_is_reported_without_hiding_independent_data(self):
        original = self.image_bytes
        case = self.record["cases"][0]
        bs = 1024 << struct.unpack_from("<I", original, 1024 + 24)[0]
        try:
            for length in (0, bs):
                with self.subTest(length=length):
                    self.image_bytes = encrypted_link_payload_length(original, case, length)
                    self.image.write_bytes(self.image_bytes)
                    for key in (self.keys, "fscrypt:none"):
                        listing = self.invoke("7z", "l", key)
                        self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
                        self.assertIn(b"Read-only limitations = ", listing.stdout)
                    result = self.invoke("7z", "x", self.keys, case["name"] + "/link")
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")
                    result = self.invoke("7z", "x", self.keys,
                                         case["name"] + "/" + case["filename"])
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout,
                                     bytes.fromhex(case["plain_first"]) + bytes(4096) + b"end")
        finally:
            self.image_bytes = original
            self.image.write_bytes(original)

    def test_classic_symlink_transport_preserves_native_ciphertext_decoding(self):
        original = self.image_bytes
        case = self.record["cases"][0]
        try:
            self.image_bytes = encrypted_link_size(encrypted_link_indirect(original, case), case, 1)
            self.image.write_bytes(self.image_bytes)
            for key, expected in ((self.keys, case["symlink_target"]),
                                  ("fscrypt:none", case["nokey_symlink_target"])):
                with self.subTest(key_available=key != "fscrypt:none"):
                    listing = self.invoke("7z", "l", key)
                    self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
                    path = case["name"] + "/link"
                    if key == "fscrypt:none":
                        chunks = listing.stdout.decode().replace("\r\n", "\n").split("\n\n")
                        chunk = next(part for part in chunks if "Symbolic Link = " + expected in part)
                        path = next(line[7:] for line in chunk.splitlines() if line.startswith("Path = "))
                        self.assertTrue(path.replace("\\", "/").startswith(case["name"] + "/"))
                    result = self.invoke("7z", "x", key, path)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected.encode())
        finally:
            self.image_bytes = original
            self.image.write_bytes(original)

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
