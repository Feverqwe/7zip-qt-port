"""HFS embedded-volume wrapper controls against Linux's MDB decoder."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import struct


def wrap(image, allocation=4096):
    count = (len(image) + allocation - 1) // allocation
    embedded = bytearray(image.ljust(count * allocation, b"\0"))
    embedded[-1024:-512] = image[-1024:-512]
    prefix = bytearray(2048 + allocation)
    prefix[1024:1026] = b"BD"
    struct.pack_into(">H", prefix, 1034, 0x8200)
    struct.pack_into(">H", prefix, 1042, count + 1)
    struct.pack_into(">I", prefix, 1044, allocation)
    struct.pack_into(">H", prefix, 1052, 4)
    prefix[1148:1150] = b"HX" if image[1024:1026] == b"HX" else b"H+"
    struct.pack_into(">HH", prefix, 1150, 1, count)
    return prefix + embedded + bytes(1024)


def volume(kind):
    from tests.formats.hfs.support.hfs_fixture import volume as base_volume
    image, files = base_volume("hfsx" if "hfsx" in kind else "base")
    if kind.startswith("wrapper-version-"):
        image = bytearray(image)
        version = {"wrapper-version-hplus5": 5, "wrapper-version-hfsx4": 4,
                   "wrapper-version-old": 3, "wrapper-version-new": 6}[kind]
        struct.pack_into(">H", image, 1026, version)
        return bytes(image), files
    if kind == "wrapper-non-power":
        image = wrap(image, 1536)
    else:
        image = wrap(image)
    if kind.startswith("wrapper-nested"):
        image = wrap(image, 512)
        if kind == "wrapper-nested-short":
            count = struct.unpack_from(">H", image, 1152)[0]
            struct.pack_into(">H", image, 1152, count - 8)
    elif kind == "wrapper-no-slock":
        struct.pack_into(">H", image, 1034, 0x0200)
    elif kind == "wrapper-no-spared":
        struct.pack_into(">H", image, 1034, 0x8000)
    elif kind == "wrapper-zero-step":
        struct.pack_into(">H", image, 1052, 0)
        struct.pack_into(">H", image, 1150, 0)
    elif kind == "wrapper-backup-signature":
        image[-2048:-2046] = b"HX"
    elif kind == "wrapper-backup-version":
        struct.pack_into(">H", image, len(image) - 2046, 99)
    elif kind not in ("wrapper-non-power", "wrapper-nested", "wrapper-nested-hfsx"):
        raise ValueError(kind)
    return bytes(image), files
