"""Native ext4 xattrs plus checksum-correct EA-inode read failures."""
from pathlib import Path
import json
import struct
import urllib.parse
import zlib
from ext_modern_fixture import crc32c


class XattrVolume:
    name = "data.bin"
    size = 100

    def __init__(self, kind="xattrs"):
        root = Path(__file__).parent / "fixtures/ext4"
        self.record = json.loads((root / "manifest.json").read_text())["images"]["xattrs"]
        self.data = bytearray(zlib.decompress((root / "xattrs.ext4.zlib").read_bytes()))
        self.files = {"data.bin": bytes(range(100)), "alias.bin": bytes(range(100)),
                      "data.bin:$XATTR.user.comment": b"ordinary colon file"}
        for path, values in self.record["attributes"].items():
            for name, value in values.items():
                label = "$XATTR." + urllib.parse.quote(name, safe="._-")
                if name == "user.case" or (path == "data.bin" and name == "user.comment"):
                    label += "#1"
                self.files[(path or "[ROOT]") + ":" + label] = bytes.fromhex(value)
        self.seed = struct.unpack_from("<I", self.data, 1024 + 0x270)[0] if (
            struct.unpack_from("<I", self.data, 1024 + 96)[0] & 0x2000) else crc32c(
                0xffffffff, self.data[1024 + 104:1024 + 120])
        parent = self.record["parent_offset"]
        ea = self.record["ea_offset"]
        self.ea_data_offset = (self.u32(ea + 60) | struct.unpack_from("<H", self.data, ea + 58)[0] << 32) * 1024
        entry = self.record["entries"]["1.large"]
        def put(offset, fmt, *values):
            struct.pack_into("<" + fmt, self.data, offset, *values)
        if kind == "xattrs-signed":
            signed = self.record["entries"]["1.данные"]
            put(signed["offset"] + 12, "I", self.entry_hash("данные", self.u32(ea + 8), True))
            self.refresh_entry(signed)
        elif kind == "xattrs-lustre":
            put(ea + 8, "III", 0, 0, self.record["parent_inode"])
            put(ea + 36, "I", 0)
            put(ea + 100, "I", self.u32(parent + 100))
            self.refresh_inode(self.record["ea_inode"], ea)
        elif kind == "xattrs-bad-data":
            physical = self.u32(ea + 60) | struct.unpack_from("<H", self.data, ea + 58)[0] << 32
            self.data[physical * 1024 + 123] ^= 1
        elif kind == "xattrs-bad-hash":
            put(ea + 8, "I", self.u32(ea + 8) ^ 1)
            self.refresh_inode(self.record["ea_inode"], ea)
        elif kind == "xattrs-bad-entry-hash":
            put(entry["offset"] + 12, "I", self.u32(entry["offset"] + 12) ^ 1)
            self.refresh_entry(entry)
        elif kind == "xattrs-missing-flag":
            put(ea + 32, "I", self.u32(ea + 32) & ~0x200000)
            self.refresh_inode(self.record["ea_inode"], ea)
        elif kind in ("xattrs-no-links", "xattrs-many-links"):
            put(ea + 26, "H", 0 if kind == "xattrs-no-links" else 2)
            self.refresh_inode(self.record["ea_inode"], ea)
        elif kind == "xattrs-no-refs":
            put(ea + 12, "I", 0)
            put(ea + 36, "I", 0)
            self.refresh_inode(self.record["ea_inode"], ea)
        elif kind == "xattrs-size":
            put(ea + 4, "I", self.u32(ea + 4) - 1)
            self.refresh_inode(self.record["ea_inode"], ea)
        elif kind == "xattrs-self":
            put(entry["offset"] + 4, "I", self.record["parent_inode"])
            self.refresh_entry(entry)
        elif kind == "xattrs-unwritten":
            length = struct.unpack_from("<H", self.data, ea + 56)[0]
            put(ea + 56, "H", length | 0x8000)
            value_hash = crc32c(self.seed, bytes(entry["size"]))
            put(ea + 8, "I", value_hash)
            for key, other in self.record["entries"].items():
                if other["inode"] == self.record["ea_inode"]:
                    put(other["offset"] + 12, "I", self.entry_hash(key.split('.', 1)[1], value_hash))
                    self.refresh_entry(other)
            self.refresh_inode(self.record["ea_inode"], ea)
        elif kind == "xattrs-bad-acl":
            acl = self.record["entries"]["2."]
            value = acl["block"] * 1024 + struct.unpack_from("<H", self.data, acl["offset"] + 2)[0]
            put(value, "I", 0)
            self.refresh_entry(acl)
        elif kind != "xattrs":
            raise ValueError(kind)

    def u32(self, offset):
        return struct.unpack_from("<I", self.data, offset)[0]

    def refresh_inode(self, ino, start):
        struct.pack_into("<H", self.data, start + 124, 0)
        struct.pack_into("<H", self.data, start + 130, 0)
        seed = crc32c(crc32c(self.seed, struct.pack("<I", ino)), self.data[start + 100:start + 104])
        checksum = crc32c(seed, self.data[start:start + 256])
        struct.pack_into("<H", self.data, start + 124, checksum & 65535)
        struct.pack_into("<H", self.data, start + 130, checksum >> 16)

    def refresh_entry(self, entry):
        if not entry["block"]:
            self.refresh_inode(self.record["parent_inode"], self.record["parent_offset"])
        else:
            start = entry["block"] * 1024
            struct.pack_into("<I", self.data, start + 16, 0)
            checksum = crc32c(crc32c(self.seed, struct.pack("<Q", entry["block"])),
                              self.data[start:start + 1024])
            struct.pack_into("<I", self.data, start + 16, checksum)

    @staticmethod
    def entry_hash(name, value_hash, signed=False):
        result = 0
        for byte in name.encode():
            if signed and byte >= 128:
                byte -= 256
            result = ((result << 5) ^ (result >> 27) ^ byte) & 0xffffffff
        return ((result << 16) ^ (result >> 16) ^ value_hash) & 0xffffffff

    def read(self, offset, length):
        return bytes(range(100))[offset:offset + length]
