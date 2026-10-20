"""Read-only PD allocation setup, ordered dependencies and VDS recovery."""
import binascii
import struct
from udf_fixture import volume as base_volume

CASES = ("ua-table", "ua-table-fe", "ua-table-inline", "ua-table-indirect",
         "ua-table-length-flags", "ua-bitmap", "ua-readonly", "ua-writeonce", "ua-none",
         "ua-domain-dirty", "ua-domain-ident", "ua-domain-protected", "ua-missing-lvid",
         "ua-invalid-lvid", "ua-contents", "ua-freed-table", "ua-freed-bitmap",
         "ua-mixed", "ua-no-info", "ua-order-ro-first", "ua-backward-table")
BAD = ("ua-bad-table", "ua-table-outside", "ua-table-tag", "ua-table-strategy",
       "ua-table-hidden", "ua-order-table-first", "ua-forward-table",
       "ua-reserve-table", "ua-reserve-eof", "ua-reserve-metadata", "ua-reserve-vat")
MODULE_BAD = ("ua-bitmap-overflow", "ua-use-length")


def volume(kind):
    source = ("inline" if kind == "ua-table-inline" else "metadata-bootstrap" if kind == "ua-reserve-metadata"
              else "vat15-inline" if kind == "ua-reserve-vat" else "native")
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

    if kind in ("ua-reserve-metadata", "ua-reserve-vat"):
        pos = 51 * block
        image[pos + 446:pos + block] = bytes(block - 446)
        struct.pack_into("<II", image, pos + 264, 6, 1)
        struct.pack_into("<H", image, pos + 256, 0)
        tag(pos, 446)
        if kind == "ua-reserve-metadata":
            struct.pack_into("<I", image, 35 * block + 446 + 40, 99)
            tag(35 * block)
        else:
            last = len(image) // block - 1
            image[last * block + 27] = 255
            tag(last * block)
            struct.pack_into("<II", image, 50 * block + 188, 300, 21)
            tag(50 * block)
            struct.pack_into("<H", image, 300 * block + 408, 0)
            tag(300 * block)
            for sector, size in ((303, 180), (305, 88)):
                offset = sector * block
                while offset < sector * block + size:
                    struct.pack_into("<H", image, offset + 28, 0)
                    tag(offset)
                    offset += (38 + image[offset + 19] + struct.unpack_from("<H", image, offset + 36)[0] + 3) & ~3
        return bytes(image), files

    pos = 277 * block
    if kind in ("ua-table-fe", "ua-table-inline", "ua-table-indirect", "ua-table-hidden",
                "ua-forward-table", "ua-backward-table"):
        image[pos:pos + block] = image[263 * block:264 * block]
        struct.pack_into("<I", image, pos + 12, 20)
        tag(pos)
    else:
        image[pos:pos + block] = bytes(block)
        struct.pack_into("<HH", image, pos, 263, 2)
        struct.pack_into("<I", image, pos + 12, 20)
        struct.pack_into("<H", image, pos + 20, 4)
        image[pos + 27] = 1
        struct.pack_into("<I", image, pos + 36, block - 39 if kind == "ua-use-length" else 8)
        tag(pos, block if kind == "ua-use-length" else 48)
    if kind == "ua-bad-table":
        image[pos + 4] ^= 1
    elif kind == "ua-table-tag":
        struct.pack_into("<I", image, pos + 12, 99)
        tag(pos)
    elif kind == "ua-table-strategy":
        struct.pack_into("<H", image, pos + 20, 2)
        tag(pos)

    for sector in (34, 50):
        pd = sector * block
        image[pd + 56:pd + 184] = bytes(128)
        struct.pack_into("<III", image, pd + 184, 3, 257,
                         100 if kind in ("ua-table-indirect", "ua-forward-table", "ua-backward-table") else 21)
        struct.pack_into("<II", image, pd + 56, (3 << 30) | 1 if kind == "ua-table-length-flags" else block,
                         99 if kind == "ua-table-outside" else 20)
        if kind in ("ua-readonly", "ua-writeonce", "ua-none"):
            struct.pack_into("<I", image, pd + 184,
                             {"ua-readonly": 1, "ua-writeonce": 2, "ua-none": 0}[kind])
        if kind in ("ua-bitmap", "ua-bitmap-overflow"):
            struct.pack_into("<II", image, pd + 56, 0, 0)
            struct.pack_into("<II", image, pd + 64, block, 0xffffffff)
        if kind == "ua-bitmap-overflow":
            struct.pack_into("<II", image, pd + 188, 0, 0xffffffff)
        if kind == "ua-contents":
            image[pd + 25:pd + 32] = b"BADTYPE"
        if kind in ("ua-freed-table", "ua-freed-bitmap", "ua-mixed"):
            offset = 80 if kind == "ua-freed-table" else 88 if kind == "ua-freed-bitmap" else 64
            struct.pack_into("<II", image, pd + offset, block, 0xffffffff)
        if kind == "ua-no-info":
            image[pd + 56:pd + 184] = bytes(128)
        tag(pd)
        lv = (sector + 1) * block
        image[lv + 242] = 1 if kind == "ua-domain-protected" else 0
        if kind == "ua-domain-dirty": image[lv + 216] = 1
        if kind == "ua-domain-ident": image[lv + 217] = ord('!')
        if kind == "ua-missing-lvid": struct.pack_into("<I", image, lv + 432, 0)
        tag(lv)

    if kind in ("ua-readonly", "ua-writeonce", "ua-none", "ua-domain-dirty", "ua-domain-ident",
                "ua-domain-protected", "ua-missing-lvid", "ua-invalid-lvid", "ua-contents",
                "ua-freed-table", "ua-freed-bitmap", "ua-mixed", "ua-no-info"):
        # These policies must skip, rather than reject, an unreadable allocation table.
        image[277 * block + 4] ^= 1
    if kind == "ua-invalid-lvid":
        image[64 * block + 4] ^= 1

    if kind == "ua-bitmap-overflow":
        recognition = bytes(image[16 * block:19 * block])
        image[:21 * block] = image[257 * block:278 * block]
        image[16 * block:19 * block] = recognition
        image[100 * block:104 * block] = files["alpha.bin"]
        struct.pack_into("<I", image, 6 * block + 180, 100)
        tag(6 * block)

    if kind in ("ua-table-indirect", "ua-forward-table", "ua-backward-table"):
        image[279 * block:280 * block] = image[277 * block:278 * block]
        struct.pack_into("<I", image, 279 * block + 12, 22)
        tag(279 * block)
        struct.pack_into("<H", image, 277 * block + 20, 4096)
        tag(277 * block)
        ie = 278 * block
        image[ie:ie + block] = bytes(block)
        struct.pack_into("<HH", image, ie, 259, 2)
        struct.pack_into("<I", image, ie + 12, 21)
        struct.pack_into("<IIH", image, ie + 36, block,
                         6 if kind != "ua-table-indirect" else 22,
                         1 if kind != "ua-table-indirect" else 0)
        tag(ie, 52)

    if kind in ("ua-order-ro-first", "ua-order-table-first", "ua-forward-table", "ua-backward-table"):
        cross = kind in ("ua-forward-table", "ua-backward-table")
        for first_pd in (34, 50):
            first, second = first_pd * block, (first_pd + 2) * block
            image[second:second + block] = image[first:first + block]
            struct.pack_into("<I", image, second + 12, first_pd + 2)
            struct.pack_into("<H", image, second + 22, 1)
            struct.pack_into("<III", image, second + 184, 3, 360, 60)
            if cross:
                struct.pack_into("<II", image, second + 56, 0, 0)
                struct.pack_into("<II", image, second + 64, block, 0xffffffff)
            else:
                struct.pack_into("<I", image, first + 184, 1)
                struct.pack_into("<I", image, second + 60, 99)
            tag(first); tag(second)
            if kind in ("ua-order-table-first", "ua-backward-table"):
                left, right = bytes(image[first:first + block]), bytes(image[second:second + block])
                image[first:first + block], image[second:second + block] = right, left
                struct.pack_into("<I", image, first + 12, first_pd)
                struct.pack_into("<I", image, second + 12, first_pd + 2)
                tag(first); tag(second)
            lv = (first_pd + 1) * block
            image[lv + 446:lv + 452] = struct.pack("<BBHH", 1, 6, 1, 1)
            struct.pack_into("<II", image, lv + 264, 12, 2)
            tag(lv, 452)
        image[366 * block:367 * block] = image[263 * block:264 * block]

    if kind in ("ua-reserve-table", "ua-reserve-eof"):
        image[50 * block:52 * block] = original[50 * block:52 * block]
        struct.pack_into("<I", image, 34 * block + 188, 300)
        if kind == "ua-reserve-eof":
            struct.pack_into("<I", image, 34 * block + 192, 1000)
            struct.pack_into("<I", image, 34 * block + 60, 500)
        tag(34 * block)
    if kind == "ua-table-hidden":
        offset = 260 * block
        while offset < 260 * block + 180:
            if image[offset + 39:offset + 48] == b"alpha.bin":
                struct.pack_into("<I", image, offset + 24, 20)
                tag(offset)
                break
            offset += (38 + image[offset + 19] + struct.unpack_from("<H", image, offset + 36)[0] + 3) & ~3
    return bytes(image), files
