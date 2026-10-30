"""Bounded bootstrap copies and independent readable MFT segments."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
from tests.formats.ntfs.support.ntfs_fixture import attributes, put, record, save_record, runs, u32, u64

CASES = ("recovery-boot-io", "recovery-primary-address", "recovery-mirror-address",
         "recovery-both-addresses", "recovery-mirror-header", "recovery-mirror-signature", "recovery-mirror-data",
         "recovery-mirror-io", "recovery-primary-control", "recovery-both-bad",
         "recovery-mirror-bounds", "recovery-mirror-named", "recovery-list-partial",
         "recovery-list-io", "recovery-list-segment", "recovery-attribute-tail",
         "recovery-record-tail", "recovery-inactive-bootstrap", "recovery-only-inactive")
PARTIAL = ("recovery-list-partial", "recovery-list-io", "recovery-list-segment")
FATAL = ("recovery-both-bad", "recovery-both-addresses", "recovery-mirror-bounds", "recovery-mirror-named",
         "recovery-only-inactive")


def volume(kind):
    from tests.formats.ntfs.support.ntfs_fixture import volume as base
    from tests.formats.ntfs.support.ntfs_ea_fixture import resident, set_attributes
    from tests.formats.ntfs.support.ntfs_mft_fixture import volume as fragmented
    if kind not in CASES:
        raise ValueError(kind)
    layout = {"recovery-list-partial": "mft-list-bad-list",
              "recovery-list-io": "mft-list-nonresident",
              "recovery-list-segment": "mft-list-bad-id"}.get(kind)
    source, files = fragmented(layout) if layout else base()
    image = bytearray(source)
    primary = u64(image, 48) * 4096
    mirror = u64(image, 56) * 4096
    offset, rec = record(image, 0)
    if kind in ("recovery-primary-address", "recovery-mirror-address", "recovery-both-addresses"):
        invalid = u64(image, 40) // 8 + 1
        for boot in (0, len(image) - 512):
            if kind != "recovery-mirror-address":
                put(image, boot + 48, "Q", invalid)
            if kind != "recovery-primary-address":
                put(image, boot + 56, "Q", invalid)
    if kind in PARTIAL or kind in ("recovery-primary-control", "recovery-attribute-tail",
                                   "recovery-record-tail", "recovery-inactive-bootstrap", "recovery-only-inactive"):
        put(image, mirror, "I", 0)
    if kind in PARTIAL:
        # Put all three creator files inside an explicitly mapped first
        # segment, leaving the remaining MFT records unavailable.
        attrs = [a for _, a in attributes(rec)]
        data = next(a for a in attrs if u32(a, 0) == 0x80)
        mapping = runs([(17, u64(image, 48))])
        data[64:] = mapping + bytes(len(data) - 64 - len(mapping))
        put(data, 24, "Q", 16)
        put(data, 48, "QQ", u64(data, 40), u64(data, 40))
        set_attributes(image, offset, rec, attrs)
    if kind in ("recovery-mirror-header", "recovery-mirror-bounds", "recovery-mirror-named"):
        put(rec, 28, "I", 512)
        save_record(image, offset, rec)
    elif kind in ("recovery-mirror-signature", "recovery-both-bad"):
        put(image, primary, "I", 0)
        if kind == "recovery-both-bad":
            put(image, mirror, "I", 0)
    elif kind == "recovery-mirror-data":
        data = next(a for _, a in attributes(rec) if u32(a, 0) == 0x80)
        # Keep the attribute framing valid, but the mapping cannot be decoded.
        data[64:] = bytes(len(data) - 64)
        attrs = [data if u32(a, 0) == 0x80 else a for _, a in attributes(rec)]
        set_attributes(image, offset, rec, attrs)
    elif kind == "recovery-attribute-tail":
        attrs = [a for _, a in attributes(rec)]
        bad = resident(0xd0, bytes(8), 50)
        put(bad, 4, "I", 0)
        set_attributes(image, offset, rec, attrs + [bad])
    elif kind == "recovery-record-tail":
        attrs = [a for _, a in attributes(rec)]
        data = next(a for a in attrs if u32(a, 0) == 0x80)
        put(data, 48, "Q", u64(data, 48) - 1)
        put(data, 56, "Q", u64(data, 56) - 1)
        set_attributes(image, offset, rec, attrs)
        files.pop("gamma")
    elif kind == "recovery-inactive-bootstrap":
        put(rec, 22, "H", 0)
        save_record(image, offset, rec)
    elif kind == "recovery-only-inactive":
        for number in range(67):
            if image[primary + number * 1024:primary + number * 1024 + 4] == b"FILE":
                start, inactive = record(image, number)
                put(inactive, 22, "H", int.from_bytes(inactive[22:24], "little") & ~1)
                save_record(image, start, inactive)
    if kind in ("recovery-mirror-bounds", "recovery-mirror-named"):
        # Decode the existing mirror frame with the creator's normal fixup helper.
        relocated = bytearray(image)
        put(relocated, 48, "Q", u64(image, 56))
        _, copy = record(relocated, 0)
        attrs = [a for _, a in attributes(copy)]
        data = next(a for a in attrs if u32(a, 0) == 0x80)
        if kind == "recovery-mirror-bounds":
            data[64:] = bytes(len(data) - 64)
        else:
            from tests.formats.ntfs.support.ntfs_security_fixture import named_nonresident
            named = named_nonresident(data, 0x80, "unrelated",
                                       [(u64(data, 40) // 4096, u64(image, 48))],
                                       u64(data, 48), 1)
            attrs[attrs.index(data)] = named
        set_attributes(image, mirror, copy, attrs)
    return bytes(image), files
