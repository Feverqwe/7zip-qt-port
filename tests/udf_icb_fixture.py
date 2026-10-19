"""Rewritten UDF inode hierarchies over disposable native producer images."""
import binascii
import struct
from udf_fixture import volume as base_volume

CASES = ("icb-file", "icb-efe", "icb-inline", "icb-root", "icb-directory",
         "icb-chain", "icb-limit-1024", "icb-cross-partition", "icb-ad-continuation",
         "icb-metadata", "icb-metadata-cross", "icb-no-ie", "icb-terminal",
         "icb-empty-ie", "icb-bad-ie-crc", "icb-bad-ie-location", "icb-reserved",
         "icb-extent-short", "icb-extent-long", "icb-root-extent-zero")
BAD = ("icb-cycle", "icb-limit-1025", "icb-bad-partition", "icb-outside",
       "icb-target-location", "icb-target-crc", "icb-target-version",
       "icb-target-checksum", "icb-target-crc-length", "icb-strategy",
       "icb-ad-type", "icb-metadata-long")


def volume(kind):
    block = 2048
    metadata = kind.startswith("icb-metadata")
    cross = kind in ("icb-cross-partition", "icb-metadata-cross")
    root = kind in ("icb-root", "icb-root-extent-zero")
    source_kind = "metadata-bootstrap" if metadata else "ad-cross-partition" if cross else "native"
    source, files = base_volume(source_kind)
    image = bytearray(source)
    links = 1024 if kind == "icb-limit-1024" else 1025 if kind == "icb-limit-1025" else 3 if kind == "icb-chain" else 1
    first = 20 if metadata else 21
    last = first + links * 2
    length = max(100, last + 10)
    if (257 + length + 1) * block > len(image):
        anchor = source[-block:]
        image.extend(bytes((257 + length + 257) * block - len(image)))
        image[-block:] = anchor
        image[-257 * block:-256 * block] = anchor

    def tag(pos, size=None):
        if size is None:
            size = 16 + struct.unpack_from("<H", image, pos + 10)[0]
        struct.pack_into("<HH", image, pos + 8,
                         binascii.crc_hqx(image[pos + 16:pos + size], 0), size - 16)
        image[pos + 4] = 0
        image[pos + 4] = sum(image[pos:pos + 16]) & 255

    if len(image) != len(source):
        for pos in (len(image) - block, len(image) - 257 * block):
            struct.pack_into("<I", image, pos + 12, pos // block)
            tag(pos)

    def entries(sector, size):
        pos = sector * block
        end = pos + size
        while pos < end:
            name_size = image[pos + 19]
            impl_size = struct.unpack_from("<H", image, pos + 36)[0]
            name = bytes(image[pos + 38 + impl_size:pos + 38 + impl_size + name_size])
            yield pos, name[1:]
            pos += (38 + impl_size + name_size + 3) & ~3

    def move_identifier(name):
        for pos, current in entries(260, 180):
            if current == name:
                struct.pack_into("<I", image, pos + 24, first)
                if kind in ("icb-extent-short", "icb-extent-long"):
                    struct.pack_into("<I", image, pos + 20, 32 if kind == "icb-extent-short" else 4096)
                tag(pos)
                return
        raise ValueError(name)

    for sector in (34, 50):
        struct.pack_into("<I", image, sector * block + 192, length)
        tag(sector * block)
    original_sector = 277 if metadata else 259 if root else 261 if kind == "icb-directory" else 263
    original = bytes(image[original_sector * block:(original_sector + 1) * block])
    if root:
        struct.pack_into("<I", image, 257 * block + 404, first)
        if kind == "icb-root-extent-zero":
            struct.pack_into("<I", image, 257 * block + 400, 0)
        tag(257 * block)
    elif not metadata:
        move_identifier(b"nested" if kind == "icb-directory" else b"alpha.bin")

    if kind == "icb-metadata-cross":
        image[360 * block:381 * block] = image[257 * block:278 * block]
        for source_sector, sector in ((34, 36), (50, 52)):
            pos = sector * block
            image[pos:pos + block] = image[source_sector * block:(source_sector + 1) * block]
            struct.pack_into("<I", image, pos + 12, sector)
            struct.pack_into("<H", image, pos + 22, 1)
            struct.pack_into("<II", image, pos + 188, 360, 60)
            tag(pos)
        # The dependent metadata inode refers to partition 1, which Linux
        # needs loaded before it processes partition 0's metadata map.
        for first_pd, second_pd in ((34, 36), (50, 52)):
            a, b = first_pd * block, second_pd * block
            left, right = bytes(image[a:a + block]), bytes(image[b:b + block])
            image[a:a + block], image[b:b + block] = right, left
            struct.pack_into("<I", image, a + 12, first_pd)
            struct.pack_into("<I", image, b + 12, second_pd)
            tag(a)
            tag(b)
        for sector in (35, 51):
            pos = sector * block
            metadata_map = bytes(image[pos + 446:pos + 510])
            image[pos + 452:pos + 516] = metadata_map
            image[pos + 446:pos + 452] = image[pos + 440:pos + 446]
            struct.pack_into("<H", image, pos + 450, 1)
            struct.pack_into("<II", image, pos + 264, 76, 3)
            struct.pack_into("<H", image, pos + 256, 2)
            struct.pack_into("<H", image, pos + 240, 0x250)
            tag(pos, 516)
        struct.pack_into("<H", image, 257 * block + 408, 2)
        tag(257 * block)
        for sector, size in ((260, 180), (262, 88)):
            for pos, _ in entries(sector, size):
                struct.pack_into("<H", image, pos + 28, 2)
                tag(pos)

    def write_fe(logical, partition, stale=False, extended=False):
        pos = ((360 if partition else 257) + logical) * block
        image[pos:pos + block] = original
        struct.pack_into("<I", image, pos + 12, logical)
        struct.pack_into("<H", image, pos + 20, 4096 if stale else 4)
        # Final timestamps also identify the selected revision through metadata.
        if not stale:
            struct.pack_into("<H", image, pos + 86, 2041)
        if stale:
            struct.pack_into("<Q", image, pos + 56, 0)
            struct.pack_into("<I", image, pos + 172, 0)
        elif metadata:
            struct.pack_into("<Q", image, pos + 56, 21 * block)
            if kind == "icb-metadata-long":
                struct.pack_into("<H", image, pos + 34, 1)
                struct.pack_into("<I", image, pos + 172, 16)
                image[pos + 176:pos + 192] = struct.pack("<IIH6x", 21 * block, 0, 0)
        elif not root and kind != "icb-directory":
            payload = (b"latest UDF inode\n" * 512)[:8192]
            files["alpha.bin"] = payload
            data_logical = last + 2 if links >= 1024 else 41
            data_sector = (360 if partition else 257) + data_logical
            image[data_sector * block:(data_sector + 4) * block] = payload
            struct.pack_into("<Q", image, pos + 56, len(payload))
            struct.pack_into("<H", image, pos + 34, 0)
            struct.pack_into("<I", image, pos + 172, 8)
            struct.pack_into("<II", image, pos + 176, len(payload), data_logical)
            if kind == "icb-inline":
                files["alpha.bin"] = b"inline selected inode"
                struct.pack_into("<Q", image, pos + 56, len(files["alpha.bin"]))
                struct.pack_into("<H", image, pos + 34, 3)
                struct.pack_into("<I", image, pos + 172, len(files["alpha.bin"]))
                image[pos + 176:pos + 176 + len(files["alpha.bin"])] = files["alpha.bin"]
            elif kind == "icb-ad-continuation":
                struct.pack_into("<II", image, pos + 176, (3 << 30) | block, 31)
                aed = 288 * block
                image[aed:aed + block] = bytes(block)
                struct.pack_into("<HH", image, aed, 258, 2)
                struct.pack_into("<I", image, aed + 12, 31)
                struct.pack_into("<I", image, aed + 20, 8)
                struct.pack_into("<II", image, aed + 24, len(payload), 41)
                tag(aed, 32)
        if extended:
            old = bytes(image[pos:pos + block])
            image[pos:pos + block] = bytes(block)
            for dst, src, count in ((0, 0, 64), (64, 56, 8), (72, 64, 8), (80, 72, 24),
                                    (104, 84, 12), (116, 96, 12), (128, 108, 4),
                                    (136, 112, 16), (168, 128, 48)):
                image[pos + dst:pos + dst + count] = old[src:src + count]
            image[pos + 216:pos + block] = old[176:block - 40]
            struct.pack_into("<H", image, pos, 266)
        offset = 40 if extended else 0
        size = 176 + offset + struct.unpack_from("<I", image, pos + 172 + offset)[0]
        tag(pos, size)
        return pos

    for i in range(links):
        logical = first + i * 2
        write_fe(logical, 0, stale=True)
        ie = (257 + logical + 1) * block
        image[ie:ie + block] = bytes(block)
        struct.pack_into("<HH", image, ie, 259, 2)
        struct.pack_into("<I", image, ie + 12, logical + 1)
        target = first if kind == "icb-cycle" else logical + 2
        part = 99 if kind == "icb-bad-partition" else 1 if cross and i + 1 == links else 0
        if kind == "icb-outside":
            target = length
        struct.pack_into("<IIH", image, ie + 36, block, target, part)
        tag(ie, 52)
    final = write_fe(last, int(cross), extended=kind == "icb-efe")
    ie = (257 + first + 1) * block
    fallback = kind in ("icb-no-ie", "icb-terminal", "icb-empty-ie", "icb-bad-ie-crc", "icb-bad-ie-location")
    if fallback:
        # Stale FE is a valid empty file if its optional IE cannot select a revision.
        files["alpha.bin"] = b""
        if kind == "icb-no-ie":
            image[ie:ie + block] = bytes(block)
        elif kind == "icb-terminal":
            struct.pack_into("<H", image, ie, 260)
            tag(ie, 36)
        elif kind == "icb-empty-ie":
            struct.pack_into("<I", image, ie + 36, 0)
            tag(ie, 52)
        elif kind == "icb-bad-ie-crc":
            image[ie + 40] ^= 1
        else:
            struct.pack_into("<I", image, ie + 12, 0)
            tag(ie, 52)
    if kind == "icb-metadata-cross":
        files["alpha.bin"] = (b"physical metadata backing\n" * 400)[:8192]
        image[272 * block:276 * block] = files["alpha.bin"]
    if kind == "icb-reserved":
        image[final + 5] = 7
        tag(final)
    elif kind == "icb-strategy":
        struct.pack_into("<H", image, final + 20, 3)
        tag(final)
    elif kind == "icb-ad-type":
        struct.pack_into("<H", image, final + 34, 2)
        tag(final)
    elif kind == "icb-target-location":
        struct.pack_into("<I", image, final + 12, 0)
        tag(final)
    elif kind == "icb-target-crc":
        image[final + 56] ^= 1
    elif kind == "icb-target-version":
        struct.pack_into("<H", image, final + 2, 9)
        tag(final)
    elif kind == "icb-target-checksum":
        image[final + 4] ^= 1
    elif kind == "icb-target-crc-length":
        struct.pack_into("<H", image, final + 10, block)
        image[final + 4] = 0
        image[final + 4] = sum(image[final:final + 16]) & 255
    return bytes(image), files
