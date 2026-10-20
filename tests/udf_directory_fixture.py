"""UDF FID policies and block/extent iterator variants from native images."""
import binascii
import struct

CASES = ("fi-checksum", "fi-crc", "fi-version", "fi-location", "fi-reserved",
         "fi-padding", "fi-file-version", "fi-directory-hint", "fi-file-hint",
         "fi-impl-use", "fi-empty", "fi-extent-short", "fi-header-split",
         "fi-name-split", "fi-impl-split", "fi-boundary", "fi-fragmented",
         "fi-aed", "fi-long-partition", "fi-inline-padding", "fi-inline-crc",
         "fi-inline-ea", "fi-deleted-invalid", "fi-hidden-invalid")
BAD = ("fi-ident", "fi-crc-length", "fi-imp-unaligned", "fi-entry-large",
       "fi-tail-header", "fi-tail-entry", "fi-hole", "fi-implicit-hole", "fi-root-file", "fi-root-device")
MODULE_BAD = ("fi-ad-cycle",)


def volume(kind):
    from udf_fixture import volume as base
    source = "ia-char" if kind == "fi-root-device" else "ad-cross-partition" if kind == "fi-long-partition" else "directory-inline" if kind.startswith("fi-inline-") else "native"
    image, files = base(source)
    image = bytearray(image)
    block = 2048
    root = 259 * block

    def u32(at): return struct.unpack_from("<I", image, at)[0]
    def tag(at, length=None):
        if length is None: length = 16 + struct.unpack_from("<H", image, at + 10)[0]
        struct.pack_into("<HH", image, at + 8,
                         binascii.crc_hqx(image[at + 16:at + length], 0), length - 16)
        image[at + 4] = 0
        image[at + 4] = sum(image[at:at + 16]) & 255
    def record(template, length=None, imp=0):
        old_imp = struct.unpack_from("<H", template, 36)[0]
        name = template[38 + old_imp:38 + old_imp + template[19]]
        header = bytearray(template[:38])
        if length is not None: imp = length - 40
        struct.pack_into("<H", header, 36, imp)
        result = header + bytes([0xab]) * imp + name
        result += bytes((-len(result)) & 3)
        struct.pack_into("<HH", result, 8, binascii.crc_hqx(result[16:], 0), len(result) - 16)
        result[4] = 0
        result[4] = sum(result[:16]) & 255
        return result
    def allocation(entries, long=False):
        data = b"".join(struct.pack("<IIH6x", length | (type << 30), location, partition)
                        if long else struct.pack("<II", length | (type << 30), location)
                        for type, length, location, partition in entries)
        flags = struct.unpack_from("<H", image, root + 34)[0]
        struct.pack_into("<H", image, root + 34, (flags & ~3) | int(long))
        struct.pack_into("<I", image, root + 172, len(data))
        image[root + 176:root + block] = data + bytes(block - 176 - len(data))
        tag(root, 176 + len(data))

    if kind in ("fi-root-file", "fi-root-device"):
        struct.pack_into("<I", image, 257 * block + 404, 6)
        tag(257 * block)
        return bytes(image), files
    inline = kind.startswith("fi-inline-")
    directory = root + 176 if inline else 260 * block
    if kind == "fi-inline-ea":
        data = image[root + 176:root + 356]
        image[root + 176:root + 380] = bytes(24) + data
        struct.pack_into("<I", image, root + 168, 24)
        tag(root, 380)
        return bytes(image), files
    alpha = directory + 40
    if kind in ("fi-header-split", "fi-name-split", "fi-impl-split", "fi-boundary",
                "fi-fragmented", "fi-aed", "fi-long-partition", "fi-ad-cycle"):
        parent = bytes(image[260 * block:260 * block + 40])
        alpha_record = bytes(image[260 * block + 40:260 * block + 88])
        tail = bytes(image[260 * block + 88:260 * block + 180])
        start = block - 4 if kind == "fi-header-split" else block - 44 if kind == "fi-name-split" else block - 48 if kind == "fi-boundary" else block - 64
        filler = bytearray(parent)
        filler[18] = 4
        data = parent + record(filler, start - 40) + record(alpha_record, imp=64 if kind == "fi-impl-split" else 0) + tail
        if kind == "fi-ad-cycle":
            data = record(filler, block)
            files = {}
        size = len(data)
        for sector in (34, 50):
            struct.pack_into("<I", image, sector * block + 192, 100)
            tag(sector * block)
        fragmented = kind in ("fi-fragmented", "fi-aed", "fi-long-partition", "fi-ad-cycle")
        second = (360 + 33) if kind == "fi-long-partition" else 257 + 33 if fragmented else 257 + 31
        image[(257 + 30) * block:(257 + 31) * block] = data[:block]
        image[second * block:(second + 1) * block] = data[block:] + bytes(2 * block - len(data))
        struct.pack_into("<Q", image, root + 56, 64 * block if kind == "fi-ad-cycle" else size)
        entries = [(0, block, 30, 0), (0, size - block, 33, 1 if kind == "fi-long-partition" else 0)] if fragmented else [(0, size, 30, 0)]
        if kind in ("fi-aed", "fi-ad-cycle"):
            allocation([(3, block, 21, 0)])
            at = (257 + 21) * block
            entries = [(0, block, 30, 0), (3, block, 21, 0)] if kind == "fi-ad-cycle" else entries
            ad = b"".join(struct.pack("<II", length | (type << 30), location) for type, length, location, _ in entries)
            image[at:at + block] = bytes(block)
            struct.pack_into("<HH", image, at, 258, 2)
            struct.pack_into("<II", image, at + 12, 21, 0)
            struct.pack_into("<I", image, at + 20, len(ad))
            image[at + 24:at + 24 + len(ad)] = ad
            tag(at, 24 + len(ad))
        else: allocation(entries, long=kind == "fi-long-partition")
        return bytes(image), files

    if kind == "fi-empty":
        struct.pack_into("<Q", image, root + 56, 0)
        struct.pack_into("<I", image, root + 172, 0)
        tag(root, 176)
        return bytes(image), {}
    if kind == "fi-extent-short":
        allocation([(0, 1, 3, 0)])
        return bytes(image), files
    if kind in ("fi-hole", "fi-implicit-hole", "fi-root-file", "fi-root-device"):
        allocation([(2, 180, 3, 0)] if kind == "fi-hole" else [])
        return bytes(image), files
    if kind == "fi-impl-use":
        parent = bytes(image[directory:directory + 40])
        tail = bytes(image[directory + 88:directory + 180])
        data = parent + record(image[alpha:alpha + 48], imp=8) + tail
        image[directory:directory + len(data)] = data
        struct.pack_into("<Q", image, root + 56, len(data))
        allocation([(0, len(data), 3, 0)])
        return bytes(image), files
    if kind == "fi-imp-unaligned":
        data = bytearray(image[alpha:alpha + 48])
        struct.pack_into("<H", data, 36, 2)
        # Keep the entry structurally valid for a byte-wise parser, with name moved.
        data = data[:38] + bytes(2) + data[38:48]
        data += bytes((-len(data)) & 3)
        tail = bytes(image[directory + 88:directory + 180])
        image[alpha:alpha + len(data) + len(tail)] = data + tail
        struct.pack_into("<Q", image, root + 56, 40 + len(data) + len(tail))
        allocation([(0, 40 + len(data) + len(tail), 3, 0)])
        tag(alpha, len(data))
        return bytes(image), files
    if kind == "fi-entry-large":
        struct.pack_into("<H", image, alpha + 36, 2048)
        struct.pack_into("<H", image, alpha + 10, 2080)
    elif kind == "fi-tail-header":
        struct.pack_into("<Q", image, root + 56, 40 + 16)
        tag(root)
        return bytes(image), files
    elif kind == "fi-tail-entry":
        struct.pack_into("<Q", image, root + 56, 40 + 40)
        tag(root)
        return bytes(image), files
    elif kind == "fi-checksum": image[alpha + 4] ^= 0xff
    elif kind in ("fi-crc", "fi-inline-crc"): image[alpha + 8] ^= 0xff
    elif kind == "fi-version": struct.pack_into("<H", image, alpha + 2, 0xffff)
    elif kind == "fi-location": struct.pack_into("<I", image, alpha + 12, 0xffffffff)
    elif kind == "fi-reserved": image[alpha + 5] = 0x81
    elif kind in ("fi-padding", "fi-inline-padding"): image[directory + 38:directory + 40] = b"!!"
    elif kind == "fi-file-version": struct.pack_into("<H", image, alpha + 16, 55)
    elif kind == "fi-directory-hint": image[directory + 132 + 18] &= ~2
    elif kind == "fi-file-hint": image[alpha + 18] |= 2
    elif kind in ("fi-deleted-invalid", "fi-hidden-invalid"):
        image[alpha + 18] |= 4 if kind == "fi-deleted-invalid" else 1
        struct.pack_into("<H", image, alpha + 28, 99)
        files.pop("alpha.bin")
    elif kind == "fi-ident": struct.pack_into("<H", image, alpha, 258)
    elif kind == "fi-crc-length": struct.pack_into("<H", image, alpha + 10, 16)
    else: raise ValueError(kind)
    # FID integrity fields are deliberately left invalid; Linux iterator reads
    # the record's type/length, rather than tag checksum/CRC/version/location.
    if inline: tag(root)
    return bytes(image), files
