"""PE/COFF file spans, debug records and resource projections."""
from pathlib import Path
import json
import struct
import sys

CASES = ("pe32", "pe64", "late-header", "unaligned", "no-dirs",
         "headers-only", "debug-empty-first", "certificate", "resource", "bitmap",
         "strings", "version", "coff", "coff-reloc", "reloc-no-dir", "tail",
         "pe32-certificate", "late-certificate", "checksum")
BAD = ("magic", "signature", "pe-offset", "opt-magic", "opt-size", "directory-count",
       "headers-size", "short-table", "short-data", "section-wrap", "cert-span",
       "cert-wrap", "debug-span", "debug-wrap", "symbol-span", "symbol-strings")


def volume(kind):
    coff = kind.startswith("coff")
    bits = 32 if kind.startswith("pe32") else 64
    peoff = 0x2345 if kind.startswith("late-") else 65 if kind == "unaligned" else 64
    minimum = 96 if bits == 32 else 112
    directories = 0 if kind in ("no-dirs", "reloc-no-dir") else 16
    optsize = 0 if coff else minimum + directories * 8
    count = 2048 if kind == "many-sections" else 2 if kind == "high-offset" else 0 if kind == "headers-only" else 3
    optstart = 20 if coff else peoff + 24
    table = optstart + optsize
    dataoff = (table + count * 40 + 511) & ~511
    payload = (b"PE payload\0" + bytes(range(256))) * 19
    if kind == "checksum":
        payload *= 14
    second = b"second section\0" * 37
    target, index = ".text", 0
    dirs = bytearray(directories * 8)
    extra = b""
    name = b".reloc" if kind in ("coff-reloc", "reloc-no-dir") else b".text"
    target = name.decode()
    if kind in ("resource", "bitmap", "strings", "version"):
        name = b".rsrc"
        restype = {"resource": 10, "bitmap": 2, "strings": 6, "version": 16}[kind]
        if kind == "bitmap":
            data = struct.pack("<IiiHHIIIIII", 40, 1, 1, 1, 24, 0, 4, 0, 0, 0, 0) + b"RGB\0"
            payload = struct.pack("<2sIII", b"BM", 14 + len(data), 0, 54) + data
        elif kind == "strings":
            data = struct.pack("<H", 2) + "Hi".encode("utf-16le") + bytes(30)
            payload = "0\tHi\r\n".encode("utf-16le")
        elif kind == "version":
            key = "VS_VERSION_INFO\0".encode("utf-16le")
            fixed = struct.pack("<13I", 0xfeef04bd, 0x10000, 0x10002, 0x30004,
                                0x10002, 0x30004, 0x3f, 0, 0x40004, 1, 0, 0, 0)
            data = struct.pack("<HHH", 6 + len(key) + 2 + len(fixed), len(fixed), 0) + key + bytes(2) + fixed
            payload = ("FILEVERSION    1,2,3,4\r\nPRODUCTVERSION 1,2,3,4\r\n"
                       "FILEFLAGSMASK  0x3F\r\nFILEFLAGS      0x0\r\nFILEOS         VOS_NT_WINDOWS32\r\n"
                       "FILETYPE       VFT_APP\r\nFILESUBTYPE    0x0\r\n{\r\n}\r\n").encode("utf-16le")
        else:
            data = payload
        resource = bytearray(88)
        for off, ident, dest in ((0, restype, 24 | 0x80000000), (24, 1, 48 | 0x80000000), (48, 1033, 72)):
            struct.pack_into("<H", resource, off + 14, 1)
            struct.pack_into("<II", resource, off + 16, ident, dest)
        struct.pack_into("<IIII", resource, 72, 0x1000 + 88, len(data), 0, 0)
        first = bytes(resource) + data
        target = {"resource": ".rsrc/RCDATA/1", "bitmap": ".rsrc/BITMAP/1.bmp",
                  "strings": ".rsrc/string.txt", "version": ".rsrc/version.txt"}[kind]
    else:
        first = payload
    if kind in ("debug-empty-first", "debug-span", "debug-wrap"):
        first = bytes(28) + struct.pack("<IIIIIII", 0, 1, 0, 2, 37, 0, dataoff + 56 + len(second))
        extra = b"debug payload\0" + bytes(23)
        struct.pack_into("<II", dirs, 6 * 8, 0x1000, len(first))
        if kind in ("debug-span", "debug-wrap"):
            first = first[:52] + struct.pack("<I", 0xfffffff0 if kind == "debug-wrap" else 0x100000)
        payload, target, index = extra, ".debug1", 3
    if kind.endswith("certificate") or kind in ("cert-span", "cert-wrap"):
        extra = struct.pack("<IHH", 16, 0x200, 2) + b"\x30\x02\x05\0PAD!"
        struct.pack_into("<II", dirs, 4 * 8, dataoff + len(first) + len(second), len(extra))
        payload, target, index = extra, "CERTIFICATE", 3
        if kind in ("cert-span", "cert-wrap"):
            struct.pack_into("<II", dirs, 4 * 8, 0xfffffff0 if kind == "cert-wrap" else 0x100000, len(extra))
    image = bytearray(dataoff)
    if not coff:
        image[:2] = b"MZ"
        struct.pack_into("<I", image, 60, peoff)
        image[peoff:peoff + 4] = b"PE\0\0"
        struct.pack_into("<H", image, optstart, 0x10b if bits == 32 else 0x20b)
        struct.pack_into("<II", image, optstart + 32, 4096, 512)
        struct.pack_into("<II", image, optstart + 56, 0x8000, dataoff)
        struct.pack_into("<H", image, optstart + 68, 10)
        struct.pack_into("<I", image, optstart + minimum - 4, directories)
        image[optstart + minimum:optstart + optsize] = dirs
    symboloff = dataoff + len(first) + len(second) + len(extra)
    struct.pack_into("<HHIIIHH", image, 0 if coff else peoff + 4,
                     0x14c if bits == 32 else 0x8664, count, 0,
                     symboloff if coff else 0, 1 if coff else 0, optsize, 2)
    for i in range(count):
        offset = dataoff if i == 0 else dataoff + len(first) if i == 1 else 0xffffffff
        raw = first if i == 0 else second if i == 1 and kind != "high-offset" else b""
        section_name = name if i == 0 else b".data" if i == 1 and kind != "high-offset" else b".bss"
        va = 0 if kind in ("coff-reloc", "reloc-no-dir") else 0x1000 * (i + 1)
        struct.pack_into("<8sIIIIIIHHI", image, table + i * 40,
                         section_name, len(raw) if raw else 0x12345, va, len(raw), offset,
                         0, 0, 0, 0, 0x40000040)
    if count:
        image += first + (b"" if kind == "high-offset" else second) + extra
    else:
        payload, target, index = b"", None, 0
    if coff:
        image += bytes(18) + struct.pack("<I", 4)
    if kind == "checksum":
        padded = bytes(image) + bytes(len(image) % 2)
        checksum = sum(struct.unpack("<" + "H" * (len(padded) // 2), padded))
        checksum = (checksum & 0xffff) + (checksum >> 16)
        checksum = (checksum & 0xffff) + (checksum >> 16)
        struct.pack_into("<I", image, optstart + 64, (checksum + len(image)) & 0xffffffff)
    meta = dict(payload=payload.hex(), target=target, index=index, data_offset=dataoff,
                table_offset=table, pe_offset=peoff, opt_start=optstart, second=second.hex())
    if kind == "high-offset":
        gap = 0xfffffff0 - dataoff
        struct.pack_into("<I", image, table + 20, dataoff + gap)
        meta.update(gap_start=str(dataoff), gap_size=str(gap), data_offset=str(dataoff + gap))
    if kind == "tail":
        image += b"overlay bytes"
    if kind in ("symbol-span", "symbol-strings"):
        offset = len(image) if kind == "symbol-strings" else 0xfffffff0
        struct.pack_into("<II", image, peoff + 12, offset, 1)
        if kind == "symbol-strings":
            image += bytes(18) + struct.pack("<I", 3)
    if kind == "magic": image[0] = 0
    if kind == "signature": image[peoff] = 0
    if kind == "pe-offset": struct.pack_into("<I", image, 60, 0xfffffff0)
    if kind == "opt-magic": struct.pack_into("<H", image, optstart, 0)
    if kind == "opt-size": struct.pack_into("<H", image, peoff + 20, 95)
    if kind == "directory-count": struct.pack_into("<I", image, optstart + minimum - 4, 17)
    if kind == "headers-size": struct.pack_into("<I", image, optstart + 60, len(image) + 1)
    if kind == "short-table": image = image[:table + 39]
    if kind == "short-data": image = image[:-1]
    if kind == "section-wrap": struct.pack_into("<II", image, table + 16, len(first), 0xfffffff0)
    return bytes(image), meta


if __name__ == "__main__":
    image, metadata = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps(metadata))
