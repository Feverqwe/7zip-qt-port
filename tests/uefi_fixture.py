"""EFI capsule headers, Quark wrappers and existing firmware-volume consumers."""
from pathlib import Path
import json
import lzma
import struct
import sys
import uuid

GUIDS = tuple(bytes.fromhex(s) for s in (
    "bd86663b760d3040b70eb5519e2fc5a0", "8ba63c4a2377fb48803d578cc1fec44d",
    "b9829153b5ab9143b69ae3a943f72fcc"))
FMP = uuid.UUID("6dcbd5ed-e82d-4c44-bda1-7194199ad92a").bytes_le
CASES = ("generic", "fmp", "zero-guid", "padding", "flags", "empty", "short", "standard",
         "legacy1", "legacy2", "split", "quark", "quark-fv", "fv-capsule", "lzma-capsule",
         "me-capsule", "fv", "fv-lzma", "generic-quark-prefix",
         "generic-quark-prefix-large", "quark-ambiguous")
BAD = ("short-header", "header-small", "image-small", "image-span", "legacy-short",
       "v2-short", "body-before", "body-after", "quark-short", "quark-chain",
       "fv-checksum", "fv-span", "me-span")


def firmware(payload, count=1, compressed=False):
    body = bytearray(72)
    for i in range(count):
        body += bytes((-len(body)) % 8)
        content = payload
        typ = 1
        if compressed:
            plain = (len(payload) + 4).to_bytes(3, "little") + b"\x19" + payload
            data = lzma.compress(plain, format=lzma.FORMAT_ALONE)
            data = data[:5] + struct.pack("<Q", len(plain)) + data[13:]
            content = (len(data) + 9).to_bytes(3, "little") + b"\x01" + struct.pack("<IB", len(plain), 2) + data
            typ = 2
        h = bytearray(uuid.UUID(f"{0x11223344+i:08x}-5566-7788-99aa-bbccddeeff00").bytes_le)
        h += bytes((0, 0xaa, typ, 0)) + (24 + len(content)).to_bytes(3, "little") + b"\xfb"
        h[16] = (-(sum(h) - h[17] - h[23])) & 255
        body += h + content
    body += b"\xff" * ((-len(body)) % 512)
    body[16:32] = uuid.UUID("8c8ce578-8a3d-4f1c-9935-896185c32dd3").bytes_le
    struct.pack_into("<QIIHHHBB", body, 32, len(body), 0x4856465f, 0x800, 72, 0, 0, 0, 2)
    struct.pack_into("<IIII", body, 56, 1, len(body), 0, 0)
    struct.pack_into("<H", body, 50, (-sum(struct.unpack("<36H", body[:72]))) & 0xffff)
    return bytes(body)


def volume(kind):
    payload = (b"Capsule payload\0" + bytes(range(256))) * 19
    body = payload
    target = "payload.bin"
    header = 28
    guid = FMP
    offset = header
    prefix = b""
    if kind == "empty": body = payload = b""
    if kind == "short": body = payload = b"x"
    if kind == "fmp": body = payload = struct.pack("<IHHQ", 1, 0, 1, 16) + payload
    if kind == "zero-guid": guid = bytes(16)
    if kind in ("generic-quark-prefix", "generic-quark-prefix-large"):
        guid = struct.pack("<I", 0x5f435348) + FMP[4:]
        body = bytearray(40 if kind == "generic-quark-prefix" else 2048)
        struct.pack_into("<I", body, 4, 1024)
        if kind == "generic-quark-prefix":
            struct.pack_into("<I", body, 24, 1)
        payload = bytes(body)
    if kind == "padding": header = offset = 129
    if kind == "standard": guid = GUIDS[2]
    if kind in ("legacy1", "split", "legacy-short", "body-before", "body-after"):
        header, offset, guid = 80, 80, GUIDS[0]
    if kind in ("legacy2", "v2-short"):
        header, offset, guid = 32, 32, GUIDS[1]
    if kind in ("fv", "fv-lzma", "fv-capsule", "lzma-capsule", "quark-fv", "fv-checksum", "fv-span", "many-fv"):
        body = firmware(payload, count=2048 if kind == "many-fv" else 1,
                        compressed=kind in ("fv-lzma", "lzma-capsule"))
        target = "11223344.raw" if kind in ("fv-lzma", "lzma-capsule") else "11223344"
        guid = GUIDS[2]
        if kind == "fv-checksum": body = body[:50] + b"\x12\x34" + body[52:]
        if kind == "fv-span": body = body[:32] + struct.pack("<Q", len(body) + 512) + body[40:]
    if kind in ("me-capsule", "me-span"):
        data = bytearray(4096)
        data[:16] = b"\xff" * 16
        struct.pack_into("<II", data, 16, 0x0ff0a55a, 0x100000)
        struct.pack_into("<8I", data, 256, 0x00010001 if kind == "me-span" else 0,
                         *([0xfff] * 7))
        body = payload = bytes(data)
        guid, target = GUIDS[2], "Descriptor"
    image = bytearray(header)
    image[:16] = guid
    struct.pack_into("<III", image, 16, header, 0xdead0000 if kind == "flags" else 0x30000,
                     header + len(body))
    if guid == GUIDS[0]:
        struct.pack_into("<II", image, 48, 1 if kind == "split" else 0, offset)
    if guid == GUIDS[1]: struct.pack_into("<H", image, 28, offset)
    image += body
    if kind in ("quark", "quark-fv", "quark-short", "quark-chain", "quark-high", "quark-ambiguous"):
        prefix = bytearray(1024)
        struct.pack_into("<III", prefix, 0, 0x5f435348, 7, 0)
        struct.pack_into("<I", prefix, 32, 1024)
        if kind == "quark-ambiguous":
            struct.pack_into("<III", prefix, 16, 28, 0, 64)
        if kind == "quark-chain": struct.pack_into("<I", prefix, 52, 1)
        image = prefix + image
    meta = dict(payload=payload.hex(), target=target, index=0,
                format="UEFIf" if kind.startswith("fv") and kind in ("fv", "fv-lzma") else "UEFIc",
                data_offset=len(prefix) + (header + 72 + 24 if target == "11223344" else header))
    if kind in ("fv", "fv-lzma"):
        image = bytearray(body)
        meta["data_offset"] = 96
    if kind in ("high-header", "quark-high"):
        payload = b"last 15 bytes!!"
        image = bytearray(prefix) + bytearray(FMP + struct.pack("<III", 0xfffffff0, 0, 0xffffffff)) + payload
        meta.update(payload=payload.hex(), gap_start=str(len(prefix) + 28),
                    gap_size=str(0xfffffff0 - 28), data_offset=str(len(prefix) + 0xfffffff0))
    if kind == "large-fv-padding":
        body = firmware(payload)
        image = FMP + struct.pack("<III", 1 << 30, 0, (1 << 30) + len(body)) + body
        meta.update(payload=body.hex(), gap_start="28", gap_size=str((1 << 30) - 28),
                    data_offset=str(1 << 30))
        image = bytearray(image)
    if kind == "short-header": image = image[:27]
    if kind == "header-small": struct.pack_into("<I", image, 16, 27)
    if kind == "image-small": struct.pack_into("<I", image, 24, 27)
    if kind == "image-span": struct.pack_into("<I", image, 24, 0xffffffff)
    if kind == "legacy-short": image = image[:40]
    if kind == "v2-short": struct.pack_into("<I", image, 16, 28)
    if kind == "body-before": struct.pack_into("<I", image, 52, 79)
    if kind == "body-after": struct.pack_into("<I", image, 52, len(image) + 1)
    if kind == "quark-short": image = image[:1040]
    return bytes(image), meta


if __name__ == "__main__":
    image, metadata = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps(metadata))
