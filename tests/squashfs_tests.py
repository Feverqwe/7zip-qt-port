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

    def test_linux_lz4_and_modern_lzo_rle(self):
        for codec in ("lz4", "lzo-rle"):
            data, files = volume(codec=codec)
            for name, expected in files.items():
                with self.subTest(codec=codec, name=name):
                    result = self.run_image("7z", data, "x", name)
                    self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                    self.assertEqual(result.stdout, expected)

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
                    result = self.run_image(engine, volume(fault)[0])
                    self.assertNotEqual(result.returncode, 0, result.stdout.decode(errors="replace"))

    def test_xattr_tables_and_out_of_line_values(self):
        data, files = volume(xattrs=True)
        for name in ("alpha", "hardlink"):
            result = self.run_image("7z", data, "x", name)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
            self.assertEqual(result.stdout, files[name])
        for fault in ("xattr-ids-zero", "xattr-index-outside"):
            with self.subTest(fault=fault):
                result = self.run_image("7z", volume(fault, xattrs=True)[0], "l")
                self.assertNotEqual(result.returncode, 0, result.stdout.decode(errors="replace"))
        for fault in ("xattr-count-negative", "xattr-id-invalid", "xattr-ool-outside"):
            with self.subTest(fault=fault):
                data, files = volume(fault, xattrs=True)
                result = self.run_image("7z", data, "x", "alpha")
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(result.stdout, files["alpha"])
                listing = self.run_image("7z", data, "l")
                self.assertEqual(listing.returncode, 0, listing.stderr.decode(errors="replace"))
                self.assertIn(b"Read-only limitations = Some SquashFS extended attributes are unavailable", listing.stdout)

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

    def test_id_metadata_index_rejects_an_excessive_gap(self):
        result = self.run_image("7z", volume("id-metadata-gap")[0], "l")
        self.assertNotEqual(result.returncode, 0)

    def test_export_table_reference_must_match_the_inode(self):
        import struct
        data = bytearray(volume(xattrs=True, export=True)[0])
        lookup = struct.unpack_from("<Q", data, 88)[0]
        metadata = struct.unpack_from("<Q", data, lookup)[0]
        struct.pack_into("<Q", data, metadata + 2, 1)
        result = self.run_image("7z", data, "l")
        self.assertNotEqual(result.returncode, 0, result.stdout.decode(errors="replace"))

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
                self.assertEqual(result.stdout, b"")

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
