"""ELF class/endian records and Linux core extended numbering fixtures."""
from pathlib import Path
import json
import struct
import sys

CASES = ("32le", "32be", "64le", "64be", "padding", "unaligned", "ehsize",
         "unused-geometry", "short32", "null-data", "zero-offset", "sections32le", "sections32be",
         "sections64le", "sections64be", "no-names", "large-entsize", "section-info", "tail")
BAD = ("magic", "class", "byte-order", "version", "ph-entry", "ph-count", "ph-offset",
       "short-header", "short-table", "short-data", "segment-wrap", "memory-wrap", "memory-short",
       "section-wrap", "sh-entry", "names-index", "names-type", "names-end", "name-offset",
       "section-zero", "xnum-missing", "xnum-small")
NATIVE = ("native-normal", "native-padding", "native-unaligned", "native-ehsize", "native-null")


def native_volume(kind):
    phoff = 65 if kind == "native-unaligned" else 64
    code = bytes.fromhex("b83c000000bf2a0000000f05")  # x86-64 exit(42).
    image = bytearray(256 + len(code))
    ident = b"\x7fELF\x02\x01\x01\0\0" + (b"padding" if kind == "native-padding" else bytes(7))
    image[:64] = ident + struct.pack("<HHIQQQIHHHHHH", 2, 62, 1, 0x400100, phoff, 0,
                                    0, 0 if kind == "native-ehsize" else 64, 56,
                                    2 if kind == "native-null" else 1, 0, 0, 0)
    image[phoff:phoff + 56] = struct.pack("<IIQQQQQQ", 1, 5, 0, 0x400000, 0x400000,
                                        len(image), len(image), 4096)
    if kind == "native-null":
        image[phoff + 56:phoff + 112] = struct.pack("<IIQQQQQQ", 0, 0, (1 << 64) - 1,
                                                  0, 0, (1 << 64) - 1, 0, 0)
    image[256:] = code
    return bytes(image), dict(payload=bytes(image).hex(), target="0", index=0,
                             count=2 if kind == "native-null" else 1,
                             bss=None, data_offset=0)


def volume(kind):
    if kind.startswith("native-"):
        return native_volume(kind)
    bits = 32 if "32" in kind else 64
    endian = ">" if kind.endswith("be") else "<"
    header_size, phsize, shsize = (52, 32, 40) if bits == 32 else (64, 56, 64)
    maximum = (1 << bits) - 1
    payload = (b"ELF payload\0" + bytes(range(256))) * 19
    section_mode = kind.startswith("sections") or kind in (
        "no-names", "large-entsize", "section-wrap", "sh-entry", "names-index", "names-type",
        "names-end", "name-offset", "section-zero", "sections-xnum", "section-info")
    extended_core = kind in ("core-xnum", "xnum-small")
    if kind == "sections-xnum":
        bits, endian, header_size, phsize, shsize, maximum = 32, ">", 52, 32, 40, 0xffffffff

    def ph(typ, offset, size, virtual=None, address=0, flags=4):
        virtual = size if virtual is None else virtual
        if bits == 64:
            return struct.pack(endian + "IIQQQQQQ", typ, flags, offset, address, address,
                               size, virtual, 4096)
        return struct.pack(endian + "IIIIIIII", typ, offset, address, address, size, virtual,
                           flags, 4096)

    def sh(typ=0, offset=0, size=0, name=0, link=0, info=0, entsize=0):
        return struct.pack(endian + ("IIQQQQIIQQ" if bits == 64 else "IIIIIIIIII"),
                           name, typ, 0, 0, offset, size, link, info, 0 if typ == 0 else 1, entsize)

    phoff, shoff, phnum, shnum, names_index = header_size, 0, 3, 0, 0
    target, target_index, bss_index, count = "0", 0, None, 3
    metadata = {}
    if section_mode:
        phoff, phnum = 0, 0
        shnum = 65538 if kind == "sections-xnum" else 4
        shoff = header_size
        target_index = shnum - 2 if kind == "sections-xnum" else 1
        names_index = shnum - 1
        target = ".payload"
        if kind == "section-info":
            # sh_info is a symbol boundary for SYMTAB, not a section index.
            payload = bytes(24) + struct.pack(endian + "IBBHQQ", 1, 0x10, 0, 1, 0, 0)
        bss_index = 2 if shnum == 4 else None
        dataoff = shoff + shnum * shsize
        names = b"\0.payload\0.bss\0.shstrtab\0"
        table = bytearray(shnum * shsize)
        table[target_index * shsize:(target_index + 1) * shsize] = sh(
            2 if kind == "section-info" else 1, dataoff, len(payload), 1,
            link=names_index if kind == "section-info" else 0,
            info=1 if kind == "section-info" else 0,
            entsize=24 if kind == "section-info" else 1 << 40 if kind == "large-entsize" else 0)
        if bss_index is not None:
            table[bss_index * shsize:(bss_index + 1) * shsize] = sh(8, maximum, 0x12345, 10)
        table[names_index * shsize:(names_index + 1) * shsize] = sh(
            3, dataoff + len(payload), len(names), 15)
        if kind == "sections-xnum":
            table[:shsize] = sh(size=shnum, link=names_index)
        if kind == "no-names":
            names_index, target = 0, str(target_index)
        if kind == "section-wrap":
            table[target_index * shsize:(target_index + 1) * shsize] = sh(1, maximum - 3, 8, 1)
        if kind == "names-type":
            table[names_index * shsize:(names_index + 1) * shsize] = sh(
                1, dataoff + len(payload), len(names), 15)
        if kind == "name-offset":
            table[target_index * shsize:(target_index + 1) * shsize] = sh(
                1, dataoff, len(payload), len(names))
        if kind == "names-end":
            names = names[:-1] + b"!"
        if kind == "section-zero":
            table[:shsize] = sh(1)
        body = bytes(table) + payload + names
        count = shnum
    elif extended_core:
        phnum, shnum = 65536, 1
        target_index, target = phnum - 1, str(phnum - 1)
        dataoff = phoff + phnum * phsize
        table = bytes(phsize * (phnum - 1)) + ph(1, dataoff, len(payload))
        shoff = dataoff + len(payload)
        body = table + payload + sh(size=1, info=4 if kind == "xnum-small" else phnum)
        count = phnum
    elif kind == "short32":
        phoff, phnum, count, target = 0, 0, 0, None
        body, payload = b"", b""
    else:
        if kind == "unaligned":
            phoff += 1
        dataoff = phoff + phnum * phsize
        second = b"second segment\0" * 97
        table = ph(1, dataoff, len(payload)) + ph(1, dataoff + len(payload), len(second))
        table += ph(0 if kind == "null-data" else 0x6474e551,
                    maximum if kind in ("null-data", "zero-offset") else 0,
                    maximum if kind == "null-data" else 0, virtual=0)
        if kind == "segment-wrap":
            table = ph(1, maximum - 3, 8) + table[phsize:]
        elif kind == "memory-wrap":
            table = ph(1, dataoff, len(payload), address=maximum - 3) + table[phsize:]
        elif kind == "memory-short":
            table = ph(1, dataoff, len(payload), virtual=1) + table[phsize:]
        body = bytes(phoff - header_size) + table + payload + second

    ident = b"\x7fELF" + bytes((2 if bits == 64 else 1, 2 if endian == ">" else 1, 1, 0, 0)) + bytes(7)
    if kind == "padding":
        ident = ident[:9] + bytes(range(1, 8))
    declared_size = 0 if kind == "ehsize" else header_size
    raw_phnum = 65535 if extended_core or kind == "xnum-missing" else phnum
    raw_shnum = 0 if kind == "sections-xnum" else shnum
    raw_names = 65535 if kind == "sections-xnum" else names_index
    header = ident + struct.pack(endian + ("HHIQQQIHHHHHH" if bits == 64 else "HHIIIIIHHHHHH"),
                                 1 if section_mode or kind == "short32" else 4 if extended_core else 2,
                                 62 if bits == 64 else 3, 1, 0, phoff, shoff, 0, declared_size,
                                 phsize if phnum else 0, raw_phnum,
                                 shsize if shnum else 123 if kind == "unused-geometry" else 0,
                                 raw_shnum, raw_names)
    image = header + body
    if kind in ("magic", "class", "byte-order", "version"):
        offset = {"magic": 0, "class": 4, "byte-order": 5, "version": 6}[kind]
        image = image[:offset] + b"\0" + image[offset + 1:]
    elif kind in ("ph-entry", "ph-count", "ph-offset", "sh-entry", "names-index"):
        offset = {"ph-entry": 54, "ph-count": 56, "ph-offset": 32,
                  "sh-entry": 58, "names-index": 62}[kind]
        patch = struct.pack("<Q", maximum) if kind == "ph-offset" else struct.pack(
            "<H", 65534 if kind == "ph-count" else 4 if kind == "names-index" else 1)
        image = image[:offset] + patch + image[offset + len(patch):]
    elif kind == "short-header":
        image = image[:51]
    elif kind == "short-table":
        image = image[:header_size + phsize - 1]
    elif kind == "short-data":
        image = image[:-1]
    elif kind == "tail":
        image += b"trailing overlay"
    elif kind == "high-offset":
        gap = (1 << 61) - phoff
        adjusted = bytearray(image)
        struct.pack_into("<Q", adjusted, 32, phoff + gap)
        for record in range(phnum):
            offset = phoff + record * phsize + 8
            original = struct.unpack_from("<Q", adjusted, offset)[0]
            if original:
                struct.pack_into("<Q", adjusted, offset, original + gap)
        image = bytes(adjusted)
        metadata["gap_size"] = str(gap)
    metadata.update(payload=payload.hex(), target=target, index=target_index,
                    count=count, bss=bss_index, data_offset=dataoff if count else 0)
    return image, metadata


if __name__ == "__main__":
    image, metadata = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps(metadata))
