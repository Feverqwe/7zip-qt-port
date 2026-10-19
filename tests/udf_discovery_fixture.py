"""Native UDF descriptors relocated for volume discovery and geometry checks."""
import binascii
import struct
from udf_fixture import volume as base_volume

CASES = ("ds-block-512", "ds-block-1024", "ds-block-2048", "ds-block-4096",
         "ds-block-4096-spaced", "ds-lvd-size", "ds-backup-last", "ds-backup-middle",
         "ds-bad-primary", "ds-open-512", "ds-last-minus-one", "ds-last-minus-two",
         "ds-last-minus-150", "ds-last-minus-152", "ds-main-reserve", "ds-no-terminator",
         "ds-stop-invalid", "ds-vdp", "ds-vdp-64", "ds-prevailing-new",
         "ds-prevailing-tie", "ds-prevailing-older", "ds-prevailing-lvd",
         "ds-partitions-growth", "ds-fsd-zero", "ds-fsd-large", "ds-nsr03",
         "ds-direct-nsr", "ds-vrs-type-version", "ds-lvid-max", "ds-lvid-chain",
         "ds-lvid-missing", "ds-lvid-cycle")
MODULE_ONLY = ("ds-lvid-invalid-last", "ds-lvid-short-use", "ds-lvid-partial")
BAD = ("ds-no-anchors", "ds-no-pvd", "ds-no-lvd", "ds-bad-lvd", "ds-vdp-65",
       "ds-vdp-cycle", "ds-vrs-invalid", "ds-vrs-no-nsr", "ds-lvid-new-read",
       "ds-lvid-chain-new", "ds-fsd-location", "ds-partition-wrap")


def volume(kind):
    original, files = base_volume("native")
    old_block = 2048
    block = 512 if kind == "ds-block-512" else 1024 if kind in ("ds-block-1024", "ds-lvd-size") else 4096 if kind.startswith("ds-block-4096") else 2048
    image = bytearray(800 * block)
    for sector in range(len(original) // old_block):
        data = original[sector * old_block:(sector + 1) * old_block]
        if struct.unpack_from("<H", data)[0] != 2:
            image[sector * block:sector * block + min(block, old_block)] = data[:block]

    def tag(pos, size=None):
        if size is None:
            size = 16 + struct.unpack_from("<H", image, pos + 10)[0]
        struct.pack_into("<HH", image, pos + 8,
                         binascii.crc_hqx(image[pos + 16:pos + size], 0), size - 16)
        image[pos + 4] = 0
        image[pos + 4] = sum(image[pos:pos + 16]) & 255

    def descriptor(source, target):
        data = original[source * old_block:(source + 1) * old_block]
        pos = target * block
        image[pos:pos + block] = data[:block] + bytes(max(0, block - old_block))
        struct.pack_into("<I", image, pos + 12, target)
        tag(pos)
        return pos

    for source, target in ((32, 80), (33, 81), (34, 82), (35, 83), (36, 84), (37, 85),
                           (48, 96), (49, 97), (50, 98), (51, 99), (52, 100), (53, 101)):
        descriptor(source, target)
    for sector in (82, 98):
        struct.pack_into("<I", image, sector * block + 192, 100)
        tag(sector * block)
    for sector in (83, 99):
        struct.pack_into("<I", image, sector * block + 212, 2048 if kind == "ds-lvd-size" else block)
        struct.pack_into("<II", image, sector * block + 432, 0, 0)
        tag(sector * block)
    for sector in range(257, 278):
        pos = sector * block
        if struct.unpack_from("<H", image, pos)[0] == 261:
            size = struct.unpack_from("<Q", image, pos + 56)[0]
            struct.pack_into("<Q", image, pos + 64, (size + block - 1) // block)
            tag(pos)
    struct.pack_into("<I", image, 263 * block + 180, 40)
    tag(263 * block)
    image[297 * block:297 * block + len(files["alpha.bin"])] = files["alpha.bin"]

    # Recognition descriptors always begin at byte 32768, independent of block size.
    stride = 4096 if kind == "ds-block-4096-spaced" else 2048
    image[32768:32768 + 3 * stride] = bytes(3 * stride)
    for index, ident in enumerate((b"BEA01", b"NSR03" if kind == "ds-nsr03" else b"NSR02", b"TEA01")):
        pos = 32768 + index * stride
        image[pos + 1:pos + 6] = ident
        image[pos + 6] = 1
    if kind == "ds-direct-nsr":
        image[32769:32774] = b"NSR02"
    elif kind == "ds-vrs-type-version":
        image[32768] = 42
        image[32774] = 9
    elif kind == "ds-vrs-invalid":
        image[32769:32774] = b"BAD00"
    elif kind == "ds-vrs-no-nsr":
        image[32768 + stride + 1:32768 + stride + 6] = b"TEA01"

    def anchor(sector, length=16 * block):
        pos = sector * block
        image[pos:pos + block] = bytes(block)
        struct.pack_into("<HH", image, pos, 2, 2)
        struct.pack_into("<I", image, pos + 12, sector)
        struct.pack_into("<IIII", image, pos + 16, length, 80, 16 * block, 96)
        tag(pos, 32)

    anchors = [256, 543, 799]
    for sector in anchors:
        anchor(sector)
    if kind in ("ds-backup-last", "ds-backup-middle", "ds-bad-primary", "ds-open-512",
                "ds-no-anchors") or kind.startswith("ds-last-minus-"):
        image[256 * block:257 * block] = bytes(block)
    if kind == "ds-bad-primary":
        anchor(256)
        image[256 * block + 4] ^= 1
    elif kind == "ds-backup-last" or kind.startswith("ds-last-minus-"):
        image[543 * block:544 * block] = bytes(block)
    elif kind == "ds-backup-middle":
        image[799 * block:800 * block] = bytes(block)
    elif kind in ("ds-open-512", "ds-no-anchors"):
        for sector in (543, 799):
            image[sector * block:(sector + 1) * block] = bytes(block)
        if kind == "ds-open-512":
            anchor(512)
    if kind.startswith("ds-last-minus-"):
        count = {"one": 1, "two": 2, "150": 150, "152": 152}[kind.rsplit("-", 1)[1]]
        image.extend(bytes(count * block))

    if kind in ("ds-main-reserve", "ds-no-pvd"):
        image[80 * block + 4] ^= 1
        if kind == "ds-no-pvd":
            image[96 * block + 4] ^= 1
    elif kind == "ds-no-lvd":
        descriptor(37, 83)
    elif kind == "ds-bad-lvd":
        struct.pack_into("<I", image, 83 * block + 264, block)
        tag(83 * block)
    elif kind == "ds-no-terminator":
        for sector in anchors:
            anchor(sector, 5 * block)
    elif kind == "ds-stop-invalid":
        image[84 * block + 4] ^= 1
    elif kind.startswith("ds-vdp"):
        descriptor(34, 288)
        descriptor(35, 289)
        struct.pack_into("<I", image, 288 * block + 192, 100)
        tag(288 * block)
        struct.pack_into("<I", image, 289 * block + 212, block)
        struct.pack_into("<II", image, 289 * block + 432, 0, 0)
        tag(289 * block)
        descriptor(37, 290)
        image[82 * block:84 * block] = bytes(2 * block)
        count = 65 if kind == "ds-vdp-65" else 64 if kind == "ds-vdp-64" else 1
        pointers = [81] + list(range(112, 112 + count - 1))
        for i, sector in enumerate(pointers):
            pos = sector * block
            image[pos:pos + block] = bytes(block)
            struct.pack_into("<HH", image, pos, 3, 2)
            struct.pack_into("<I", image, pos + 12, sector)
            target = 81 if kind == "ds-vdp-cycle" else pointers[i + 1] if i + 1 < count else 288
            struct.pack_into("<III", image, pos + 16, i + 1, 3 * block, target)
            tag(pos, 28)
    elif kind in ("ds-prevailing-new", "ds-prevailing-tie", "ds-prevailing-older"):
        image[400 * block:421 * block] = image[257 * block:278 * block]
        image[440 * block:440 * block + len(files["alpha.bin"])] = files["alpha.bin"]
        pos = descriptor(34, 85)
        struct.pack_into("<III", image, pos + 184, 1, 400, 100)
        struct.pack_into("<I", image, pos + 16, 9 if kind == "ds-prevailing-older" else 10 if kind == "ds-prevailing-tie" else 11)
        tag(pos)
        struct.pack_into("<I", image, 82 * block + 16, 10)
        tag(82 * block)
        descriptor(37, 86)
        poison = 440 if kind == "ds-prevailing-older" else 297
        image[poison * block:poison * block + len(files["alpha.bin"])] = b"X" * len(files["alpha.bin"])
    elif kind == "ds-prevailing-lvd":
        pos = descriptor(35, 85)
        struct.pack_into("<II", image, pos + 432, 0, 0)
        struct.pack_into("<I", image, pos + 252, 60)
        struct.pack_into("<I", image, pos + 16, 11)
        tag(pos)
        struct.pack_into("<I", image, 83 * block + 16, 10)
        tag(83 * block)
        image[317 * block:318 * block] = image[257 * block:258 * block]
        struct.pack_into("<I", image, 317 * block + 12, 60)
        tag(317 * block)
        image[257 * block + 4] ^= 1
        descriptor(37, 86)
    elif kind == "ds-partitions-growth":
        pos = descriptor(35, 81)
        struct.pack_into("<I", image, pos + 212, block)
        struct.pack_into("<II", image, pos + 432, 0, 0)
        tag(pos)
        for number in range(1, 44):
            pos = descriptor(34, 82 + number)
            struct.pack_into("<H", image, pos + 22, number)
            struct.pack_into("<II", image, pos + 188, 0xfffffffe, 5)
            tag(pos)
        descriptor(37, 126)
        for sector in anchors:
            anchor(sector, 47 * block)
    elif kind in ("ds-fsd-zero", "ds-fsd-large"):
        for sector in (83, 99):
            struct.pack_into("<I", image, sector * block + 248, 0 if kind == "ds-fsd-zero" else 0x3fffffff)
            tag(sector * block)
    elif kind == "ds-fsd-location":
        struct.pack_into("<I", image, 257 * block + 12, 1)
        tag(257 * block)
    elif kind == "ds-partition-wrap":
        for sector in (82, 98):
            struct.pack_into("<II", image, sector * block + 188, 0xfffffff0, 100)
            tag(sector * block)

    if kind.startswith("ds-lvid-"):
        def lvid(sector, revision, next_sector=0, invalid=False, short=False):
            pos = sector * block
            image[pos:pos + block] = bytes(block)
            struct.pack_into("<HH", image, pos, 9, 2)
            struct.pack_into("<I", image, pos + 12, sector)
            struct.pack_into("<I", image, pos + 28, 1)
            struct.pack_into("<II", image, pos + 32, block if next_sector else 0, next_sector)
            struct.pack_into("<II", image, pos + 72, 0xffffffff if invalid else 1, 0 if short else 46)
            struct.pack_into("<II", image, pos + 80, 0xffffffff, 100)
            struct.pack_into("<HHH", image, pos + 128, revision, 0x300 if kind == "ds-lvid-max" else 0x102, 0x300)
            tag(pos, 134)
        for sector in (83, 99):
            struct.pack_into("<II", image, sector * block + 432, 1 if kind == "ds-lvid-partial" else block, 128)
            tag(sector * block)
        if kind != "ds-lvid-missing":
            next_sector = 129 if kind in ("ds-lvid-chain", "ds-lvid-chain-new", "ds-lvid-invalid-last") else 128 if kind == "ds-lvid-cycle" else 0
            lvid(128, 0x300 if kind in ("ds-lvid-new-read", "ds-lvid-chain", "ds-lvid-short-use") else 0x260 if kind == "ds-lvid-max" else 0x102,
                 next_sector, short=kind == "ds-lvid-short-use")
            if next_sector == 129:
                lvid(129, 0x300 if kind == "ds-lvid-chain-new" else 0x102, invalid=kind == "ds-lvid-invalid-last")
            elif kind == "ds-lvid-partial":
                lvid(129, 0x300)
    return bytes(image), files
