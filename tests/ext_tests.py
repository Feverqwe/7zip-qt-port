"""Production ext sparse reads, extent boundaries and malformed directories."""
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib
import base64
import hashlib
import json
from types import SimpleNamespace
from ext_fixture import Volume, NAME
from ext_modern_fixture import ModernVolume
from ext_verity_fixture import VerityVolume
from ext_xattr_fixture import XattrVolume
from ext_block_fixture import BlockVolume
from ext_symlink_fixture import SymlinkVolume
from ext_inode_fixture import inode_volume
from ext_directory_fixture import DirectoryVolume

RUNTIME = Path(sys.argv.pop(1))
MODULE = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""


class ExtTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-ext-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        core = "7z.dll" if sys.platform == "win32" else "7z.so"
        for name in ("7z" + SUFFIX, "7zz" + SUFFIX, core):
            shutil.copy2(RUNTIME / name, cls.runtime / name)
        (cls.runtime / "Formats").mkdir()
        shutil.copy2(MODULE, cls.runtime / "Formats" / MODULE.name)

    def run_image(self, v, engine="7z", command="x", name=NAME, streams=False, properties=(), memory_limit=None):
        with tempfile.TemporaryDirectory(dir=self.runtime) as temporary:
            path = Path(temporary) / "образ ext 空.img"
            path.write_bytes(v.data)
            args = [str(self.runtime / (engine + SUFFIX)), command, "-tExt", "-sccUTF-8", "-bsp0"]
            if streams or (name and ':' in name):
                args += ["-sns"]
            else:
                args += ["-sns-"]
            args += ["-so", "-bso0", "-bse2"] if command == "x" else ["-slt"]
            args += list(properties)
            args += ["--", str(path)]
            if name is not None:
                args += [name]
            def set_limit():
                import resource
                resource.setrlimit(resource.RLIMIT_AS, (memory_limit, memory_limit))
            result = subprocess.run(args, stdin=subprocess.DEVNULL, capture_output=True, timeout=10,
                                    preexec_fn=set_limit if memory_limit is not None else None)
            self.assertEqual(path.read_bytes(), v.data)
            return result

    def test_builtin_verity_signatures_use_explicit_archive_trust(self):
        fixtures = json.loads((Path(__file__).parent / "fixtures/ext4/verity-signatures.json").read_text())
        trusted = "-mverity.trust=" + fixtures["root"]
        foreign = "-mverity.trust=" + fixtures["collision"]
        for record in fixtures["cases"]:
            with self.subTest(algorithm=record["name"]):
                volume = SimpleNamespace(data=zlib.decompress(base64.b64decode(record["image"])))
                unsigned = record["name"] == "unsigned"
                result = self.run_image(volume, properties=[trusted, "-mverity.require=off"])
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(len(result.stdout), record["file_size"])
                self.assertEqual(hashlib.sha256(result.stdout).hexdigest(), record["sha256"])
                required = self.run_image(volume, properties=[trusted, "-mverity.require+"])
                self.assertEqual(required.returncode == 0, not unsigned, required.stderr.decode(errors="replace"))
                if unsigned:
                    continue
                for properties in ([], [foreign], [trusted, "-mverity.trust=not-hex"]):
                    rejected = self.run_image(volume, properties=properties)
                    self.assertNotEqual(rejected.returncode, 0)
                    self.assertEqual(rejected.stdout, b"")
                damaged = bytearray(volume.data)
                damaged[record["signature_offset"] + record["signature_size"] - 1] ^= 1
                rejected = self.run_image(SimpleNamespace(data=damaged), properties=[trusted, "-mverity.require=off"])
                self.assertNotEqual(rejected.returncode, 0)
                self.assertEqual(rejected.stdout, b"")

    def test_extended_inode_times_match_between_engines(self):
        for layout in ("modern-time", "modern-time-zero"):
            with self.subTest(layout=layout):
                fixture = subprocess.run([sys.executable, str(Path(__file__).with_name("ext_fixture.py")), layout],
                                         capture_output=True, check=True)
                volume = SimpleNamespace(data=fixture.stdout)
                timestamps = []
                for engine in ("7z", "7zz"):
                    result = self.run_image(volume, engine=engine, command="l")
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    fields = [line for line in result.stdout.replace(b"\r\n", b"\n").splitlines()
                              if line.startswith((b"Modified = ", b"Created = ", b"Accessed = ", b"Changed = "))]
                    self.assertTrue(any(b"2106-" in line for line in fields), fields)
                    if layout == "modern-time-zero":
                        self.assertTrue(any(b"2242-" in line for line in fields), fields)
                    timestamps.append(fields)
                self.assertEqual(timestamps[0], timestamps[1])

    def test_file_references_cannot_expose_filesystem_metadata(self):
        for mapping in ("direct", "unused-direct", "indirect-index", "indirect-data",
                        "extent", "extent-unwritten", "extent-index", "xattr"):
            for target in ("super", "gdt", "block-map", "inode-map", "table", "journal"):
                with self.subTest(mapping=mapping, target=target):
                    result = self.run_image(BlockVolume(mapping, target))
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")

    def test_reserved_journal_blocks_remain_readable_by_their_owner(self):
        volume = BlockVolume(target="journal-owner")
        result = self.run_image(volume, name="[SYS]/Journal")
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(result.stdout, volume.journal_payload)
        result = self.run_image(volume)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(result.stdout, volume.read(0, volume.size))

    def test_overlapping_metadata_descriptors_are_rejected(self):
        result = self.run_image(BlockVolume(mapping="descriptor-overlap"))
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, b"")

    def test_symlink_cluster_ea_inode_and_huge_block_geometry(self):
        for case in ("bigalloc", "bigalloc-huge", "ea-slow", "ea-fast-extents"):
            volume = SymlinkVolume(case)
            for engine in ("7z", "7zz"):
                with self.subTest(case=case, engine=engine):
                    result = self.run_image(volume, engine, "l", "link")
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertIn("Symbolic Link = " + volume.target, result.stdout.decode("utf-8"))

    def test_invalid_symlink_flags_and_fast_target_lengths_are_rejected(self):
        for case in ("append", "immutable", "embedded-nul", "unterminated", "empty"):
            for engine in ("7z", "7zz"):
                with self.subTest(case=case, engine=engine):
                    result = self.run_image(SymlinkVolume(case), engine, "l", None)
                    self.assertNotEqual(result.returncode, 0)
        result = self.run_image(SymlinkVolume("ea-slow-alias"))
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, b"")

    def test_empty_files_do_not_bypass_root_extent_validation(self):
        for fault in ("magic", "capacity", "entries", "wrapped-logical", "metadata-alias"):
            with self.subTest(fault=fault):
                volume = Volume("extent")
                root = volume.file_inode + 40
                volume.put32(volume.file_inode + 4, 0)
                if fault == "magic":
                    struct.pack_into("<H", volume.data, root, 0)
                elif fault == "capacity":
                    struct.pack_into("<H", volume.data, root + 4, 5)
                elif fault == "entries":
                    struct.pack_into("<H", volume.data, root + 2, 5)
                elif fault == "wrapped-logical":
                    volume.put32(root + 12, 0xFFFFFFFF)
                else:
                    volume.put32(root + 20, volume.table)
                result = self.run_image(volume, command="l", name=None)
                self.assertNotEqual(result.returncode, 0)

    def test_named_inodes_require_linux_read_acceptance(self):
        for case in ("mode-zero", "mode-unknown", "unlinked", "casefold", "reserved-alias",
                     "quota-alias", "negative-size", "bitmap-overflow", "extent-overflow"):
            with self.subTest(case=case):
                result = self.run_image(inode_volume(case))
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_legacy_inode_numbers_and_extra_inode_requirements(self):
        for engine in ("7z", "7zz"):
            volume = inode_volume("legacy-control")
            result = self.run_image(volume, engine)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
            self.assertEqual(result.stdout, volume.read(0, volume.size))
        result = self.run_image(inode_volume("legacy-reserved-alias"))
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, b"")
        for offset in (0x15C, 0x15E):
            volume = ModernVolume()
            volume.put32(1024 + 100, 2 | 0x400 | 0x40)
            struct.pack_into("<H", volume.data, 1024 + offset, 132)
            volume.refresh()
            result = self.run_image(volume)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(result.stdout, b"")

    def test_sparse_inode_size_limits_do_not_require_dense_allocation(self):
        for case in ("bitmap-boundary", "extent-boundary"):
            with self.subTest(case=case):
                volume = inode_volume(case)
                result = self.run_image(volume, command="l")
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertIn(("Size = " + str(volume.declared_size)).encode(), result.stdout)

    def test_directory_holes_are_not_checksums_of_zero_metadata(self):
        for mapping in ("indirect", "extent", "unwritten"):
            volume = DirectoryVolume(mapping)
            for engine in ("7z", "7zz"):
                with self.subTest(mapping=mapping, engine=engine):
                    listing = self.run_image(volume, engine, "l", None)
                    self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
                    self.assertIn(b"Path = after-hole.bin", listing.stdout)
                    result = self.run_image(volume, engine, name="after-hole.bin")
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, volume.read(0, volume.size))
        for volume in (DirectoryVolume("allocated-zero"), DirectoryVolume(fault="checksum")):
            result = self.run_image(volume, command="l", name=None)
            self.assertNotEqual(result.returncode, 0)

    @unittest.skipUnless(sys.platform == "linux", "Linux child address-space limit")
    def test_largedir_reads_past_four_gib_with_bounded_memory(self):
        volume = DirectoryVolume(tail_logical=(1 << 32) // 1024 + 1)
        for engine in ("7z", "7zz"):
            with self.subTest(engine=engine):
                listing = self.run_image(volume, engine, "l", None, memory_limit=512 << 20)
                self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
                self.assertIn(b"Path = after-hole.bin", listing.stdout)
                result = self.run_image(volume, engine, name="after-hole.bin", memory_limit=512 << 20)
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(result.stdout, volume.read(0, volume.size))

    def test_casefold_encoding_is_recognized_without_changing_stored_names(self):
        for flags in (0, 1, 0x8000):
            volume = DirectoryVolume(encoding=1, encoding_flags=flags)
            result = self.run_image(volume)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
            self.assertEqual(result.stdout, volume.read(0, volume.size))
        for encoding in (0, 0xFFFF):
            volume = DirectoryVolume(encoding=encoding)
            result = self.run_image(volume)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(result.stdout, b"")
            # The encoding fields have no meaning without the casefold feature.
            volume.put32(1024 + 96, volume.u32(1024 + 96) & ~0x20000)
            volume.put32(volume.root_inode + 32, volume.u32(volume.root_inode + 32) & ~0x40000000)
            volume.refresh()
            result = self.run_image(volume)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))

    def test_presented_directory_names_require_real_path_components(self):
        for name in (b"", b"bad/name.bin"):
            for inline in (False, True):
                volume = ModernVolume(inline=True) if inline else DirectoryVolume()
                record = volume.inline_dir_inode + 44 if inline else volume.root_block * 1024 + 24
                volume.data[record + 6] = len(name)
                volume.data[record + 8:record + 8 + len(name)] = name
                volume.refresh()
                for engine in (("7z",) if inline else ("7z", "7zz")):
                    with self.subTest(name=name, inline=inline, engine=engine):
                        result = self.run_image(volume, engine, "l", None)
                        self.assertNotEqual(result.returncode, 0)

    def test_unlinked_inventory_slots_cannot_become_unknown_files(self):
        result = self.run_image(inode_volume("orphan-unreferenced"), command="l", name=None)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertNotIn(b"[UNKNOWN]", result.stdout)

    def test_special_inode_device_encoding_is_not_a_data_block_pointer(self):
        for engine in ("7z", "7zz"):
            with self.subTest(engine=engine):
                empty = self.run_image(inode_volume("special-empty"), engine)
                self.assertEqual(empty.returncode, 0, empty.stderr.decode(errors="replace"))
                self.assertEqual(empty.stdout, b"")
                rejected = self.run_image(inode_volume("special-payload"), engine)
                self.assertNotEqual(rejected.returncode, 0)
                self.assertEqual(rejected.stdout, b"")

    def test_xattr_names_values_and_acl_serialization(self):
        for kind in ("xattrs", "xattrs-signed", "xattrs-lustre"):
            volume = XattrVolume(kind)
            listing = self.run_image(volume, command="l", name=None, streams=True)
            self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
            plain = self.run_image(volume, command="l", name=None)
            self.assertEqual(plain.returncode, 0, plain.stderr.decode(errors="replace"))
            self.assertNotIn(b"Alternate Stream = +", plain.stdout)
            self.assertIn(b"Path = data.bin:$XATTR.user.comment\n", plain.stdout.replace(b"\r\n", b"\n"))
            for name, expected in volume.files.items():
                with self.subTest(kind=kind, name=name):
                    self.assertIn(name.encode(), listing.stdout)
                    result = self.run_image(volume, name=name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

    def test_ea_inode_integrity_cannot_return_value_bytes(self):
        for fault in ("bad-data", "bad-hash", "bad-entry-hash", "missing-flag", "no-links",
                      "many-links", "no-refs", "size", "self", "unwritten", "bad-acl"):
            with self.subTest(fault=fault):
                result = self.run_image(XattrVolume("xattrs-" + fault), name="data.bin:$XATTR.user.large")
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_sparse_indirect_and_extent_reads(self):
        for engine in ("7z", "7zz"):
            for layout in ("direct", "single", "double-hole", "double-subtree", "sparse-short",
                           "extent", "extent-tree", "extent-depth5", "extent-two-branches"):
                with self.subTest(engine=engine, layout=layout):
                    v = Volume(layout)
                    result = self.run_image(v, engine)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, v.read(0, v.size))

    def test_verity_empty_short_salted_sparse_and_multilevel_reads(self):
        for algorithm, salt, size in (("sha256", b"", 0), ("sha256", b"", 100),
                                       ("sha512", b"pepper", 100),
                                       ("sha256", b"", 35 * 1024 + 17),
                                       ("sha512", bytes(range(32)), 35 * 1024 + 17)):
            with self.subTest(algorithm=algorithm, salt=salt, size=size):
                volume = VerityVolume(algorithm, salt, size)
                result = self.run_image(volume)
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(result.stdout, volume.read(0, size))

    def test_verity_corruption_cannot_return_success(self):
        for fault in ("data", "tree", "root", "version", "reserved", "algorithm", "salt-size",
                      "data-size", "block-size", "signature-size", "descriptor-size"):
            with self.subTest(fault=fault):
                volume = VerityVolume("sha512", b"salt")
                descriptor = volume.descriptor_physical
                offsets = {"data": volume.first_data, "tree": volume.metadata_physical * 1024,
                           "root": descriptor + 16, "version": descriptor, "reserved": descriptor + 112,
                           "algorithm": descriptor + 1, "salt-size": descriptor + 3,
                           "data-size": descriptor + 8, "block-size": descriptor + 2,
                           "signature-size": descriptor + 4, "descriptor-size": descriptor + 1020}
                if fault in ("algorithm", "salt-size", "block-size"):
                    volume.data[offsets[fault]] = 255
                else:
                    if fault == "descriptor-size":
                        volume.put32(offsets[fault], 1)
                    else:
                        volume.data[offsets[fault]] ^= 1
                result = self.run_image(volume)
                self.assertNotEqual(result.returncode, 0)

    def test_verity_descriptor_padding_is_accepted(self):
        volume = VerityVolume()
        # Linux permits descriptor padding beyond the fixed header/signature.
        volume.put32(volume.descriptor_physical + 1020, 257)
        result = self.run_image(volume)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(result.stdout, volume.read(0, volume.size))

    def test_linux_creator_verity_files(self):
        path = Path(__file__).parent / "fixtures" / "ext4" / "verity.ext4.zlib"
        class Image:
            data = zlib.decompress(path.read_bytes())
        volume = VerityVolume()
        for name, expected in ((NAME, volume.read(0, volume.size)),
                               ("sha512.bin", volume.read(0, volume.size)),
                               ("short.bin", bytes(range(100))), ("empty.bin", b"")):
            with self.subTest(name=name):
                result = self.run_image(Image, name=name)
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(result.stdout, expected)

    def test_standalone_cannot_publish_unverified_verity_data(self):
        volume = VerityVolume(size=100)
        volume.data[volume.first_data] ^= 1
        result = self.run_image(volume, engine="7zz")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, b"")

    def test_modern_metadata_and_full_inline_payload(self):
        # Linux C readers live in the dynamic format module; standalone 7zz
        # retains the independent 7-Zip LGPL classic reader.
        for engine in ("7z",):
            for inline, seed, descriptor in ((False, False, 64), (False, True, 128),
                                              (True, False, 64), (True, True, 128)):
                with self.subTest(engine=engine, inline=inline, seed=seed, descriptor=descriptor):
                    volume = ModernVolume(inline, seed, descriptor)
                    result = self.run_image(volume, engine)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    expected = volume.payload if inline else volume.read(0, volume.size)
                    self.assertEqual(result.stdout, expected)
                    if inline:
                        result = self.run_image(volume, engine, name="nested/alias.bin")
                        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                        self.assertEqual(result.stdout, expected)

    def test_metadata_corruption_is_rejected(self):
        for location in ("super", "group", "inode-map", "block-map", "inode", "dir", "extent"):
            with self.subTest(location=location):
                volume = ModernVolume()
                offsets = {"super": 1024 + 120, "group": volume.gdt * 1024 + 12,
                           "inode-map": volume.inode_map * 1024 + 1,
                           "block-map": volume.block_map * 1024 + 4,
                           "inode": volume.file_inode + 8, "dir": volume.root_block * 1024 + 40,
                           "extent": volume.extent_blocks[0] * 1024 + 8}
                volume.data[offsets[location]] ^= 1
                result = self.run_image(volume)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_indexed_directory_checksums_and_count_bounds(self):
        volume = ModernVolume(indexed=True)
        result = self.run_image(volume)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(result.stdout, volume.read(0, volume.size))
        for fault in ("root-checksum", "index-checksum", "count", "limit", "leaf-checksum"):
            with self.subTest(fault=fault):
                volume = ModernVolume(indexed=True)
                if fault == "root-checksum":
                    volume.data[volume.root_block * 1024 + 1020] ^= 1
                elif fault == "index-checksum":
                    volume.data[volume.index_blocks[1][0] * 1024 + 1020] ^= 1
                elif fault == "leaf-checksum":
                    volume.data[volume.dir_leaves[0] * 1024 + 1020] ^= 1
                else:
                    struct.pack_into("<H", volume.data, volume.root_block * 1024 +
                                     (34 if fault == "count" else 32), 65535)
                result = self.run_image(volume)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_indexed_short_names_and_empty_leaf_checksum_dispatch(self):
        for empty in (False, True):
            volume = ModernVolume(indexed=True, short_leaf=True, empty_leaf=empty)
            for engine in ("7z", "7zz"):
                with self.subTest(empty=empty, engine=engine):
                    for name in ("a", NAME):
                        result = self.run_image(volume, engine, name=name)
                        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                        self.assertEqual(result.stdout, volume.read(0, volume.size))
                    listed = self.run_image(volume, engine, command="l", name=None)
                    self.assertEqual(listed.returncode, 0, listed.stderr.decode(errors="replace"))
                    self.assertIn(b"Path = a\n", listed.stdout.replace(b"\r\n", b"\n"))
            for block in volume.dir_leaves:
                damaged = bytearray(volume.data)
                damaged[block * 1024 + 1020] ^= 1
                result = self.run_image(SimpleNamespace(data=damaged))
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_linux_creator_inline_bigalloc_and_distributed_descriptors(self):
        fixtures = Path(__file__).parent / "fixtures" / "ext4"
        for case in ("inline", "bigalloc", "meta-bg"):
            with self.subTest(case=case):
                class Image:
                    data = zlib.decompress((fixtures / (case + ".ext4.zlib")).read_bytes())
                for name, expected in ((NAME, bytes(range(100))), ("big.bin", bytes(range(256)) * 257)):
                    result = self.run_image(Image, name=name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

    def test_inline_xattr_and_directory_corruption(self):
        for fault in ("overlap", "value-size", "external", "missing-data", "record", "inode-number"):
            with self.subTest(fault=fault):
                volume = ModernVolume(inline=True)
                if fault == "overlap":
                    struct.pack_into("<H", volume.data, volume.file_inode + 166, 0)
                elif fault == "value-size":
                    volume.put32(volume.file_inode + 172, 1 << 25)
                elif fault == "external":
                    volume.put32(volume.file_inode + 168, 2)
                elif fault == "missing-data":
                    volume.data[volume.file_inode + 180] = ord("X")
                elif fault == "record":
                    struct.pack_into("<H", volume.data, volume.inline_dir_inode + 48, 55)
                else:
                    volume.put32(volume.inline_dir_inode + 44, 1000)
                volume.refresh()
                result = self.run_image(volume)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_unknown_layout_and_encrypted_content_cannot_succeed(self):
        for encrypted in (False, True):
            with self.subTest(encrypted=encrypted):
                volume = ModernVolume()
                features = struct.unpack_from("<I", volume.data, 1024 + 96)[0]
                volume.put32(1024 + 96, features | (0x10000 if encrypted else 1 << 31))
                if encrypted:
                    volume.put32(volume.file_inode + 32, (1 << 19) | 0x800)
                volume.refresh()
                result = self.run_image(volume)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_creator_os_and_encryption_feature_compatibility(self):
        # Check extraction, including accepted boundaries: a creator OS alone
        # must not reject readable data, nor may an unused encryption byte.
        for creator, features, level, accepted in (
                (1, 0, 0, True), (1, 0x80, 0, False), (1, 0x400, 0, False),
                (0, 0x80 | 0x400, 0, True), (2, 0x80, 0, True),
                (3, 0x80, 0, True), (4, 0x80, 0, True),
                (0, 0x80 | 0x10000, 0, True),
                (0, 0x80 | 0x10000, 1, False),
                (0, 0x80 | 0x10000, 255, False), (0, 0x80, 255, True)):
            with self.subTest(creator=creator, features=features, level=level):
                volume = ModernVolume(descriptor_size=64 if features & 0x80 else 32)
                volume.put32(1024 + 72, creator)
                volume.put32(1024 + 96, 2 | 0x40 | features)
                volume.data[1024 + 0x176] = level
                volume.refresh()
                result = self.run_image(volume)
                if accepted:
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, volume.read(0, volume.size))
                else:
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")

    def test_external_xattr_checksum_and_value_bounds(self):
        volume = ModernVolume()
        volume.add_external_xattr()
        result = self.run_image(volume)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(result.stdout, volume.read(0, volume.size))
        for fault in ("checksum", "header", "overlap"):
            with self.subTest(fault=fault):
                volume = ModernVolume()
                volume.add_external_xattr()
                start = volume.xattr_block * 1024
                if fault == "checksum":
                    volume.data[start + 16] ^= 1
                elif fault == "header":
                    volume.put32(start + 8, 2)
                    volume.refresh()
                else:
                    struct.pack_into("<BBHIII", volume.data, start + 32, 4, 1, 8, 0, 64, 0)
                    volume.data[start + 48:start + 52] = b"name"
                    volume.refresh()
                result = self.run_image(volume)
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, b"")

    def test_complete_group_and_fast_symlink_with_xattr(self):
        for engine in ("7z", "7zz"):
            v = Volume("direct", full_group=True)
            result = self.run_image(v, engine)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
            self.assertEqual(result.stdout, v.read(0, v.size))
            v = Volume("fastlink-xattr")
            result = self.run_image(v, engine, "l", None)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
            self.assertIn("Symbolic Link = " + NAME, result.stdout.decode("utf-8"))

    def test_invalid_extent_headers_ranges_and_parent_boundaries(self):
        for engine in ("7z", "7zz"):
            for fault in ("zero-max", "too-large-max", "entries-over-max", "zero-length", "overlap",
                          "physical-range", "child-first", "index-order", "child-overrun",
                          "conflicting-flags", "acl-range", "empty-child"):
                with self.subTest(engine=engine, fault=fault):
                    v = Volume("extent-tree" if fault.startswith("child") or fault == "empty-child" else "extent")
                    root = v.file_inode + 40
                    if fault == "zero-max":
                        struct.pack_into("<H", v.data, root + 4, 0)
                    elif fault == "too-large-max":
                        struct.pack_into("<H", v.data, root + 4, 5)
                    elif fault == "entries-over-max":
                        struct.pack_into("<H", v.data, root + 4, 1)
                    elif fault == "zero-length":
                        struct.pack_into("<H", v.data, root + 16, 0)
                    elif fault == "overlap":
                        struct.pack_into("<I", v.data, root + 24, 2)
                    elif fault == "physical-range":
                        struct.pack_into("<I", v.data, root + 20, v.blocks)
                    elif fault == "child-first":
                        struct.pack_into("<I", v.data, root + 12, 1)
                    elif fault == "conflicting-flags":
                        v.put32(v.file_inode + 32, (1 << 19) | (1 << 28))
                    elif fault == "acl-range":
                        v.put32(v.file_inode + 104, v.blocks)
                    elif fault == "empty-child":
                        struct.pack_into("<H", v.data, v.extent_blocks[0] * v.block_size + 2, 0)
                    elif fault == "index-order":
                        block = v.allocate(v.extent_header(0, [(2, 1, 8)], 4))
                        v.data[root:root + 60] = v.extent_header(1, [(2, block), (2, block)], 4)
                    else:
                        block = v.extent_blocks[0]
                        v.data[root:root + 60] = v.extent_header(1, [(2, block), (3, block)], 4)
                    result = self.run_image(v, engine)
                    self.assertIn(result.returncode, (1, 2), result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, b"")

    def test_invalid_directory_records_terminate(self):
        for engine in ("7z", "7zz"):
            for value in (0, 4, 10, 1020):
                with self.subTest(engine=engine, record_length=value):
                    v = Volume("direct")
                    pos = v.root_block * v.block_size + 24
                    struct.pack_into("<I", v.data, pos, 0)
                    struct.pack_into("<H", v.data, pos + 4, value)
                    result = self.run_image(v, engine, "l", None)
                    self.assertIn(result.returncode, (1, 2), result.stderr)

    def test_directory_records_cannot_cross_blocks_or_use_unaligned_lengths(self):
        for engine in ("7z", "7zz"):
            for crossing in (False, True):
                with self.subTest(engine=engine, crossing=crossing):
                    v = Volume("direct")
                    start = v.root_block * v.block_size
                    directory_size = v.block_size * (2 if crossing else 1)
                    if crossing:
                        v.put32(v.root_inode + 4, directory_size)
                        v.put32(v.root_inode + 28, 4)
                        v.put32(v.root_inode + 44, v.root_block + 1)
                    v.data[start + 24:start + directory_size] = bytes(directory_size - 24)
                    first_length = 1020 if crossing else 10
                    struct.pack_into("<H", v.data, start + 28, first_length)
                    struct.pack_into("<H", v.data, start + 24 + first_length + 4,
                                     directory_size - 24 - first_length)
                    result = self.run_image(v, engine, "l", None)
                    self.assertIn(result.returncode, (1, 2), result.stderr)

    def test_large_block_directory_empty_record_encodings(self):
        for engine in ("7z", "7zz"):
            for encoded in (0, 0xFFFF):
                with self.subTest(engine=engine, encoded=encoded):
                    v = Volume("direct", block_bits=16)
                    block = v.allocate()
                    struct.pack_into("<H", v.data, block * v.block_size + 4, encoded)
                    v.put32(v.root_inode + 4, 2 * v.block_size)
                    v.put32(v.root_inode + 28, 2 * v.block_size // 512)
                    v.put32(v.root_inode + 44, block)
                    v.superblock()
                    result = self.run_image(v, engine)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, v.read(0, v.size))


if __name__ == "__main__":
    unittest.main()
