"""Read results and rejection of malformed SquashFS through both delivered engines."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
import zlib
import json
import urllib.parse
from squashfs_fixture import volume, legacy_volume, creator_payload

RUNTIME = Path(sys.argv.pop(1))
MODULE = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""


class SquashfsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-squashfs-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        core = "7z.dll" if sys.platform == "win32" else "7z.so"
        for name in ("7z" + SUFFIX, "7zz" + SUFFIX, core):
            shutil.copy2(RUNTIME / name, cls.runtime / name)
        (cls.runtime / "Formats").mkdir()
        shutil.copy2(MODULE, cls.runtime / "Formats" / MODULE.name)

    def run_image(self, engine, data, command="t", name=None, streams=False):
        with tempfile.TemporaryDirectory(dir=self.runtime) as temporary:
            path = Path(temporary) / "образ 空.squashfs"
            path.write_bytes(data)
            args = [str(self.runtime / (engine + SUFFIX)), command, "-tSquashFS", "-sccUTF-8", "-bsp0"]
            args += ["-sns" if streams else "-sns-"]
            if command == "x":
                args += ["-so", "-bso0", "-bse2"]
            elif command == "l":
                args += ["-slt"]
            args += ["--", str(path)]
            if name:
                args.append(name)
            result = subprocess.run(args, capture_output=True, timeout=10)
            self.assertEqual(path.read_bytes(), data)
            return result

    def test_blocks_fragments_sparse_and_unicode(self):
        for engine in ("7z", "7zz"):
            for compressed in (False, True):
                data, files = volume(compressed=compressed)
                for name, expected in files.items():
                    with self.subTest(engine=engine, compressed=compressed, name=name):
                        result = self.run_image(engine, data, "x", name)
                        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                        self.assertEqual(result.stdout, expected)

    def test_optional_padding_preserves_physical_size_and_file_data(self):
        import struct
        data, files = volume(compressed=True)
        used = struct.unpack_from("<Q", data, 40)[0]
        aligned, _ = volume(fault="padding-aligned", compressed=True)
        variants = ((data, len(data)), (data[:used], used),
                    (data[:used] + b"\0", used), (data[:used] + b"nonzero tail", used),
                    (aligned, len(aligned)))
        for engine in ("7z", "7zz"):
            for image, expected_size in variants:
                with self.subTest(engine=engine, size=len(image), physical=expected_size):
                    listing = self.run_image(engine, image, "l")
                    self.assertIn(listing.returncode, (0, 1), listing.stderr.decode(errors="replace"))
                    text = listing.stdout.decode("utf-8").replace("\r\n", "\n")
                    self.assertIn(f"Physical Size = {expected_size}\n", text)
                    result = self.run_image(engine, image, "x", "alpha")
                    self.assertIn(result.returncode, (0, 1), result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, files["alpha"])

    def test_linux_lz4_and_modern_lzo_rle(self):
        for codec in ("lz4", "lzo-rle"):
            data, files = volume(codec=codec)
            for name, expected in files.items():
                with self.subTest(codec=codec, name=name):
                    result = self.run_image("7z", data, "x", name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

    def test_linux_reads_one_verified_stream_per_compressed_block(self):
        for codec in ("zlib", "xz", "zstd"):
            for scope in ("data", "inode"):
                for kind in (("tail", "concat", "multiblock", "truncated", "damaged")
                             if codec == "xz" else ("tail", "concat", "truncated", "damaged")):
                    with self.subTest(codec=codec, scope=scope, kind=kind):
                        data, files = volume(fault=f"frame-{codec}-{scope}-{kind}")
                        result = self.run_image("7z", data, "x", "alpha")
                        if kind in ("tail", "concat", "multiblock"):
                            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                            self.assertEqual(result.stdout, files["alpha"])
                        else:
                            self.assertNotEqual(result.returncode, 0)
                            self.assertEqual(result.stdout, b"")

    def test_data_and_fragment_alias_preserve_each_compression_mode(self):
        for fault in ("cache-data-compressed", "cache-fragment-compressed"):
            data, files = volume(fault=fault)
            for engine in ("7z", "7zz"):
                for name in ("alpha", "данные 空"):
                    with self.subTest(fault=fault, engine=engine, name=name):
                        result = self.run_image(engine, data, "x", name)
                        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                        self.assertEqual(result.stdout, files[name])

    def test_special_device_metadata_and_readable_siblings(self):
        data, files = volume(fault="special-inodes", xattrs=True)
        listing = self.run_image("7z", data, "l", streams=True)
        self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
        records = {}
        for block in listing.stdout.decode("utf-8").replace("\r\n", "\n").split("\n\n"):
            fields = dict(line.split(" = ", 1) for line in block.splitlines() if " = " in line)
            if "Path" in fields:
                records[fields["Path"]] = fields
        for name, major, minor in (("block", 0xabc, 0xdef12), ("char", 0, 0),
                                    ("extended-block", 0xfff, 0xfffff),
                                    ("extended-char", 0x123, 0x45678)):
            with self.subTest(name=name):
                self.assertEqual(records[name]["Device Major"], str(major))
                self.assertEqual(records[name]["Device Minor"], str(minor))
        for name, expected in files.items():
            result = self.run_image("7z", data, "x", name)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
            self.assertEqual(result.stdout, expected)
        for name in ("extended-block", "extended-char", "extended-fifo", "extended-socket"):
            attribute = name + ":$XATTR.user.note"
            self.assertEqual(records[attribute].get("Device Major", ""), "")
            self.assertEqual(records[attribute].get("Device Minor", ""), "")
            result = self.run_image("7z", data, "x", attribute, streams=True)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
            self.assertEqual(result.stdout, b"value")

    def test_core_only_standalone_rejects_new_decoder_features(self):
        for codec in ("lz4", "lzo-rle"):
            result = self.run_image("7zz", volume(codec=codec)[0], "x", "alpha")
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(result.stdout, b"")

    def test_compressor_options_reject_invalid_bootstrap_and_versions(self):
        import struct
        for fault in ("missing", "version", "short", "compressed", "truncated"):
            data = bytearray(volume(codec="lz4")[0])
            if fault == "missing":
                struct.pack_into("<H", data, 24, 0)
            elif fault == "version":
                struct.pack_into("<I", data, 98, 2)
            elif fault == "short":
                struct.pack_into("<H", data, 96, 0x8004)
            elif fault == "compressed":
                struct.pack_into("<H", data, 96, 8)
            else:
                struct.pack_into("<Q", data, 40, 100)
            with self.subTest(fault=fault):
                result = self.run_image("7z", data)
                self.assertNotEqual(result.returncode, 0, result.stdout.decode(errors="replace"))

    def test_official_creator_fixtures_on_every_platform(self):
        fixtures = Path(__file__).parent / "fixtures/squashfs"
        records = json.loads((fixtures / "manifest.json").read_text())["images"]
        for path in sorted(fixtures.glob("*.squashfs.zlib")):
            if "attributes" in records[path.name.removesuffix(".squashfs.zlib")]:
                continue  # Different payloads are checked by the native-xattr scenario.
            data = zlib.decompress(path.read_bytes())
            for engine in ("7z", "7zz"):
                if path.name.startswith("lz4") and engine == "7zz":
                    continue
                for name, expected in (("данные 空", creator_payload()),
                                       ("hardlink", creator_payload()), ("small", b"fragment")):
                    with self.subTest(image=path.name, engine=engine, name=name):
                        result = self.run_image(engine, data, "x", name)
                        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                        self.assertEqual(result.stdout, expected)

    def test_xz_dictionary_options_are_checked_before_listing(self):
        import struct
        fixture = Path(__file__).parent / "fixtures/squashfs/xz.squashfs.zlib"
        original = zlib.decompress(fixture.read_bytes())
        for dictionary in (0, 5, 0xffffffff):
            data = bytearray(original)
            struct.pack_into("<I", data, 98, dictionary)
            with self.subTest(dictionary=dictionary):
                result = self.run_image("7z", data, "l")
                self.assertNotEqual(result.returncode, 0, result.stdout.decode(errors="replace"))

    def test_lz4_option_flags_follow_linux_compatibility(self):
        import struct
        data, files = volume(codec="lz4")
        data = bytearray(data)
        struct.pack_into("<I", data, 102, 0xffffffff)
        result = self.run_image("7z", data, "x", "alpha")
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(result.stdout, files["alpha"])

    def test_empty_filesystem_without_fragment_index(self):
        for engine in ("7z", "7zz"):
            result = self.run_image(engine, volume(no_fragments=True)[0])
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))

    def test_legacy_v3_little_and_big_endian_read_extension(self):
        for engine in ("7z", "7zz"):
            for endian in ("<", ">"):
                for compressed in (False, True):
                    with self.subTest(engine=engine, endian=endian, compressed=compressed):
                        data, expected = legacy_volume(endian, compressed)
                        result = self.run_image(engine, data, "x", "alpha")
                        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                        self.assertEqual(result.stdout, expected)

    def test_corrupt_metadata_and_data_boundaries(self):
        for engine in ("7z", "7zz"):
            for fault in ("fragment-large", "fragment-zero", "data-zero", "fragment-outside", "data-outside", "cross-block-inode",
                          "cycle", "count-wrap", "name-nul", "name-slash", "root-not-directory",
                          "uid-index", "negative-size", "metadata-bomb"):
                with self.subTest(engine=engine, fault=fault):
                    if engine == "7z" and fault == "uid-index":
                        continue  # Unknown ownership is exposed with a limited-read diagnostic.
                    result = self.run_image(engine, volume(fault)[0])
                    self.assertNotEqual(result.returncode, 0, result.stdout.decode(errors="replace"))

    def test_xattr_tables_and_out_of_line_values(self):
        data, files = volume(xattrs=True)
        for name in ("alpha", "hardlink"):
            result = self.run_image("7z", data, "x", name)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
            self.assertEqual(result.stdout, files[name])
        for fault in ("xattr-ids-zero", "xattr-index-outside", "xattr-count-negative", "xattr-id-invalid", "xattr-ool-outside"):
            with self.subTest(fault=fault):
                data, files = volume(fault, xattrs=True)
                result = self.run_image("7z", data, "x", "alpha")
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(result.stdout, files["alpha"])
                listing = self.run_image("7z", data, "l")
                self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
                self.assertIn(b"Some SquashFS extended attributes are unavailable", listing.stdout)

    def test_published_native_xattr_values_and_stream_filtering(self):
        fixtures = Path(__file__).parent / "fixtures/squashfs"
        record = json.loads((fixtures / "manifest.json").read_text())["images"]["xattr-values"]
        data = zlib.decompress((fixtures / "xattr-values.squashfs.zlib").read_bytes())
        listing = self.run_image("7z", data, "l", streams=True)
        self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
        paths = []
        for block in listing.stdout.decode("utf-8").replace("\r\n", "\n").split("\n\n"):
            lines = block.splitlines()
            if "Alternate Stream = +" in lines:
                paths += [line[7:] for line in lines if line.startswith("Path = ")]
        plain = self.run_image("7z", data, "l")
        self.assertEqual(plain.returncode, 0, plain.stderr.decode(errors="replace"))
        self.assertNotIn(b"Alternate Stream = +", plain.stdout)
        self.assertIn(b"data.bin:$XATTR.user.comment", plain.stdout)
        for name, expected in (("data.bin", bytes(range(100))), ("alias.bin", bytes(range(100))),
                               ("[ROOT]", b"ordinary root name"),
                               ("data.bin:$XATTR.user.comment", b"ordinary colon file")):
            for engine in ("7z", "7zz"):
                result = self.run_image(engine, data, "x", name)
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(result.stdout, expected)
        for host, attributes in record["attributes"].items():
            for name, expected in attributes.items():
                base = (host or "[ROOT]_") + ":$XATTR." + urllib.parse.quote(name, safe="._-")
                candidates = [path for path in paths if path == base or path.startswith(base + "#")]
                self.assertEqual(len(candidates), 1, base)
                result = self.run_image("7z", data, "x", candidates[0], streams=True)
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(result.stdout, bytes.fromhex(expected), candidates[0])

    def test_broken_indexes_preserve_the_readable_subset(self):
        fragment_faults = ("fragment-table-index-outside", "fragment-table-start-outside",
                           "fragment-count-large", "table-indexes-combined")
        faults = fragment_faults + ("id-metadata-gap", "id-table-index-outside", "id-table-start-outside",
                                   "id-count-zero", "xattr-table-start-outside", "export-index-outside",
                                   "export-table-start-outside")
        for fault in faults:
            data, files = volume(fault, xattrs=True, export=True)
            with self.subTest(fault=fault):
                listing = self.run_image("7z", data, "l")
                self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
                self.assertIn(b"metadata indexes are unavailable", listing.stdout)
                good = self.run_image("7z", data, "x", "данные 空")
                self.assertEqual(good.returncode, 0, good.stderr.decode(errors="replace"))
                self.assertEqual(good.stdout, files["данные 空"])
                first = self.run_image("7z", data, "x", "alpha")
                self.assertEqual(first.stdout, files["alpha"][:8192] if fault in fragment_faults else files["alpha"])
                self.assertEqual(first.returncode == 0, fault not in fragment_faults)

    def test_only_referenced_fragments_are_needed(self):
        for fault in ("fragment-remap", "fragment-high-index", "fragment-unused-block"):
            data, files = volume(fault, xattrs=True, export=True)
            for name in ("alpha", "hardlink", "данные 空"):
                with self.subTest(fault=fault, name=name):
                    result = self.run_image("7z", data, "x", name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, files[name])

    def test_unreachable_inode_dependencies_do_not_limit_the_namespace(self):
        for fault in ("inode-unused-owner", "inode-unused-fragment", "inode-unused-xattr"):
            data, files = volume(fault, xattrs=True, export=True)
            with self.subTest(fault=fault):
                listing = self.run_image("7z", data, "l")
                self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
                self.assertNotIn(b"Read-only limitations =", listing.stdout)
                for name in ("alpha", "hardlink", "данные 空"):
                    result = self.run_image("7z", data, "x", name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, files[name])

    def test_export_payload_is_independent_of_ordinary_file_reads(self):
        for engine in ("7z", "7zz"):
            for xattrs in (False, True):
                for fault in ("export-reference", "export-metadata-zero", "export-compressed-invalid"):
                    data, files = volume(fault, xattrs=xattrs, export=True)
                    with self.subTest(engine=engine, xattrs=xattrs, fault=fault):
                        listing = self.run_image(engine, data, "l")
                        self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
                        for name in ("alpha", "данные 空"):
                            result = self.run_image(engine, data, "x", name)
                            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                            self.assertEqual(result.stdout, files[name])
        result = self.run_image("7zz", volume("export-index-outside", export=True)[0], "l")
        self.assertNotEqual(result.returncode, 0)

    def test_partial_directory_entries_preserve_readable_files(self):
        for fault in ("directory-reference", "directory-unavailable", "directory-tail"):
            data, files = volume(fault, xattrs=True)
            with self.subTest(fault=fault):
                listing = self.run_image("7z", data, "l")
                self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
                self.assertIn(b"directories could not be read completely", listing.stdout)
                result = self.run_image("7z", data, "x", "hardlink")
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(result.stdout, files["hardlink"])
                if fault == "directory-reference":
                    missing = self.run_image("7z", data, "x", "alpha")
                    self.assertNotEqual(missing.returncode, 0)
                    self.assertEqual(missing.stdout, files["alpha"])
                elif fault == "directory-unavailable":
                    missing = self.run_image("7z", data, "x", "alpha")
                    self.assertNotEqual(missing.returncode, 0)
                    self.assertEqual(missing.stdout, b"")
                rejected = self.run_image("7zz", data, "l")
                self.assertNotEqual(rejected.returncode, 0)

    def test_directory_metadata_blocks_do_not_hide_other_directories(self):
        for fault in ("directory-block-clean", "directory-block-header", "directory-block-zlib", "directory-block-span", "directory-block-shared", "directory-block-unused"):
            data, files = volume(fault)
            with self.subTest(fault=fault):
                result = self.run_image("7z", data, "l")
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                if fault not in ("directory-block-clean", "directory-block-shared", "directory-block-unused"):
                    self.assertNotIn(b"Path = broken/payload", result.stdout)
                    self.assertNotIn(b"Path = payload", result.stdout)
                for name in ("alpha", "good 空/payload"):
                    result = self.run_image("7z", data, "x", name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, files[name])

    def test_inode_record_failures_are_local_to_their_entries(self):
        for fault in ("inode-record", "inode-unused", "inode-count-short", "inode-count-large", "inode-length"):
            data, files = volume(fault, xattrs=True)
            with self.subTest(fault=fault):
                listing = self.run_image("7z", data, "l")
                self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
                self.assertIn(b"inode records are unavailable", listing.stdout)
                result = self.run_image("7z", data, "x", "данные 空")
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(result.stdout, files["данные 空"])
                if fault == "inode-record":
                    result = self.run_image("7z", data, "x", "alpha")
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, b"")
        data, files = volume("directory-block-inode")
        result = self.run_image("7z", data, "x", "good 空/payload")
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(result.stdout, files["good 空/payload"])

    def test_inode_metadata_failures_preserve_independent_files(self):
        for fault in ("inode-block-clean", "inode-block-header", "inode-block-zlib"):
            data, files = volume(fault)
            with self.subTest(fault=fault):
                result = self.run_image("7z", data, "x", "данные 空")
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(result.stdout, files["данные 空"])
                first = self.run_image("7z", data, "x", "alpha")
                if fault == "inode-block-clean":
                    self.assertEqual(first.returncode, 0)
                    self.assertEqual(first.stdout, files["alpha"])
                else:
                    self.assertNotEqual(first.returncode, 0)
                    self.assertEqual(first.stdout, b"")

    def test_inode_records_span_short_metadata_blocks(self):
        for fault in ("inode-split", "inode-split-directory", "inode-split-link", "inode-block-unused"):
            data, files = volume(fault, xattrs=True, export=True)
            for name in ("alpha", "данные 空"):
                with self.subTest(fault=fault, name=name):
                    result = self.run_image("7z", data, "x", name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, files[name])

    def test_late_blocklist_failure_preserves_readable_prefix(self):
        for fault in ("blocklist-clean", "blocklist-header", "blocklist-zlib"):
            data, files = volume(fault)
            with self.subTest(fault=fault):
                healthy = self.run_image("7z", data, "x", "данные 空")
                self.assertEqual(healthy.returncode, 0, healthy.stderr.decode(errors="replace"))
                self.assertEqual(healthy.stdout, files["данные 空"])
                result = self.run_image("7z", data, "x", "alpha")
                if fault == "blocklist-clean":
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, files["alpha"])
                else:
                    self.assertNotEqual(result.returncode, 0)
                    self.assertEqual(result.stdout, files["alpha"][:8192])

    def test_directory_index_failure_does_not_hide_files(self):
        for fault in ("directory-index-clean", "directory-index-header", "directory-index-zlib",
                      "directory-index-name", "directory-index-count", "directory-index-inline-name"):
            data, files = volume(fault, xattrs=True, export=True)
            for name in ("alpha", "данные 空"):
                with self.subTest(fault=fault, name=name):
                    result = self.run_image("7z", data, "x", name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, files[name])
        data, files = volume("directory-block-index-recovery")
        result = self.run_image("7z", data, "x", "broken/payload")
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        self.assertEqual(result.stdout, files["broken/payload"])

    def test_directory_fields_span_short_metadata_blocks(self):
        for fault in ("directory-block-short", "directory-block-short-bad"):
            data, files = volume(fault)
            for name in ("alpha", "broken/payload"):
                with self.subTest(fault=fault, name=name):
                    result = self.run_image("7z", data, "x", name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, files[name])
            if fault == "directory-block-short":
                result = self.run_image("7z", data, "x", "good 空/payload")
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(result.stdout, files["good 空/payload"])

    def test_inode_inventory_does_not_hide_independent_files(self):
        for fault, name in (("inode-inventory-large-recovery", "данные 空"),
                            ("inode-inventory-overlap", "observed")):
            data, files = volume(fault)
            result = self.run_image("7z", data, "x", name)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
            self.assertEqual(result.stdout, files[name])
        data, _ = volume("inode-inventory-overlap")
        result = self.run_image("7z", data, "x", "recover")
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(result.stdout, b"")

    def test_ambiguous_inode_numbers_do_not_merge_unrelated_files(self):
        data, files = volume("directory-ambiguous", xattrs=True)
        with tempfile.TemporaryDirectory(dir=self.runtime) as temporary:
            root = Path(temporary)
            image, output = root / "image.squashfs", root / "output"
            image.write_bytes(data)
            result = subprocess.run([str(self.runtime / ("7z" + SUFFIX)), "x", "-y", "-o" + str(output),
                                     "--", str(image), "hardlink", "данные 空"], capture_output=True, timeout=10)
            self.assertNotEqual(result.returncode, 0)
            for name in ("hardlink", "данные 空"):
                self.assertEqual((output / name).read_bytes(), files[name])
            self.assertEqual(image.read_bytes(), data)

    def test_hardlink_identity_and_symlink_target_during_native_cli_extraction(self):
        data, files = volume(xattrs=True)
        with tempfile.TemporaryDirectory(dir=self.runtime) as temporary:
            root = Path(temporary)
            image = root / "image.squashfs"
            image.write_bytes(data)
            output = root / "output"
            result = subprocess.run([str(self.runtime / ("7z" + SUFFIX)), "x", "-y", "-o" + str(output),
                                     "--", str(image), "alpha", "hardlink"], capture_output=True, timeout=10)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
            self.assertEqual((output / "alpha").read_bytes(), files["alpha"])
            self.assertEqual((output / "hardlink").read_bytes(), files["alpha"])
            self.assertTrue((output / "alpha").samefile(output / "hardlink"))

    def test_shared_fragment_with_nonzero_offset_is_validated_before_read(self):
        for engine in ("7z", "7zz"):
            for fault in ("fragment-large", "fragment-outside"):
                result = self.run_image(engine, volume(fault)[0], "x", "данные 空")
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(result.stdout, (b"B" * 8192 + bytes(8192)) if engine == "7z" else b"")

    def test_unavailable_ownership_and_fragment_metadata(self):
        for fault in ("uid-index", "gid-index", "id-metadata-zero", "fragment-metadata-zero",
                      "fragment-index-invalid", "metadata-combined"):
            data, files = volume(fault, xattrs=True)
            fragment = fault.startswith("fragment-") or fault == "metadata-combined"
            with self.subTest(fault=fault):
                listing = self.run_image("7z", data, "l")
                self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
                self.assertIn(b"Read-only limitations =", listing.stdout)
                self.assertIn(b"Some SquashFS", listing.stdout)
                for name in ("alpha", "hardlink", "данные 空"):
                    result = self.run_image("7z", data, "x", name)
                    incomplete = fragment and name != "данные 空"
                    if incomplete:
                        self.assertNotEqual(result.returncode, 0)
                    else:
                        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, files[name][:8192] if incomplete else files[name])

    @unittest.skipUnless(shutil.which("mksquashfs"), "requires official squashfs-tools")
    def test_official_images_with_fragments_exports_xattrs_and_hardlinks(self):
        with tempfile.TemporaryDirectory(prefix="sevenzip-squashfs-native-") as temporary:
            root = Path(temporary)
            source = root / "source"
            source.mkdir()
            payload = creator_payload()
            (source / "данные 空").write_bytes(payload)
            (source / "hardlink").hardlink_to(source / "данные 空")
            (source / "small").write_bytes(b"fragment")
            (source / "empty").mkdir()
            # Read native xattr tables when the host filesystem permits their creation.
            import os
            try:
                os.setxattr(source / "small", b"user.sevenzip", b"archive fixture")
            except (OSError, AttributeError):
                pass
            for codec in ("gzip", "xz", "lzo", "lz4", "zstd"):
                image = root / "native.squashfs"
                created = subprocess.run(["mksquashfs", str(source), str(image), "-noappend",
                                          "-processors", "1", "-comp", codec, "-no-progress"],
                                         capture_output=True, timeout=15)
                self.assertEqual(created.returncode, 0, created.stderr.decode(errors="replace"))
                for engine in ("7z", "7zz"):
                    if codec == "lz4" and engine == "7zz":
                        continue
                    for name, expected in (("данные 空", payload), ("hardlink", payload),
                                           ("small", b"fragment")):
                        with self.subTest(codec=codec, engine=engine, name=name):
                            result = self.run_image(engine, image.read_bytes(), "x", name)
                            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                            self.assertEqual(result.stdout, expected)


if __name__ == "__main__":
    unittest.main()
