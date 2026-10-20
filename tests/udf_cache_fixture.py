"""Physical inode aliases, hidden-role conflicts and non-FE inode publication."""
import binascii
import struct
from udf_fixture import volume as base_volume

CASES = ("uc-unknown", "uc-duplicate", "uc-bounded", "uc-inline", "uc-indirect",
         "uc-sparable", "uc-virtual", "uc-metadata", "uc-use-metadata", "uc-rekey")
METADATA_ONLY = ("uc-use-short", "uc-use-long", "uc-use-inline", "uc-use-chain")
BAD = ("uc-before", "uc-hidden-metadata", "uc-hidden-root", "uc-hidden-vat15",
       "uc-hidden-vat20", "uc-use-root", "uc-use-ad")
MODULE_BAD = ("uc-use-length",)


def volume(kind):
    source = ("inline" if kind == "uc-inline" else "icb-file" if kind in ("uc-indirect", "uc-use-chain")
              else "sp-packet" if kind == "uc-sparable"
              else "md-ad-chain" if kind == "uc-rekey"
              else "vat15-inline" if kind in ("uc-virtual", "uc-hidden-vat15")
              else "vat20-inline" if kind == "uc-hidden-vat20"
              else "metadata-bootstrap" if kind in ("uc-metadata", "uc-use-metadata", "uc-hidden-metadata", "uc-hidden-root")
              else "pm-duplicate" if kind == "uc-duplicate" else "native")
    original, files = base_volume(source)
    image = bytearray(original)
    block = 2048

    def tag(pos, size=None):
        if size is None:
            size = 16 + struct.unpack_from("<H", image, pos + 10)[0]
        struct.pack_into("<HH", image, pos + 8,
                         binascii.crc_hqx(image[pos + 16:pos + size], 0), size - 16)
        image[pos + 4] = 0
        image[pos + 4] = sum(image[pos:pos + 16]) & 255

    if kind.startswith("uc-use-"):
        sector = 277 if kind == "uc-use-metadata" else 259 if kind == "uc-use-root" else 280 if kind == "uc-use-chain" else 263
        pos = sector * block
        logical = sector - 257
        image[pos:pos + block] = bytes(block)
        struct.pack_into("<HH", image, pos, 263, 2)
        struct.pack_into("<I", image, pos + 12, logical)
        struct.pack_into("<H", image, pos + 20, 4)
        image[pos + 27] = 4 if kind == "uc-use-root" else 1
        descriptor = 1 if kind == "uc-use-long" else 3 if kind == "uc-use-inline" else 2 if kind == "uc-use-ad" else 0
        struct.pack_into("<H", image, pos + 34, descriptor)
        length = block - 39 if kind == "uc-use-length" else 16 if descriptor == 1 else 8
        struct.pack_into("<I", image, pos + 36, length)
        image[pos + 40:pos + 48] = struct.pack("<II", 21 * block, 0) if kind == "uc-use-metadata" else b"no file!"
        tag(pos, min(block, 40 + length))
        if kind == "uc-use-metadata":
            for sector in (35, 51):
                struct.pack_into("<H", image, sector * block + 256, 1)
                tag(sector * block)
            struct.pack_into("<H", image, 257 * block + 408, 1)
            tag(257 * block)
            for sector, size in ((260, 180), (262, 88)):
                offset = sector * block
                while offset < sector * block + size:
                    struct.pack_into("<H", image, offset + 28, 1)
                    tag(offset)
                    offset += (38 + image[offset + 19] + struct.unpack_from("<H", image, offset + 36)[0] + 3) & ~3
            return bytes(image), files
        files["alpha.bin"] = b""
        return bytes(image), files

    if kind == "uc-hidden-root":
        for sector in (35, 51):
            struct.pack_into("<I", image, sector * block + 446 + 40, 2)
            tag(sector * block)
        return bytes(image), files

    data_sector, inode_sector = (323, 322) if kind == "uc-sparable" else (303, 302) if source.startswith("vat") or kind == "uc-rekey" else (260, 259)
    entries = []
    offset = data_sector * block
    while offset < data_sector * block + 180:
        size = (38 + image[offset + 19] + struct.unpack_from("<H", image, offset + 36)[0] + 3) & ~3
        entries.append(bytearray(image[offset:offset + size]))
        offset += size
    alpha = next(entry for entry in entries if entry[39:48] == b"alpha.bin")

    if kind == "uc-rekey":
        for sector in (341, 509):
            image[sector * block:(sector + 1) * block] = image[306 * block:307 * block]
            struct.pack_into("<I", image, sector * block + 12, 9)
            tag(sector * block)
        struct.pack_into("<I", alpha, 24, 9)
        alias_partition, alias_block = 0, 84
    elif kind.startswith("uc-hidden-"):
        alias_partition = 0
        alias_block = 20 if kind == "uc-hidden-metadata" else len(image) // block - 1 - 257
    else:
        alias_block = 326 if kind == "uc-sparable" else 306 if kind == "uc-virtual" else 278 if kind == "uc-indirect" else 263
        for sector in (35, 51):
            pos = sector * block
            length, count = struct.unpack_from("<II", image, pos + 264)
            alias_partition = 1 if kind == "uc-duplicate" else count
            if kind != "uc-duplicate":
                extra = struct.pack("<BBHH", 1 if kind == "uc-bounded" else 42, 6, 1, 1)
                image[pos + 440 + length:pos + 446 + length] = extra
                struct.pack_into("<II", image, pos + 264, length + 6, count + 1)
                tag(pos, 446 + length)
            if kind == "uc-metadata":
                struct.pack_into("<H", image, pos + 256, 1)
                tag(pos)
        if kind == "uc-bounded":
            alias_block = 3
            for source_pd, target in ((34, 36), (50, 52)):
                pos = target * block
                image[pos:pos + block] = image[source_pd * block:(source_pd + 1) * block]
                struct.pack_into("<I", image, pos + 12, target)
                struct.pack_into("<H", image, pos + 22, 1)
                struct.pack_into("<II", image, pos + 188, 260, 1)
                tag(pos)
        if kind == "uc-metadata":
            struct.pack_into("<H", image, 257 * block + 408, 1)
            tag(257 * block)
            for entry in entries:
                struct.pack_into("<H", entry, 28, 1)

    alias = bytearray(alpha[:38]) + b"\x08alias.bin"
    alias.extend(bytes((-len(alias)) % 4))
    alias[19] = 10
    struct.pack_into("<IH", alias, 24, alias_block, alias_partition)
    entries.insert(1, alias) if kind == "uc-before" else entries.append(alias)
    directory = bytearray()
    for entry in entries:
        struct.pack_into("<H", entry, 10, len(entry) - 16)
        struct.pack_into("<H", entry, 8, binascii.crc_hqx(entry[16:], 0))
        entry[4] = 0
        entry[4] = sum(entry[:16]) & 255
        directory.extend(entry)
    pos = data_sector * block
    image[pos:pos + block] = directory + bytes(block - len(directory))
    pos = inode_sector * block
    struct.pack_into("<Q", image, pos + 56, len(directory))
    struct.pack_into("<I", image, pos + 176, len(directory))
    tag(pos)
    files["alias.bin"] = files["alpha.bin"]
    return bytes(image), files


def attributes(kind):
    if kind not in METADATA_ONLY:
        raise ValueError(kind)
    return {"alpha.bin": {"mode": 0, "uid": 0, "gid": 0, "links": 1, "size": 0}}
