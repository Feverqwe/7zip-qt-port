"""Linux MSDOS subtype labels with independently declared partition spans."""
import json
from pathlib import Path
import struct
import sys

CASES = ("freebsd", "freebsd-relative", "netbsd", "openbsd", "bsd-count-short",
         "bsd-count-large", "bsd-unused", "bsd-outside", "bsd-badmagic",
         "solaris-old", "solaris-new", "solaris-8", "solaris-badversion",
         "solaris-badmagic", "solaris-outside-parent", "solaris-clamp",
         "unixware", "unixware-count-small", "unixware-clamp", "unixware-outside",
         "unixware-badmagic", "minix", "minix-first-type", "minix-signature",
         "minix-outside-parent", "combined", "with-ebr")


def volume(kind):
    sector = 4096 if kind.endswith("-4kn") else 512
    base = kind.removesuffix("-4kn")
    image = bytearray().join(bytes([i % 251]) * 512 for i in range(256 * sector // 512))
    image[:512] = bytes(512)
    image[510:512] = b"\x55\xaa"
    expected = []

    def entry(offset, typ, start, count):
        image[offset:offset + 16] = bytes(16)
        image[offset + 4] = typ
        struct.pack_into("<II", image, offset + 8, start, count)

    def install(slot, typ, start, count, variant):
        entry(446 + slot * 16, typ, start, count)
        parent = start * sector // 512
        parent_size = count * sector // 512
        child = parent + (5 if sector != 512 else 8)
        child_size = 3 if sector != 512 else 4
        found = []
        if typ in (0xa5, 0xa6, 0xa9):
            label = (parent + 1) * 512
            image[label:label + 512] = bytes(512)
            struct.pack_into("<I", image, label, 0 if variant == "bsd-badmagic" else 0x82564557)
            struct.pack_into("<I", image, label + 132, 0x82564557)
            count_label = 1 if variant == "bsd-count-short" else 65535 if variant == "bsd-count-large" else 4
            struct.pack_into("<H", image, label + 138, count_label)
            relative = variant == "freebsd-relative"

            def part(index, offset, size, fs=7):
                struct.pack_into("<III", image, label + 148 + index * 16, size, offset, 0)
                image[label + 148 + index * 16 + 12] = fs

            part(0, child - parent if relative else child, child_size,
                 0 if variant == "bsd-unused" else 7)
            part(2, 0 if relative else parent, parent_size)
            if variant == "bsd-outside":
                part(0, parent - 1, 4)
                part(1, parent + parent_size - 2, 4)
                part(3, child, child_size)
            elif variant == "bsd-unused":
                part(1, child + 8, 2)
                found = [(child + 8, 2)]
            else:
                found = [(child, child_size)]
            if variant == "bsd-count-large":
                part(15, parent + 40, 2)
                found += [(parent + 40, 2)]
            if variant == "bsd-outside":
                found = [(child, child_size)]
            if variant == "bsd-badmagic":
                found = []
        elif typ in (0x82, 0xbf):
            label = (parent + 1) * 512
            image[label:label + 512] = bytes(512)
            struct.pack_into("<II", image, label + 12,
                             0 if variant == "solaris-badmagic" else 0x600ddeee,
                             2 if variant == "solaris-badversion" else 1)
            struct.pack_into("<HH", image, label + 28, 512, 8 if variant == "solaris-8" else 16)

            def part(index, start, size):
                struct.pack_into("<HHII", image, label + 72 + index * 12, 0, 0, start, size)

            part(0, child - parent, child_size)
            part(10, 32, 2)
            found = [(child, child_size)] + ([] if variant == "solaris-8" else [(parent + 32, 2)])
            if variant in ("solaris-outside-parent", "solaris-clamp"):
                end = 254 if variant == "solaris-clamp" else parent + parent_size + 4
                part(10, end - parent, 8 if variant == "solaris-clamp" else 4)
                found = [(child, child_size), (end, 2 if variant == "solaris-clamp" else 4)]
            if variant in ("solaris-badmagic", "solaris-badversion"):
                found = []
        elif typ == 0x63:
            label = (parent + 29) * 512
            image[label:label + 512] = bytes(512)
            struct.pack_into("<I", image, label + 4,
                             0 if variant == "unixware-badmagic" else 0xca5e600d)
            struct.pack_into("<I", image, label + 156, 0x600ddeee)
            struct.pack_into("<H", image, label + 172, 1 if variant == "unixware-count-small" else 16)

            def part(index, start, size):
                struct.pack_into("<HHII", image, label + 216 + index * 12, 1, 0, start, size)

            part(0, 0, len(image) // 512)
            part(1, child, child_size)
            last = 255 if variant == "unixware-clamp" else 270 if variant == "unixware-outside" else parent + 40
            part(15, last, 8 if variant == "unixware-clamp" else 2)
            found = [(child, child_size)]
            if last < len(image) // 512:
                found += [(last, 1 if variant == "unixware-clamp" else 2)]
            if variant == "unixware-badmagic":
                found = []
        elif typ == 0x81:
            label = parent * 512
            image[label:label + 512] = bytes(512)
            image[label + 510:label + 512] = b"\x55\xaa"
            entry(label + 446, 0x83 if variant == "minix-first-type" else 0x81, child, child_size)
            entry(label + 462, 0x83, parent + 12, 2)
            last = parent + parent_size + 4 if variant == "minix-outside-parent" else parent + 16
            entry(label + 478, 0x81, last, 3)
            found = [(child, child_size), (last, 3)]
            if variant == "minix-signature":
                image[label + 510] = 0
            if variant in ("minix-signature", "minix-first-type"):
                found = []
        expected.extend(found)

    if base == "combined":
        for slot, typ in enumerate((0xa5, 0xbf, 0x63, 0x81)):
            install(slot, typ, (8, 64, 128, 192)[slot], 48, "combined")
    elif base == "with-ebr":
        entry(446, 5, 8, 48)
        image[8 * 512:9 * 512] = bytes(512)
        image[8 * 512 + 510:8 * 512 + 512] = b"\x55\xaa"
        entry(8 * 512 + 446, 0x83, 2, 4)
        expected.append((10, 4))
        install(1, 0xa5, 64, 48, "freebsd")
    else:
        typ = 0xa6 if base == "openbsd" else 0xa9 if base == "netbsd" else 0xa5
        if base.startswith("solaris"):
            typ = 0x82 if base == "solaris-old" else 0xbf
        elif base.startswith("unixware"):
            typ = 0x63
        elif base.startswith("minix"):
            typ = 0x81
        install(0, typ, 16, 160, base)
    parts = [dict(offset=start * 512, size=size * 512,
                  payload=bytes(image[start * 512:(start + size) * 512]).hex())
             for start, size in expected]
    return bytes(image), parts, sector


if __name__ == "__main__":
    image, parts, _ = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps(parts))
