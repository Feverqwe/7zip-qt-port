"""Linux initramfs boundaries, mixing and inode scope; disposable local bytes."""
import bz2
import gzip
import json
import lzma
import stat
from pathlib import Path
import sys
from cpio_fixture import record, trailer
from linux_frames_fixture import lz4_frame, lz4_literal, lzo_header, lzo_block
from zstd_fixture import raw as zstd_raw

METHODS = ("gzip", "bzip2", "lzma", "xz", "lzo", "lz4", "zstd")


def compress(method, data):
    if method == "gzip":
        return gzip.compress(data, mtime=0)
    if method == "bzip2":
        return bz2.compress(data)
    if method in ("lzma", "xz"):
        return lzma.compress(data, format=lzma.FORMAT_ALONE) if method == "lzma" else lzma.compress(
            data, format=lzma.FORMAT_XZ, check=lzma.CHECK_CRC32)
    if method == "lzo":
        return lzo_header() + lzo_block(data) + bytes(4) if data else lzo_header() + bytes(4)
    if method == "lz4":
        return lz4_frame([lz4_literal(data)]) + bytes(4) if data else lz4_frame([]) + bytes(4)
    return zstd_raw(data)


CASES = tuple(m + "-" + case for m in METHODS for case in (
    "single", "no-trailer", "concat", "raw-first", "raw-last", "links", "trailer-reset",
    "empty-after", "zeros-after", "final-zeros", "large", "raw-final-unpadded",
    "zeros-before-header", "symlink-trailer")) + (
        "mixed", "leading-zeros", "xz-multiple-blocks", "zstd-wide-window", "zstd-window-limit")
BAD = tuple(m + "-" + case for m in METHODS for case in (
    "split-header", "split-body", "split-padding", "empty-first", "zeros-first",
    "truncated", "junk", "raw-unaligned")) + ("zstd-window-exceeded",)


def volume(kind):
    method, case = kind.split("-", 1) if kind not in ("mixed", "leading-zeros") else ("gzip", kind)
    first, second = record("данные 空.txt", b"first payload"), record("second", b"second body", ino=2, crc=True)
    files = {"данные 空.txt": b"first payload"}
    image = compress(method, first + trailer())
    if method == "zstd" and case in ("wide-window", "window-limit", "window-exceeded"):
        # A known small content size bounds SDK allocation despite the declared window.
        frame = zstd_raw(first + trailer(), fcs=4)
        maximum = 31 if sys.maxsize > (1 << 32) else 30
        window_log = {"wide-window": 28, "window-limit": maximum,
                      "window-exceeded": 32}[case]
        image = frame[:4] + bytes((frame[4] & ~0x20, (window_log - 10) << 3)) + frame[5:]
    elif case == "large":
        files = {"large": bytes(range(256)) * 768}
        image = compress(method, record("large", files["large"]) + trailer())
    elif case == "multiple-blocks":
        from xz_fixture import multiple
        files = {"large": bytes(range(256)) * 768}
        image = multiple(record("large", files["large"]) + trailer(), zero_blocks=True)
    elif case in ("raw-final-unpadded", "zeros-before-header"):
        files["second"] = b"second body"
        image = compress(method, first)
        if case == "raw-final-unpadded":
            image += bytes(-len(image) % 4) + record("second", files["second"], ino=2, final_padding=False)
        else:
            image += compress(method, bytes(4) + second + trailer())
    elif case == "no-trailer":
        image = compress(method, first)
    elif case in ("concat", "raw-first", "raw-last", "mixed", "leading-zeros"):
        files["second"] = b"second body"
        if case == "concat":
            image = compress(method, first) + compress(method, second + trailer())
        elif case in ("raw-first", "leading-zeros"):
            image = first + compress(method, second + trailer())
            if case == "leading-zeros":
                image = bytes(4096) + image
        elif case == "raw-last":
            image = compress(method, first)
            image += bytes(-len(image) % 4) + second + trailer()
        else:
            files = {}
            image = b""
            for i, current in enumerate(METHODS):
                name, data = "file-" + current, bytes([65 + i]) * (i + 1)
                files[name] = data
                image += compress(current, record(name, data, ino=i + 1))
    elif case in ("links", "trailer-reset"):
        files = {"a": b"linked body", "b": b"" if case == "trailer-reset" else b"linked body"}
        image = compress(method, record("a", b"linked body", links=2))
        if case == "trailer-reset":
            image += compress(method, trailer())
        image += compress(method, record("b", links=2) + trailer())
    elif case == "symlink-trailer":
        files = {"a": b"last body", "b": b"last body", "target": b"target body"}
        image = compress(method, record("a", b"first body", links=2))
        image += compress(method, record("TRAILER!!!", b"target", ino=2,
                                         mode=stat.S_IFLNK | 0o777, crc=True))
        image += compress(method, record("b", files["b"], links=2) +
                          record("target", files["target"], ino=3) + trailer())
    elif case == "empty-after":
        image += compress(method, b"")
    elif case == "zeros-after":
        image += compress(method, bytes(77))
    elif case == "final-zeros":
        image = compress(method, first + bytes(77))
    elif case.startswith("split-"):
        offset = {"split-header": 15, "split-body": len(first) - 7,
                  "split-padding": len(first) - 1}[case]
        image = compress(method, first[:offset]) + compress(method, first[offset:] + trailer())
    elif case in ("empty-first", "zeros-first"):
        image = compress(method, b"" if case == "empty-first" else bytes(4)) + image
    elif case == "truncated":
        image = image[:-7]
    elif case == "junk":
        image += b"invalid next member"
    elif case == "raw-unaligned":
        image += bytes((1 - len(image)) % 4) + second
    return image, files


if __name__ == "__main__":
    image, files = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps({"files": {name: data.hex() for name, data in files.items()},
                      "links": {"TRAILER!!!": "target"} if sys.argv[1].endswith("symlink-trailer") else {}}))
