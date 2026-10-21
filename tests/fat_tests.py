"""Production FAT name recovery, geometry and fragmented file reads."""
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from fat_fixture import Volume, PAYLOAD, LONG_NAME, entry, long_entries, legacy_volume, readonly_volume

RUNTIME = Path(sys.argv.pop(1))
MODULE = Path(sys.argv.pop(1))
SUFFIX = ".exe" if sys.platform == "win32" else ""


class FatTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-fat-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        core = "7z.dll" if sys.platform == "win32" else "7z.so"
        for name in ("7z" + SUFFIX, "7zz" + SUFFIX, core):
            shutil.copy2(RUNTIME / name, cls.runtime / name)
        (cls.runtime / "Formats").mkdir()
        shutil.copy2(MODULE, cls.runtime / "Formats" / MODULE.name)

    def run_image(self, volume, engine, command="x", name=LONG_NAME, forced=True):
        with tempfile.TemporaryDirectory(dir=self.runtime) as temporary:
            path = Path(temporary) / "образ FAT 空.img"
            path.write_bytes(volume.data)
            args = [str(self.runtime / (engine + SUFFIX)), command, "-sccUTF-8", "-bsp0"]
            if forced:
                args += ["-tFAT"]
            args += ["-so", "-bso0", "-bse2"] if command == "x" else ["-slt"]
            args += ["--", str(path)]
            if name is not None:
                args += [name]
            result = subprocess.run(args, stdin=subprocess.DEVNULL, capture_output=True, timeout=15)
            self.assertEqual(path.read_bytes(), volume.data)
            return result

    def assert_contents(self, volume, engine, name=LONG_NAME, expected=PAYLOAD):
        result = self.run_image(volume, engine, name=name)
        self.assertIn(result.returncode, (0, 1), result.stderr.decode(errors="replace"))
        self.assertEqual(result.stdout, expected)

    def test_fragmented_variants_and_sector_boundaries(self):
        for engine in ("7z", "7zz"):
            for bits, sector, spc in ((12, 512, 1), (16, 512, 1), (32, 512, 1),
                                      (16, 4096, 1), (32, 4096, 128)):
                with self.subTest(engine=engine, bits=bits, sector=sector, spc=spc):
                    volume = Volume(bits, sector, spc)
                    self.assert_contents(volume, engine)
                    self.assert_contents(volume, engine, "EMPTY.TXT", b"")
                    listing = self.run_image(volume, engine, "l", None)
                    self.assertIn(listing.returncode, (0, 1), listing.stderr)
                    self.assertIn("File System = FAT" + str(bits), listing.stdout.decode("utf-8"))

    def test_inode_size_stops_before_unused_chain_tail(self):
        for engine in ("7z", "7zz"):
            for bits in (12, 16, 32):
                with self.subTest(engine=engine, bits=bits):
                    volume = Volume(bits)
                    volume.set_fat(volume.chain[-1], 19)
                    volume.set_fat(19, volume.eof)
                    # Empty files can retain an allocation too.
                    volume.records[-1] = entry(b"EMPTY   TXT", 20)
                    volume.set_fat(20, volume.eof)
                    volume.write_directory()
                    self.assert_contents(volume, engine)
                    self.assert_contents(volume, engine, "EMPTY.TXT", b"")

    def test_flagged_and_short_sources_keep_readable_files(self):
        for bits in (12, 16, 32):
            for mode in ("dirty", "short", "combined"):
                with self.subTest(bits=bits, mode=mode):
                    volume = readonly_volume(mode + str(bits))
                    self.assert_contents(volume, "7z")
                    self.assert_contents(volume, "7z", "EMPTY.TXT", b"")
                    listing = self.run_image(volume, "7z", "l", None)
                    self.assertIn(listing.returncode, (0, 1), listing.stderr)
                    if mode in ("dirty", "combined"):
                        self.assertIn(b"not properly unmounted", listing.stdout)
                    if mode in ("short", "combined"):
                        self.assertIn(b"shorter than", listing.stdout)

    def test_active_fat_and_invalid_index_fallback(self):
        for engine in ("7z", "7zz"):
            for active in (1, 2, 15):
                with self.subTest(engine=engine, active=active):
                    volume = Volume(32)
                    struct.pack_into("<H", volume.data, 40, 0x80 | active)
                    if active == 1:
                        # A valid second table must retain priority over the first.
                        volume.set_fat(volume.chain[0], 0, copy=0)
                    self.assert_contents(volume, engine)

    def test_linux_legacy_and_signatureless_auto_recognition(self):
        for kib in (160, 180, 320, 360):
            with self.subTest(kib=kib):
                v = legacy_volume(kib)
                result = self.run_image(v, "7z", name="LEGACY.BIN", forced=False)
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(result.stdout, PAYLOAD)
        for bits in (12, 16, 32):
            with self.subTest(bits=bits):
                v = Volume(bits)
                v.data[:3] = bytes(3)
                v.data[510:512] = bytes(2)
                result = self.run_image(v, "7z", forced=False)
                self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
                self.assertEqual(result.stdout, PAYLOAD)

    def test_legacy_fallback_rejects_unrelated_raw_images(self):
        for fault in ("bootstrap", "nonzero-bpb", "size", "truncated-fat"):
            with self.subTest(fault=fault):
                v = legacy_volume()
                if fault == "bootstrap":
                    v.data[:3] = bytes(3)
                elif fault == "nonzero-bpb":
                    v.data[13] = 1
                elif fault == "size":
                    v.data += bytes(512)
                else:
                    del v.data[600:]
                result = self.run_image(v, "7z", name="LEGACY.BIN")
                self.assertIn(result.returncode, (1, 2), result.stderr)
                self.assertEqual(result.stdout, b"")

    def test_linux_unicode_padding_and_short_codepage(self):
        for name, expected in (("снимок 😀.bin", "снимок 😀.bin"),
                               ("A\ud800B.bin", "AB.bin"), ("A\udc00B.bin", "AB.bin")):
            with self.subTest(name=ascii(name)):
                v = Volume()
                v.records = long_entries(name, v.alias) + [entry(v.alias, v.chain[0], len(PAYLOAD))]
                v.write_directory()
                self.assert_contents(v, "7z", expected)
        v = Volume()
        # Data after the first UTF-16 NUL is not part of the name.
        tail = v.records[0]
        tail[28:32] = b"X\0Y\0"
        v.write_directory()
        self.assert_contents(v, "7z")
        for alias, flags, expected in ((b"\x8eBC     TXT", 0x18, "äbc.txt"),
                                       (b"ABC\0    TXT", 0, "ABC.TXT")):
            with self.subTest(alias=alias):
                v = Volume()
                rec = entry(alias, v.chain[0], len(PAYLOAD))
                rec[12] = flags
                v.records = [rec]
                v.write_directory()
                self.assert_contents(v, "7z", expected)

    def test_linux_date_normalization(self):
        for date, expected in ((0, "1980-01-01"), ((120 << 9) | (2 << 5) | 29, "2100-03-01")):
            with self.subTest(date=date):
                v = Volume()
                struct.pack_into("<H", v.records[2], 24, date)
                v.write_directory()
                result = self.run_image(v, "7z", "l", None)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn("Modified = " + expected, result.stdout.decode("utf-8"))

    def test_free_space_recounts_fat_and_excludes_reserved_entries(self):
        for engine in ("7z", "7zz"):
            for bits in (12, 16, 32):
                with self.subTest(engine=engine, bits=bits):
                    v = Volume(bits)
                    v.set_fat(0, 0)
                    v.set_fat(1, 0)
                    if bits == 32:
                        # A valid but stale FSINFO count must not override the FAT.
                        struct.pack_into("<I", v.data, v.sector + 488, 1)
                        struct.pack_into("<H", v.data, 48, 0)
                    result = self.run_image(v, engine, "l", None)
                    self.assertIn(result.returncode, (0, 1), result.stderr)
                    free = (v.clusters - len(v.chain) - int(bits == 32)) * v.cluster_size
                    self.assertIn("Free Space = " + str(free), result.stdout.decode("utf-8"))

    def test_bpb_geometry_does_not_require_bootstrap_code(self):
        for engine in ("7z", "7zz"):
            for bits in (12, 16, 32):
                with self.subTest(engine=engine, bits=bits):
                    volume = Volume(bits, fats=5)
                    volume.data[:3] = bytes(3)
                    if bits == 32:
                        # Linux derives FAT32 from FAT lengths, not root entries.
                        struct.pack_into("<H", volume.data, 17, 16)
                        volume.data[52:64] = b"reserved1234"
                        struct.pack_into("<H", volume.data, 42, 1)
                        # A root sector now precedes the data area.
                        volume.data[volume.data_sector * 512:volume.data_sector * 512] = bytes(512)
                        struct.pack_into("<H", volume.data, 19, volume.sectors + 1)
                    volume.set_fat(0, 0)
                    self.assert_contents(volume, engine)

    def test_lfn_reserved_fields_and_escaped_alias_checksum(self):
        for engine in ("7z", "7zz"):
            volume = Volume()
            alias = b"\x05ATA~1  BIN"
            volume.records = long_entries(LONG_NAME, alias) + [entry(alias, volume.chain[0], len(PAYLOAD))]
            for record in volume.records[:-1]:
                record[12] = 7
                struct.pack_into("<H", record, 26, 123)
            volume.write_directory()
            self.assert_contents(volume, engine)

    def test_lfn_damage_resynchronizes_without_hiding_files(self):
        for engine in ("7z", "7zz"):
            for fault in ("invalid-id", "orphan", "restart", "checksum", "bad-order", "volume"):
                with self.subTest(engine=engine, fault=fault):
                    volume = Volume()
                    name = LONG_NAME
                    if fault == "invalid-id":
                        bad = bytearray(volume.records[0])
                        bad[0] = 0x40
                        volume.records.insert(0, bad)
                    elif fault == "orphan":
                        volume.records.insert(0, bytearray(volume.records[1]))
                    elif fault == "restart":
                        volume.records.insert(0, bytearray(volume.records[0]))
                    elif fault == "checksum":
                        volume.records[0][13] ^= 1
                        name = "DATA~1.BIN"
                    elif fault == "bad-order":
                        volume.records[1][0] = 3
                        name = "DATA~1.BIN"
                    elif fault == "volume":
                        volume.records.insert(0, entry(b"STALELABEL ", attr=0x18))
                    # Force the LFN sequence across a root-directory sector.
                    deleted = bytearray(32)
                    deleted[0] = 0xE5
                    volume.records = [deleted] * 15 + volume.records
                    volume.write_directory()
                    self.assert_contents(volume, engine, name)

    def test_invalid_geometry_and_short_chains_report_errors(self):
        for engine in ("7z", "7zz"):
            for fault in ("zero-fats", "metadata-overflow", "invalid-sector", "free-chain", "early-eof",
                          "cycle", "oversized-file", "fat-truncated", "truncated"):
                with self.subTest(engine=engine, fault=fault):
                    volume = Volume(32)
                    if fault == "zero-fats":
                        volume.data[16] = 0
                    elif fault == "metadata-overflow":
                        struct.pack_into("<I", volume.data, 36, 0x80000000)
                    elif fault == "invalid-sector":
                        struct.pack_into("<H", volume.data, 11, 768)
                    elif fault == "free-chain":
                        volume.set_fat(volume.chain[0], 0)
                    elif fault == "early-eof":
                        volume.set_fat(volume.chain[0], volume.eof)
                    elif fault == "cycle":
                        volume.set_fat(volume.chain[0], volume.chain[0])
                    elif fault == "oversized-file":
                        struct.pack_into("<I", volume.data, volume.root + 32 * 2 + 28, 0xFFFFFFFF)
                    elif fault == "fat-truncated":
                        del volume.data[volume.reserved * volume.sector + 100:]
                    else:
                        del volume.data[volume.offset(volume.chain[0]) + 100:]
                    result = self.run_image(volume, engine)
                    self.assertIn(result.returncode, (1, 2), result.stderr.decode(errors="replace"))


if __name__ == "__main__":
    unittest.main()
