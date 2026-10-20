"""Disposable allocation variants of a native genisoimage UDF volume."""
import binascii
import json
from pathlib import Path
import struct
import sys
import zlib

CASES = ("native", "sparse-short", "sparse-long", "hole-only", "allocated-hole",
         "eof-short", "eof-long", "unused-tail", "inline", "directory-padded",
         "directory-inline", "metadata-bootstrap", "implicit-tail", "implicit-hole",
         "eof-partition-tail", "terminator-short", "terminator-long")
CONTINUATIONS = ("ad-short", "ad-long", "ad-chain-short", "ad-chain-long",
                 "ad-limit-16", "ad-directory", "ad-cross-partition", "ad-zero",
                 "ad-implicit-tail")


def volume(kind):
    if kind.startswith("fi-"):
        from udf_directory_fixture import volume as directory_volume
        return directory_volume(kind)
    if kind.startswith("ia-"):
        from udf_inode_fixture import volume as inode_volume
        return inode_volume(kind)
    if kind.startswith("ns-"):
        from udf_names_fixture import volume as names_volume
        return names_volume(kind)
    if kind.startswith("ds-"):
        from udf_discovery_fixture import volume as discovery_volume
        return discovery_volume(kind)
    if kind.startswith("walk-"):
        from udf_walk_fixture import volume as walk_volume
        return walk_volume(kind)
    if kind.startswith("md-"):
        from udf_metadata_fixture import volume as metadata_volume
        return metadata_volume(kind)
    if kind.startswith("vat"):
        from udf_vat_fixture import volume as vat_volume
        return vat_volume(kind)
    if kind.startswith("sp-"):
        from udf_sparing_fixture import volume as sparing_volume
        return sparing_volume(kind)
    if kind.startswith("icb-"):
        from udf_icb_fixture import volume as icb_volume
        return icb_volume(kind)
    image = bytearray(zlib.decompress(
        (Path(__file__).parent / "fixtures/udf/genisoimage.udf.zlib").read_bytes()))
    files = {"alpha.bin": bytes(range(256)) * 32, "empty": b"",
             "nested/tail.bin": b"directory payload"}
    block = 2048
    entry = 263 * block

    def u32(pos):
        return struct.unpack_from("<I", image, pos)[0]

    def tag(pos, length):
        struct.pack_into("<HH", image, pos + 8,
                         binascii.crc_hqx(image[pos + 16:pos + length], 0), length - 16)
        image[pos + 4] = 0
        image[pos + 4] = sum(image[pos:pos + 16]) & 255

    def allocation(extents, long):
        return b"".join(struct.pack("<IIH6x", length | (type << 30), location, partition)
                        if long else struct.pack("<II", length | (type << 30), location)
                        for type, length, location, partition in extents)
    def descriptors(pos, extents, long=False):
        flags = struct.unpack_from("<H", image, pos + 34)[0]
        struct.pack_into("<H", image, pos + 34, (flags & ~3) | int(long))
        data = allocation(extents, long)
        struct.pack_into("<I", image, pos + 172, len(data))
        image[pos + 176:pos + block] = data + bytes(block - 176 - len(data))
        tag(pos, 176 + len(data))

    if kind.startswith("ad-"):
        for sector in (34, 50):
            pos = sector * block
            struct.pack_into("<I", image, pos + 192, 100)
            tag(pos, 16 + struct.unpack_from("<H", image, pos + 10)[0])
        long = kind.endswith("long") or kind in ("ad-cross-partition", "ad-bad-partition")
        def aed(logical, extents):
            pos = ((360 if kind == "ad-cross-partition" else 257) + logical) * block
            data = allocation(extents, long)
            image[pos:pos + block] = bytes(block)
            struct.pack_into("<HH", image, pos, 258, 2)
            struct.pack_into("<I", image, pos + 12, logical)
            struct.pack_into("<I", image, pos + 20, len(data))
            image[pos + 24:pos + 24 + len(data)] = data
            tag(pos, 24 + len(data))
        if kind == "ad-cross-partition":
            image[360 * block:381 * block] = image[257 * block:278 * block]
            for source, target in ((34, 36), (50, 52)):
                pos = target * block
                image[pos:pos + block] = image[source * block:(source + 1) * block]
                struct.pack_into("<I", image, pos + 12, target)
                struct.pack_into("<H", image, pos + 22, 1)
                struct.pack_into("<II", image, pos + 188, 360, 60)
                tag(pos, 16 + struct.unpack_from("<H", image, pos + 10)[0])
            for sector in (35, 51):
                pos = sector * block
                image[pos + 446:pos + 452] = image[pos + 440:pos + 446]
                struct.pack_into("<H", image, pos + 450, 1)
                struct.pack_into("<II", image, pos + 264, 12, 2)
                tag(pos, 452)
        partition = 1 if kind == "ad-cross-partition" else 99 if kind == "ad-bad-partition" else 0
        target = 100 if kind == "ad-outside" else 21
        directory = kind == "ad-directory"
        initial = [(0, block, 15, 0)] if kind in ("ad-short", "ad-long") else []
        descriptors(259 * block if directory else entry, initial + [(3, block, target, partition)], long)
        count = 16 if kind == "ad-limit-16" else 17 if kind == "ad-limit-17" else 3 if "chain" in kind else 1
        for i in range(count):
            if i + 1 < count or kind == "ad-cycle":
                extents = [(3, block, 21 if kind == "ad-cycle" else 22 + i, 0)]
            elif directory:
                extents = [(0, 180, 3, 0)]
            elif kind in ("ad-short", "ad-long"):
                extents = [(0, 3 * block, 16, 0)]
            elif "chain" in kind:
                extents = [(0, block, 15, 0), (2, block, 0xffffffff, 0), (0, 2 * block, 17, 0)]
                files["alpha.bin"] = files["alpha.bin"][:block] + bytes(block) + files["alpha.bin"][2 * block:]
            elif kind in ("ad-zero", "ad-implicit-tail"):
                extents = [(0, block, 15, 0)] + ([(0, 0, 0, 0), (0, 3 * block, 16, 0)] if kind == "ad-zero" else [])
                files["alpha.bin"] = files["alpha.bin"][:block] + bytes(3 * block)
            else:
                extents = [(0, 8192, 15, 1 if kind == "ad-cross-partition" else 0)]
            aed(21 + i, extents)
        if kind == "ad-overflow":
            struct.pack_into("<I", image, (257 + 21) * block + 20, 0xffffffff)
        if kind == "ad-over-block":
            struct.pack_into("<I", image, (257 + 21) * block + 20, block - 23)
    elif kind.startswith("metadata-"):
        # Keep user files on their original map; opening still has to read and
        # validate this additional metadata map before publishing any entries.
        pos = 277 * block
        image[pos:pos + block] = image[entry:entry + block]
        image[pos + 27] = 250
        struct.pack_into("<I", image, pos + 12, 20)
        struct.pack_into("<Q", image, pos + 56, block)
        descriptors(pos, [(1 if kind == "metadata-hole" else 0,
                           (22 if kind == "metadata-outside" else 21) * block, 0, 0)])
        for sector in (35, 51):
            lv = sector * block
            assert u32(lv + 264) == 6
            pm = bytearray(64)
            pm[0:2] = b"\x02\x40"
            pm[5:28] = b"*UDF Metadata Partition"
            struct.pack_into("<H", pm, 28, 0x250)
            struct.pack_into("<H", pm, 36, 1)
            struct.pack_into("<III", pm, 40, 20, 0xffffffff, 0xffffffff)
            struct.pack_into("<I", pm, 52, 1)
            struct.pack_into("<H", pm, 56, 1)
            image[lv + 446:lv + 510] = pm
            struct.pack_into("<II", image, lv + 264, 70, 2)
            tag(lv, 510)
    elif kind.startswith("sparse-"):
        descriptors(entry, [(0, block, 15, 0), (1, block, 16, 0),
                            (2, block, 0xffffffff, 0), (0, block, 18, 0)],
                    long=kind.endswith("long"))
        files["alpha.bin"] = files["alpha.bin"][:block] + bytes(2 * block) + files["alpha.bin"][-block:]
    elif kind in ("hole-only", "allocated-hole"):
        descriptors(entry, [(2 if kind == "hole-only" else 1, 8192, 0xffffffff, 0)])
        files["alpha.bin"] = bytes(8192)
    elif kind in ("eof-short", "eof-long", "unused-tail"):
        struct.pack_into("<Q", image, entry + 56, block + 13)
        extents = [(0, 8192, 15, 0)]
        if kind == "unused-tail":
            extents += [(0, 0x10000000, 0xffffffff, 99)]
        descriptors(entry, extents, long=kind != "eof-short")
        files["alpha.bin"] = files["alpha.bin"][:block + 13]
    elif kind == "eof-partition-tail":
        struct.pack_into("<Q", image, entry + 56, 13)
        descriptors(entry, [(0, 0x10000000, 19, 0)])
        files["alpha.bin"] = b"directory pay"
    elif kind in ("inline", "inline-length-mismatch", "directory-inline"):
        pos = 259 * block if kind == "directory-inline" else entry
        size = u32(pos + 56) if kind == "directory-inline" else 29
        data = bytearray(image[260 * block:260 * block + size]) if kind == "directory-inline" else files["alpha.bin"][:size]
        if kind == "directory-inline":
            offset = 0
            while offset < len(data):
                struct.pack_into("<I", data, offset + 12, 2)
                data[offset + 4] = 0
                data[offset + 4] = sum(data[offset:offset + 16]) & 255
                length = 38 + data[offset + 19] + struct.unpack_from("<H", data, offset + 36)[0]
                offset += (length + 3) & ~3
        flags = struct.unpack_from("<H", image, pos + 34)[0]
        struct.pack_into("<H", image, pos + 34, flags | 3)
        struct.pack_into("<Q", image, pos + 56, size)
        struct.pack_into("<Q", image, pos + 64, 0)
        struct.pack_into("<I", image, pos + 172, size + (7 if kind == "inline-length-mismatch" else 0))
        image[pos + 176:pos + block] = data + b"POISON!" + bytes(block - 183 - size)
        tag(pos, 176 + size + (7 if kind == "inline-length-mismatch" else 0))
        if kind != "directory-inline":
            files["alpha.bin"] = data
    elif kind == "directory-padded":
        descriptors(259 * block, [(0, block, 3, 0)])
        descriptors(261 * block, [(0, block, 5, 0)])
    elif kind == "recorded-outside":
        descriptors(entry, [(0, 8192, 0xffffffff, 0)])
    elif kind == "bad-partition":
        descriptors(entry, [(2, 8192, 0, 99)], long=True)
    elif kind in ("implicit-tail", "implicit-hole"):
        descriptors(entry, [(0, block, 15, 0)] if kind == "implicit-tail" else [])
        files["alpha.bin"] = files["alpha.bin"][:block] + bytes(3 * block) if kind == "implicit-tail" else bytes(4 * block)
    elif kind.startswith("terminator-"):
        descriptors(entry, [(0, block, 15, 0), (0, 0, 0xffffffff, 99),
                            (0, 3 * block, 16, 0)], long=kind.endswith("long"))
        files["alpha.bin"] = files["alpha.bin"][:block] + bytes(3 * block)
    elif kind == "continuation":
        descriptors(entry, [(3, block, 15, 0)])
    elif kind != "native":
        raise ValueError(kind)
    return bytes(image), files


if __name__ == "__main__":
    image, files = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    result = {name: data.hex() for name, data in files.items()}
    if "--metadata" in sys.argv[3:]:
        from udf_inode_fixture import attributes
        result = {"files": result, "attributes": attributes(sys.argv[1])}
    print(json.dumps(result))
