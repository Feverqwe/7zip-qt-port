"""Stale standard-information flags versus the selected NTFS DATA stream."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import struct
from tests.formats.ntfs.support.ntfs_fixture import attributes, nonresident, put, record, u16, u32, u64
from tests.formats.ntfs.support.ntfs_ea_fixture import allocated, resident, set_attributes
from tests.formats.ntfs.support.ntfs_security_fixture import named_nonresident, named_resident

CASES = {
    "flags-clear": (64, "alpha", "base", 0x4a23, 0x23),
    "flags-sparse": (64, "alpha", "sparse", 0x4823, 0x223),
    "flags-compressed": (64, "alpha", "compressed", 0x4223, 0x823),
    "flags-encrypted": (64, "alpha", "encrypted", 0xa23, 0x4023),
    "flags-resident": (65, "данные 空", "base", 0x4a23, 0x23),
    "flags-ads": (64, "alpha", "base", 0x23, 0x23),
    "flags-directory": (11, "[SYSTEM]/$Extend", "base", 0x4a23, 0x4a33),
    "flags-badclus": (8, "[SYSTEM]/$BadClus", "base", 0x23, 0x223),
    "flags-secure": (9, "[SYSTEM]/$Secure", "security-sparse-sds", 0x23, 0x223),
    "flags-extension-sparse": (64, "alpha", "sparse", 0x23, 0x223),
    "flags-extension-plain": (64, "alpha", "base", 0x4a23, 0x23),
    "flags-ads-sparse": (64, "alpha:other", "base", 0x23, 0x223),
    "flags-ads-resident": (64, "alpha:other", "sparse", 0x23, 0x23),
    "flags-ads-encrypted": (64, "alpha:other", "base", 0x23, 0x4023),
}


def properties(kind):
    _, path, _, _, value = CASES[kind]
    result = {"path": path, "value": value}
    if kind.startswith("flags-ads-"):
        result["base_value"] = 0x223 if kind == "flags-ads-resident" else 0x23
    return result


def volume(kind):
    from tests.formats.ntfs.support.ntfs_fixture import volume as base
    number, _, core, stale, _ = CASES[kind]
    original, files = base(core)
    image = bytearray(original)
    offset, rec = record(image, number)
    attrs = [a for _, a in attributes(rec)]
    std = next(a for a in attrs if u32(a, 0) == 0x10)
    put(std, u16(std, 20) + 32, "I", stale)
    if kind in ("flags-ads", "flags-directory"):
        # A valid all-hole named stream must not alter the ordinary file's flags.
        _, source = record(image, 64)
        template = next(a for _, a in attributes(source) if u32(a, 0) == 0x80)
        attrs.append(named_nonresident(template, 0x80, "other", [(1, None)], 4096,
                                       30, flags=0x8000))
        attrs.sort(key=lambda a: u32(a, 0))
    elif kind == "flags-badclus":
        bad = next(a for a in attrs if u32(a, 0) == 0x80 and a[9])
        size = u64(bad, 48)
        attrs[attrs.index(bad)] = named_nonresident(bad, 0x80, "$Bad",
                                                   [(size // 4096, None)], size,
                                                   u16(bad, 14), flags=0x8000)
    elif kind.startswith("flags-extension-"):
        data = next(a for a in attrs if u32(a, 0) == 0x80)
        mapping = u16(data, 32)
        lcn = int.from_bytes(data[mapping + 2:mapping + 4], "little")
        sparse = kind.endswith("sparse")
        primary = nonresident(data, [(1, lcn), (15, None)] if sparse else [(16, lcn)],
                              32 * 4096, flags=0x8000 if sparse else 0)
        put(primary, 40, "Q", 32 * 4096)
        if sparse:
            put(primary, 64, "Q", 2 * 4096)
        continuation = nonresident(data, [(15, None), (1, lcn + 31)] if sparse else [(16, lcn + 16)], 0)
        put(continuation, 12, "H", 0 if sparse else 0x4000)
        put(continuation, 16, "QQ", 16, 31)
        put(continuation, 40, "QQQ", 0, 0, 0)
        attrs[attrs.index(data)] = primary
        extension = bytearray(rec)
        put(extension, 16, "H", 2)
        put(extension, 32, "Q", number | (u16(rec, 16) << 48))
        put(extension, 44, "I", 16)
        set_attributes(image, u64(image, 0x30) * 4096 + 16 * 1024, extension, [continuation])
        rows = [(u32(a, 0), 0, number | (u16(rec, 16) << 48), u16(a, 14)) for a in attrs]
        rows.append((0x80, 16, 16 | (2 << 48), u16(continuation, 14)))
        rows.sort(key=lambda row: (row[0], row[1]))
        content = b"".join(struct.pack("<IHBBQQH6x", typ, 32, 0, 26, vcn, ref, ident)
                           for typ, vcn, ref, ident in rows)
        attrs.insert(1, resident(0x20, content, 31))
        allocated(image, 0, 0xb0, [16])
    elif kind.startswith("flags-ads-"):
        if kind == "flags-ads-resident":
            payload = b"independent resident ADS"
            ads = named_resident(0x80, "other", payload, 30)
            files["alpha:other"] = payload
        else:
            _, source = record(image, 64)
            template = next(a for _, a in attributes(source) if u32(a, 0) == 0x80)
            sparse = kind == "flags-ads-sparse"
            ads = named_nonresident(template, 0x80, "other", [(1, None if sparse else 1000)],
                                    4096, 30, flags=0x8000 if sparse else 0x4000)
            if kind == "flags-ads-sparse":
                files["alpha:other"] = bytes(4096)
            else:
                allocated(image, 6, 0x80, [1000])
        attrs.append(ads)
    set_attributes(image, offset, rec, attrs)
    return bytes(image), files
