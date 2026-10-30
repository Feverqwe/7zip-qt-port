"""Disposable TAR records exercising physical layout and SDK extensions."""
import io
import json
from pathlib import Path
import sys
import tarfile

CASES = ("ustar", "nul-type", "prefix", "gnu", "pax", "base256", "signed",
         "sparse", "sparse-extended", "symlink", "hardlink", "directory",
         "linkdata-hardlink", "linkdata-directory", "large-pax", "long-zero-tail")
BAD = ("checksum", "late-checksum", "short-header", "short-payload", "huge-size",
       "huge-pax", "huge-longname", "sparse-overlap", "sparse-size")
PAYLOAD = bytes(range(256)) * 3 + b"TAR payload\x00\xff"


def checksum(header, signed=False):
    header = bytearray(header)
    header[148:156] = b" " * 8
    total = sum(x if x < 128 or not signed else x - 256 for x in header)
    header[148:156] = f"{total:06o}\0 ".encode()
    return bytes(header)


def octal(number, size=12):
    return f"{number:0{size - 1}o}\0".encode()


def header(name="payload.bin", size=0, flag=b"0", link=""):
    info = tarfile.TarInfo(name)
    info.size, info.type, info.linkname = size, flag, link
    info.mode, info.mtime = 0o644, 1700000000
    return info.tobuf(tarfile.USTAR_FORMAT)


def record(name, data=b"", flag=b"0", link=""):
    return header(name, len(data), flag, link) + data + bytes(-len(data) % 512)


def pax_line(key, value):
    suffix = b" " + key.encode() + b"=" + value + b"\n"
    length = len(suffix) + 1
    while len(str(length)) + len(suffix) != length:
        length = len(str(length)) + len(suffix)
    return str(length).encode() + suffix


def volume(kind):
    target, payload, index, count = "payload.bin", PAYLOAD, 0, 2
    image = record(target, payload)
    linkdata = kind.startswith("linkdata-")
    if kind in ("gnu", "pax"):
        target = "каталог 空/" + "long name/" * 15 + "payload.bin"
        output = io.BytesIO()
        with tarfile.open(fileobj=output, mode="w", format=(
                tarfile.GNU_FORMAT if kind == "gnu" else tarfile.PAX_FORMAT)) as archive:
            info = tarfile.TarInfo(target)
            info.size = len(payload)
            if kind == "pax":
                info.pax_headers = {"mtime": "1700000000.123456789", "uid": "12345678"}
            archive.addfile(info, io.BytesIO(payload))
        # Keep generated extension records, replace writer's terminal padding.
        image = output.getvalue()[:output.tell() - (output.tell() % 512)]
        with tarfile.open(fileobj=io.BytesIO(image)) as archive:
            member = archive.getmember(target)
            image = image[:member.offset_data + (len(payload) + 511) // 512 * 512]
    elif kind == "prefix":
        target = "directory/" * 12 + target
        image = record(target, payload)
    elif kind in ("nul-type", "base256", "signed"):
        h = bytearray(header(target, len(payload)))
        if kind == "nul-type":
            h[156] = 0
        elif kind == "base256":
            h[124:136] = (len(payload) | (1 << 95)).to_bytes(12, "big")
        else:
            h[265:270] = b"\xff" * 5
        image = checksum(h, kind == "signed") + payload + bytes(-len(payload) % 512)
    elif kind.startswith("sparse"):
        blocks = [(512, 512), (2048, 333)]
        if kind == "sparse-extended":
            blocks = [(512 + i * 1024, 512 if i < 5 else 333) for i in range(6)]
        if kind == "sparse-overlap":
            blocks[1] = (512, 333)
        size = blocks[-1][0] + blocks[-1][1]
        data = b"".join(bytes([65 + i]) * n for i, (_, n) in enumerate(blocks))
        payload = bytearray(size)
        for i, (offset, length) in enumerate(blocks):
            payload[offset:offset + length] = bytes([65 + i]) * length
        payload = bytes(payload)
        h = bytearray(header(target, len(data), b"S"))
        h[257:265] = b"ustar  \0"
        h[483:495] = octal(size + (1 if kind == "sparse-size" else 0))
        for i, (offset, length) in enumerate(blocks[:4]):
            h[386 + i * 24:410 + i * 24] = octal(offset) + octal(length)
        extended = b""
        if len(blocks) > 4:
            h[482] = 1
            block = bytearray(512)
            for i, (offset, length) in enumerate(blocks[4:]):
                block[i * 24:(i + 1) * 24] = octal(offset) + octal(length)
            extended = bytes(block)
        image = checksum(h) + extended + data + bytes(-len(data) % 512)
    elif kind == "symlink":
        payload = b"target with spaces.bin"
        image = record(target, flag=b"2", link=payload.decode())
    elif kind in ("hardlink", "directory", "linkdata-hardlink", "linkdata-directory"):
        directory = kind.endswith("directory")
        flag, name = (b"5", "directory/") if directory else (b"1", "alias.bin")
        if linkdata:
            image = record(name, b"ignored declared payload" * 30, flag, "marker.bin")
        else:
            image = header(name, 4096 if directory else 0, flag, "marker.bin")
        image += record(target, payload)
        index, count = 1, 3
    elif kind == "large-pax":
        extra = pax_line("comment", b"x" * (192 * 1024))
        image = record("PaxHeader", extra, b"x") + image
    elif kind == "longname-tail":
        image = record("LongName", b"payload.bin\0" + bytes(49152 - 12), b"L") + image
    elif kind in ("huge-size", "huge-pax", "huge-longname"):
        h = bytearray(header(target, 0, {"huge-size": b"0", "huge-pax": b"x",
                                       "huge-longname": b"L"}[kind]))
        h[124:136] = ((1 << 95) | ((1 << 63) - 1)).to_bytes(12, "big")
        image = checksum(h)
    elif kind == "checksum":
        image = bytes([image[0] ^ 1]) + image[1:]
    elif kind == "short-header":
        image = image[:200]
    elif kind == "short-payload":
        image = image[:512 + len(payload) - 3]
    if kind not in ("short-header", "short-payload"):
        marker = record("marker.bin", b"following record")
        if kind == "late-checksum":
            marker = bytes([marker[0] ^ 1]) + marker[1:]
        image += marker + bytes(1024)
    if kind == "long-zero-tail":
        image += bytes(192 * 1024)
    return image, dict(target=target, payload=payload.hex(), index=index, count=count,
                       linkdata=linkdata)


if __name__ == "__main__":
    image, metadata = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps(metadata))
