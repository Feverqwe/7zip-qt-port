"""Deleted MFT recovery isolated from the live namespace."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
from tests.formats.ntfs.support.ntfs_fixture import attributes, nonresident, put, record, u16, u32, u64
from tests.formats.ntfs.support.ntfs_ea_fixture import resident, set_attributes
from tests.formats.ntfs.support.ntfs_security_fixture import named_resident
from tests.formats.ntfs.support.ntfs_file_metadata_fixture import root_names

CASES = ("deleted-data", "deleted-empty", "deleted-no-data", "deleted-link",
         "deleted-extension", "deleted-wof", "deleted-reparse-nonresident",
         "deleted-reparse-bad-run", "deleted-reparse-too-large", "deleted-reparse-empty",
         "deleted-reparse-missing", "deleted-link-invalid", "deleted-orphan-extension")
UNREADABLE = ("deleted-no-data", "deleted-reparse-bad-run", "deleted-reparse-too-large",
              "deleted-reparse-empty", "deleted-reparse-missing", "deleted-link-invalid")
SIDE = b"independent deleted ADS"


def volume(kind):
    from tests.formats.ntfs.support.ntfs_fixture import volume as base
    if kind not in CASES:
        raise ValueError(kind)
    original = {"deleted-link": "link-symlink", "deleted-extension": "link-extension",
                "deleted-wof": "wof-xpress4k", "deleted-reparse-nonresident": "link-nonresident",
                "deleted-reparse-bad-run": "link-nonresident", "deleted-reparse-too-large": "link-nonresident",
                "deleted-reparse-empty": "link-symlink", "deleted-reparse-missing": "link-symlink",
                "deleted-link-invalid": "link-bad-offset"}.get(kind, "base")
    source, files = base(original)
    image = bytearray(source)
    offset, rec = record(image, 66)
    attrs = [a for _, a in attributes(rec)]
    if kind == "deleted-orphan-extension":
        offset, orphan = record(image, 16)
        orphan[:u16(rec, 20)] = rec[:u16(rec, 20)]
        put(orphan, 22, "H", 0)
        put(orphan, 32, "Q", 9999 | (1 << 48))
        put(orphan, 44, "I", 16)
        set_attributes(image, offset, orphan, [resident(0xc0, b"unreachable metadata", 9)])
        return bytes(image), files, {}
    recovered = {"gamma": files.get("gamma", b"")}
    if kind in UNREADABLE:
        recovered.pop("gamma")
    if kind == "deleted-empty":
        attrs = [a for a in attrs if u32(a, 0) != 0x80]
        attrs.append(resident(0x80, b"", 4))
        recovered["gamma"] = b""
    elif kind == "deleted-no-data":
        attrs = [a for a in attrs if u32(a, 0) != 0x80]
    elif kind in ("deleted-reparse-bad-run", "deleted-reparse-too-large"):
        rp = next(a for a in attrs if u32(a, 0) == 0xc0)
        size = 16385 if kind.endswith("large") else u64(rp, 48)
        replacement = nonresident(rp, [(5 if size > 4096 else 1, len(image) // 4096 + 1)], size)
        attrs[attrs.index(rp)] = replacement
    elif kind == "deleted-reparse-empty":
        attrs = [a for a in attrs if u32(a, 0) != 0xc0] + [resident(0xc0, b"", 5)]
    elif kind == "deleted-reparse-missing":
        attrs = [a for a in attrs if u32(a, 0) != 0xc0]
    attrs.append(named_resident(0x80, "side", SIDE, 90))
    recovered["gamma:side"] = SIDE
    files = {name: value for name, value in files.items() if not name.startswith("gamma")}
    put(rec, 22, "H", u16(rec, 22) & ~1)
    set_attributes(image, offset, rec, sorted(attrs, key=lambda a: u32(a, 0)))
    if kind == "deleted-extension":
        offset, extension = record(image, 16)
        put(extension, 22, "H", u16(extension, 22) & ~1)
        set_attributes(image, offset, extension, [a for _, a in attributes(extension)])
    root_names(image, [])
    return bytes(image), files, recovered
