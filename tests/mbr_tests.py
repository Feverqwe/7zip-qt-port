"""MBR/EBR partition geometry and byte-accurate reads via both engines."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from mbr_fixture import CASES, BAD, volume
from mbr_subtypes_fixture import CASES as SUBTYPES, volume as subtype_volume

RUNTIME = Path(sys.argv.pop(1)).resolve()
MODULE = Path(sys.argv.pop(1)).resolve()
SUFFIX = ".exe" if sys.platform == "win32" else ""


class MbrTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="sevenzip-mbr-runtime-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.runtime = Path(cls.temporary.name)
        for name in ("7z" + SUFFIX, "7zz" + SUFFIX,
                     "7z.dll" if sys.platform == "win32" else "7z.so"):
            shutil.copy2(RUNTIME / name, cls.runtime / name)
        (cls.runtime / "Formats").mkdir()
        shutil.copy2(MODULE, cls.runtime / "Formats" / MODULE.name)

    def run_engine(self, engine, args):
        return subprocess.run([str(self.runtime / (engine + SUFFIX)), *args],
                              capture_output=True, timeout=15)

    def test_readable_partitions(self):
        variants = [(kind, volume(kind), False, ("7z", "7zz")) for kind in CASES]
        variants += [(kind, subtype_volume(kind)[:2], True, ("7z",)) for kind in SUBTYPES]
        for kind, (image, expected), nested, engines in variants:
            with tempfile.TemporaryDirectory(prefix="sevenzip-mbr-") as temporary:
                path = Path(temporary) / "разделы 空.bin"
                path.write_bytes(image)
                for engine in engines:
                    with self.subTest(kind=kind, engine=engine):
                        result = self.run_engine(engine, ["l", "-t*", "-slt", "-sccUTF-8", "--", str(path)])
                        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                        self.assertIn(b"Type = MBR", result.stdout)
                        records = [dict(line.split(" = ", 1) for line in section.splitlines()
                                        if " = " in line)
                                   for section in result.stdout.decode("utf-8").replace("\r\n", "\n").split("\n\n")]
                        real = [record for record in records
                                if record.get("Primary") in (("-",) if nested else ("+", "-"))]
                        self.assertEqual([(int(r["Offset"]), int(r["Size"])) for r in real],
                                         [(p["offset"], p["size"]) for p in expected])
                        self.assertEqual(path.read_bytes(), image)
                        if not real:
                            continue
                        selected = sorted({0, len(real) // 2, len(real) - 1})
                        result = self.run_engine(engine, ["x", "-tMBR", "-so", "-bso0", "-bse2",
                            "--", str(path), *[real[i]["Path"] for i in selected]])
                        self.assertEqual(result.returncode, 0, result.stderr)
                        self.assertEqual(result.stdout, b"".join(bytes.fromhex(expected[i]["payload"])
                                                                for i in selected))
                        self.assertEqual(path.read_bytes(), image)

    def test_malformed_primary(self):
        for kind in BAD:
            image, _ = volume(kind)
            with tempfile.TemporaryDirectory(prefix="sevenzip-mbr-") as temporary:
                path = Path(temporary) / "bad.mbr"
                path.write_bytes(image)
                for engine in ("7z", "7zz"):
                    with self.subTest(kind=kind, engine=engine):
                        result = self.run_engine(engine, ["l", "-tMBR", "--", str(path)])
                        self.assertEqual(result.returncode, 2)


if __name__ == "__main__":
    unittest.main()
