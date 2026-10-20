"""Small newc/crc streams and legacy CPIO controls."""
from pathlib import Path
import json
import stat
import struct
import sys

CASES = ("newc", "crc", "name-nuls", "name-max", "no-trailer", "no-final-padding",
         "zero-padding", "leading-zeros", "empty", "member-chain", "mixed", "unused-sum", "rdev",
         "links-first", "links-middle", "links-last", "links-overwrite", "links-subset",
         "inode-single", "inode-types", "inode-devices", "trailer-reset", "no-reset",
         "binary-le", "binary-be", "odc")
BAD = ("checksum", "checksum-overwritten", "checksum-empty", "name-garbage",
       "truncated-header", "truncated-data", "trailer-data", "broken-padding")


def record(name, data=b"", *, ino=1, links=1, mode=stat.S_IFREG | 0o644,
           crc=False, name_nuls=1, dev=0, rdev=0, checksum=None, final_padding=True):
    encoded = name.encode("utf-8") + bytes(name_nuls)
    values = (ino, mode, 1000, 1000, links, 1, len(data), 0, dev, rdev, 0,
              len(encoded), (sum(data) & 0xffffffff) if crc else 0)
    if checksum is not None:
        values = values[:-1] + (checksum,)
    header = b"070702" if crc else b"070701"
    header += b"".join(f"{value:08x}".encode() for value in values)
    header += encoded
    header += bytes(-len(header) % 4)
    return header + data + (bytes(-len(data) % 4) if final_padding else b"")


def trailer(crc=False):
    return record("TRAILER!!!", ino=0, links=1, mode=0, crc=crc)


def legacy(name, data=b"", *, endian=None, ino=1, mode=stat.S_IFREG | 0o644):
    name = name.encode() + b"\0"
    if endian:
        header = struct.pack(endian + "13H", 0x71c7, 0, ino, mode, 0, 0, 1, 0,
                             0, 1, len(name), len(data) >> 16, len(data) & 65535)
        header += name
        header += bytes(-len(header) % 2)
        return header + data + bytes(-len(data) % 2)
    values = ((0, 6), (ino, 6), (mode, 6), (0, 6), (0, 6), (1, 6), (0, 6),
              (1, 11), (len(name), 6), (len(data), 11))
    return b"070707" + b"".join(f"{value:0{size}o}".encode() for value, size in values) + name + data


def volume(kind):
    files = {"данные 空.txt": b"cpio payload\n"}
    links = {}
    data = record(next(iter(files)), next(iter(files.values())), crc=kind == "crc")
    ending = trailer(crc=kind == "crc")
    if kind in ("name-nuls", "name-max"):
        name = next(iter(files))
        count = 4096 - len(name.encode()) if kind == "name-max" else 17
        data = record(name, files[name], name_nuls=count)
    elif kind == "no-trailer":
        ending = b""
    elif kind == "no-final-padding":
        files = {"data": b"abc"}
        data = record("data", b"abc", final_padding=False)
        ending = b""
    elif kind == "zero-padding":
        files = {"a": b"A", "b": b"B"}
        data = record("a", b"A") + bytes(8192) + record("b", b"B", ino=2)
        ending += bytes(4099)
    elif kind == "leading-zeros":
        data = bytes(4096) + data
    elif kind == "empty":
        data, files = b"", {}
    elif kind == "member-chain":
        files = {"a": b"A", "b": b"B"}
        data = record("a", b"A") + trailer() + bytes(4096) + record("b", b"B")
    elif kind == "mixed":
        files = {"a": b"A", "b": b"B"}
        data = record("a", b"A") + record("b", b"B", ino=2, crc=True)
        ending = trailer(crc=True)
    elif kind in ("unused-sum", "rdev"):
        name = next(iter(files))
        data = record(name, files[name], checksum=77 if kind == "unused-sum" else None,
                      rdev=0x12345678 if kind == "rdev" else 0)
    elif kind.startswith("links-"):
        names = ("a", "b", "c")
        position = {"links-first": 0, "links-middle": 1, "links-last": 2,
                    "links-overwrite": 0, "links-subset": 0}[kind]
        parts = [record(name, b"first" if i == position else b"", links=7 if kind == "links-subset" else 3,
                        crc=i != 1) for i, name in enumerate(names)]
        final = b"first"
        if kind == "links-overwrite":
            final = b"last body"
            parts[2] = record(names[2], final, links=3, crc=True)
        data = b"".join(parts)
        files = {name: final for name in names}
    elif kind == "inode-single":
        files = {"empty": b"", "full": b"content"}
        data = record("empty") + record("full", b"content")
    elif kind == "inode-types":
        files = {"empty": b""}
        links = {"symbolic": "empty"}
        data = record("empty", links=2) + record("symbolic", b"empty", links=2, mode=stat.S_IFLNK | 0o777)
    elif kind == "inode-devices":
        files = {"a": b"A", "b": b"B"}
        data = record("a", b"A", links=2, dev=1) + record("b", b"B", links=2, dev=2)
    elif kind in ("trailer-reset", "no-reset"):
        files = {"a": b"A", "aa": b"A", "b": b"B", "bb": b"B"}
        if kind == "no-reset":
            files = dict.fromkeys(files, b"B")
        data = record("a", b"A", links=2) + record("aa", links=2)
        if kind == "trailer-reset":
            data += trailer()
        data += record("b", b"B", links=2) + record("bb", links=2)
    elif kind in ("binary-le", "binary-be", "odc"):
        endian = "<" if kind == "binary-le" else ">" if kind == "binary-be" else None
        files = {"old": b"legacy"}
        data = legacy("old", b"legacy", endian=endian)
        ending = legacy("TRAILER!!!", endian=endian, ino=0, mode=0)
    elif kind == "checksum":
        name = next(iter(files))
        data = record(name, files[name], crc=True, checksum=123)
    elif kind in ("checksum-overwritten", "checksum-empty"):
        data = record("a", b"first" if kind == "checksum-overwritten" else b"",
                      links=2, crc=True, checksum=123)
        data += record("b", b"last", links=2)
    elif kind == "name-garbage":
        data = bytearray(record("a", b"x", name_nuls=3))
        data[112] = ord("Z")
        data = bytes(data)
    elif kind == "truncated-header":
        return data + b"070701", files, links
    elif kind == "truncated-data":
        return data[:-7], files, links
    elif kind == "trailer-data":
        ending = record("TRAILER!!!", b"not empty", ino=0, mode=0)
    elif kind == "broken-padding":
        data += bytes(1)
    return data + ending, files, links


if __name__ == "__main__":
    image, files, links = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps({"files": {name: data.hex() for name, data in files.items()}, "links": links}))
