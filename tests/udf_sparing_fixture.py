"""Sparable partition packets over disposable native UDF images."""
import binascii
import struct
from udf_fixture import volume as base_volume

CASES = ("sp-packet", "sp-cross", "sp-reverse", "sp-backup", "sp-first-wins",
         "sp-no-table", "sp-invalid-table", "sp-table-overrun", "sp-max-table",
         "sp-declared-zero", "sp-reserved", "sp-sparse", "sp-icb",
         "sp-directory-cross", "sp-metadata")
BAD = ("sp-packet-zero", "sp-packet-unaligned", "sp-too-many-tables",
       "sp-table-too-large", "sp-mapped-outside", "sp-mapped-sentinel",
       "sp-wrapped-target", "sp-wrapped-fallback")


def volume(kind):
    block = 2048
    source_kind = "sparse-long" if kind == "sp-sparse" else "icb-file" if kind == "sp-icb" else "metadata-bootstrap" if kind == "sp-metadata" else "native"
    source, files = base_volume(source_kind)
    image = bytearray(source)
    packet = 2 if kind in ("sp-cross", "sp-reverse", "sp-directory-cross", "sp-wrapped-target") else 32
    count = 2 if kind in ("sp-backup", "sp-first-wins") else 0 if kind == "sp-no-table" else 1
    fallback = kind in ("sp-no-table", "sp-invalid-table", "sp-table-overrun")

    def tag(pos, size=None):
        if size is None:
            size = 16 + struct.unpack_from("<H", image, pos + 10)[0]
        struct.pack_into("<HH", image, pos + 8,
                         binascii.crc_hqx(image[pos + 16:pos + size], 0), size - 16)
        image[pos + 4] = 0
        image[pos + 4] = sum(image[pos:pos + 16]) & 255

    if kind == "sp-directory-cross":
        def records(data):
            pos = 0
            while pos < len(data):
                length = (38 + data[pos + 19] + struct.unpack_from("<H", data, pos + 36)[0] + 3) & ~3
                yield bytearray(data[pos:pos + length])
                pos += length
        entries = list(records(image[260 * block:260 * block + 180]))
        alpha = next(r for r in entries if bytes(r[39:39 + r[19] - 1]) == b"alpha.bin")
        for i in range(48):
            name = ("alias-%03d" % i).encode()
            r = bytearray(alpha[:38]) + b"\x08" + name
            r.extend(bytes((-len(r)) % 4))
            r[19] = len(name) + 1
            struct.pack_into("<H", r, 10, len(r) - 16)
            entries.append(r)
            files[name.decode()] = files["alpha.bin"]
        data = bytearray()
        for r in entries:
            struct.pack_into("<I", r, 12, 31 + len(data) // block)
            r[4] = 0
            struct.pack_into("<H", r, 8, binascii.crc_hqx(r[16:], 0))
            r[4] = sum(r[:16]) & 255
            data += r
        assert block < len(data) < 2 * block
        image[288 * block:288 * block + len(data)] = data
        pos = 259 * block
        struct.pack_into("<Q", image, pos + 56, len(data))
        struct.pack_into("<Q", image, pos + 64, 2)
        struct.pack_into("<II", image, pos + 176, len(data), 31)
        tag(pos)

    if kind == "sp-metadata":
        for sector in (35, 51):
            struct.pack_into("<H", image, sector * block + 256, 1)
            tag(sector * block)
        struct.pack_into("<H", image, 257 * block + 408, 1)
        tag(257 * block)
        for sector, size in ((260, 180), (262, 88)):
            pos = sector * block
            while pos < sector * block + size:
                struct.pack_into("<H", image, pos + 28, 1)
                tag(pos)
                pos += (38 + image[pos + 19] + struct.unpack_from("<H", image, pos + 36)[0] + 3) & ~3

    length = 100 if kind in ("sp-icb", "sp-wrapped-fallback") else 64 if kind == "sp-directory-cross" else 21
    for sector in (34, 50):
        struct.pack_into("<I", image, sector * block + 192, length)
        tag(sector * block)
    mappings = [(16, 320)] if kind in ("sp-cross", "sp-wrapped-target") else [(14, 324), (16, 320), (18, 326)] if kind == "sp-reverse" else [(30, 320), (32, 324)] if kind == "sp-directory-cross" else [(0, 320)]
    if not fallback:
        for original, mapped in mappings:
            image[mapped * block:(mapped + packet) * block] = image[(257 + original) * block:(257 + original + packet) * block]
        if kind == "sp-first-wins":
            image[352 * block:384 * block] = image[320 * block:352 * block]
            image[367 * block:371 * block] = b"X" * (4 * block)
        for original, _ in mappings:
            image[(257 + original) * block:(257 + original + packet) * block] = b"!" * (packet * block)
    if kind in ("sp-wrapped-target", "sp-wrapped-fallback"):
        pos = (263 if kind == "sp-wrapped-target" else 326) * block
        struct.pack_into("<I", image, pos + 180, 17 if kind == "sp-wrapped-target" else 32)
        tag(pos)
        if kind == "sp-wrapped-fallback":
            for sector in (34, 50):
                struct.pack_into("<I", image, sector * block + 188, 0xfffffff0)
                tag(sector * block)

    def table(sector, pairs, sequence):
        pos = sector * block
        image[pos:pos + block] = bytes(block)
        struct.pack_into("<HH", image, pos, 0, 2)
        struct.pack_into("<I", image, pos + 12, sector)
        image[pos + 17:pos + 35] = b"*UDF Sparing Table"
        struct.pack_into("<H", image, pos + 48, len(pairs))
        struct.pack_into("<I", image, pos + 52, sequence)
        for i, (original, mapped) in enumerate(pairs):
            struct.pack_into("<II", image, pos + 56 + i * 8, original, mapped)
        tag(pos, 56 + len(pairs) * 8)

    pairs = mappings + ([(0xfffffff0, 0)] * 248 if kind == "sp-max-table" else [])
    table(64, pairs, 1)
    if count == 2:
        table(65, [(0, 352)] if kind == "sp-first-wins" else mappings, 5)
    if kind in ("sp-backup", "sp-invalid-table"):
        image[64 * block + 56] ^= 1
    elif kind == "sp-table-overrun":
        struct.pack_into("<H", image, 64 * block + 48, 250)
        tag(64 * block)
    elif kind in ("sp-mapped-outside", "sp-mapped-sentinel", "sp-wrapped-target"):
        struct.pack_into("<I", image, 64 * block + 60, 0xffffffff if kind != "sp-mapped-outside" else len(image) // block)
        tag(64 * block)

    raw = bytearray(64)
    raw[:2] = b"\x02\x40"
    raw[5:28] = b"*UDF Sparable Partition"
    struct.pack_into("<H", raw, 28, 0x150)
    struct.pack_into("<HH", raw, 36, 1, 0)
    struct.pack_into("<H", raw, 40, packet)
    raw[42] = count
    struct.pack_into("<I", raw, 44, 2048 if kind == "sp-max-table" else 64)
    struct.pack_into("<II", raw, 48, 64, 65)
    if kind == "sp-declared-zero":
        struct.pack_into("<I", raw, 44, 0)
    elif kind == "sp-reserved":
        raw[2] = 7
    elif kind in ("sp-packet-zero", "sp-packet-unaligned"):
        struct.pack_into("<H", raw, 40, 0 if kind == "sp-packet-zero" else 3)
    elif kind == "sp-too-many-tables":
        raw[42] = 5
    elif kind == "sp-table-too-large":
        struct.pack_into("<I", raw, 44, 2049)
    for sector in (35, 51):
        pos = sector * block
        metadata_map = bytes(image[pos + 446:pos + 510]) if kind == "sp-metadata" else b""
        image[pos + 440:pos + 504] = raw
        image[pos + 504:pos + 504 + len(metadata_map)] = metadata_map
        struct.pack_into("<II", image, pos + 264, 64 + len(metadata_map), 2 if metadata_map else 1)
        struct.pack_into("<H", image, pos + 240, 0x250 if metadata_map else 0x150)
        tag(pos, 504 + len(metadata_map))
    return bytes(image), files
