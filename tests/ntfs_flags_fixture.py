"""Stale standard-information flags versus the selected NTFS DATA stream."""
from ntfs_fixture import attributes, put, record, u16, u32, u64
from ntfs_ea_fixture import set_attributes
from ntfs_security_fixture import named_nonresident

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
}


def properties(kind):
    _, path, _, _, value = CASES[kind]
    return {"path": path, "value": value}


def volume(kind):
    from ntfs_fixture import volume as base
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
    set_attributes(image, offset, rec, attrs)
    return bytes(image), files
