"""Inode acceptance and sparse-size boundaries for disposable ext images."""
import struct
from ext_fixture import Volume


def inode_volume(case):
    volume = Volume("extent" if case.startswith("extent-") else "direct")
    legacy = case.startswith("legacy-")
    if legacy:
        case = case.removeprefix("legacy-")
    inode = volume.file_inode
    entry = volume.root_block * volume.block_size + 24
    if case == "control":
        pass
    elif case in ("mode-zero", "mode-unknown", "special-empty", "special-payload"):
        struct.pack_into("<H", volume.data, inode,
                         0 if case == "mode-zero" else 0x3000 if case == "mode-unknown" else 0o20600)
        volume.data[entry + 7] = 0
        if case.startswith("special-"):
            volume.put32(inode + 4, 0 if case.endswith("empty") else 4)
            volume.put32(inode + 28, 0)
            volume.put32(inode + 40, 0x0103 if case.endswith("empty") else volume.table)
    elif case in ("unlinked", "orphan-unreferenced"):
        struct.pack_into("<H", volume.data, inode + 26, 0)
        if case == "orphan-unreferenced":
            volume.put32(entry, 0)
            # The unlinked slot is not a readable file even if its old data
            # pointer now refers to a block used by filesystem metadata.
            volume.put32(inode + 40, volume.table)
    elif case == "casefold":
        volume.put32(inode + 32, 0x40000000)
    elif case == "reserved-alias":
        reserved = volume.table * volume.block_size + 7 * 128
        volume.data[reserved:reserved + 128] = volume.data[inode:inode + 128]
        volume.put32(entry, 8)
    elif case == "quota-alias":
        volume.put32(1024 + 0x240, 12)
    elif case == "negative-size":
        volume.put32(inode + 108, 0x80000000)
    elif case.startswith(("bitmap-", "extent-")):
        # Format address limits; acceptance is also checked with native Linux.
        maximum = (12 + 256 + 256 ** 2 + 256 ** 3) * 1024 if case.startswith("bitmap-") else \
            (((1 << 32) - 1) >> 1) * 1024
        size = maximum + int(case.endswith("overflow"))
        volume.put32(inode + 4, size & 0xFFFFFFFF)
        volume.put32(inode + 108, size >> 32)
        volume.declared_size = size
    else:
        raise ValueError(case)
    if legacy:
        for offset in (76, 84, 92, 96, 100):
            volume.put32(1024 + offset, 0)
        struct.pack_into("<H", volume.data, 1024 + 88, 0)
        for offset in (7, 19, 31):
            volume.data[volume.root_block * volume.block_size + offset] = 0
    return volume
