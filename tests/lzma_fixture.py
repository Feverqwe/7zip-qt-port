"""LZMA-alone and LZMA86 framing with independent encoded payloads."""
from pathlib import Path
import json
import io
import lzma
import struct
import sys
import zipfile

CASES = ("unknown", "known", "dict-zero", "dict-one", "dict-three", "dict-1280",
         "dict-4352", "dict-5001", "dict-66048", "props-zero", "props-max",
         "empty", "empty-known", "empty-first", "concatenated", "concat-odd",
         "lzma86", "lzma86-bcj")
BAD = ("property", "range-init", "header-short", "payload-short", "size-small",
       "size-large", "size-56bit", "corrupt", "tail", "second-property", "filter")


def member(data, *, dictionary=4096, known=False, zero_props=False):
    filters = [{"id": lzma.FILTER_LZMA1, "dict_size": 4096,
                "lc": 0 if zero_props else 3, "lp": 0, "pb": 0 if zero_props else 2}]
    image = lzma.compress(data, format=lzma.FORMAT_ALONE, filters=filters)
    size = len(data) if known else 0xffffffffffffffff
    return image[:1] + struct.pack("<IQ", dictionary, size) + image[13:]


def volume(kind):
    payload = b"A" * 32
    if kind == "zip-lzma":
        output = io.BytesIO()
        with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_LZMA) as archive:
            archive.writestr("данные 空.txt", payload)
        return output.getvalue(), payload, "zip"
    image = member(payload)
    format_name = "lzma"
    if kind == "known":
        image = member(payload, known=True)
    elif kind.startswith("dict-"):
        dictionary = {"zero": 0, "one": 1, "three": 3}.get(kind[5:])
        if dictionary is None:
            dictionary = int(kind[5:])
        if dictionary >= 4096:
            payload = bytes(range(256)) * 128
        image = member(payload, dictionary=dictionary)
    elif kind == "props-zero":
        image = member(payload, zero_props=True)
    elif kind == "props-max":
        # Pinned 7zz: 7z container, LZMA:lc=8:lp=4:pb=4:d=4k, mhc=off/ms=off.
        # The pack stream is prefixed with its alone properties and known size.
        image = bytes.fromhex("e00010000020000000000000000020ee16000000")
    elif kind in ("empty", "empty-known"):
        payload = b""
        image = member(payload, known=kind == "empty-known")
    elif kind == "empty-first":
        image = member(b"", known=True, zero_props=True) + image
    elif kind in ("concatenated", "concat-odd"):
        image += member(b"second member", dictionary=4352 if kind == "concat-odd" else 4096)
        payload += b"second member"
    elif kind in ("lzma86", "lzma86-bcj"):
        image = bytes((kind == "lzma86-bcj",)) + member(payload, dictionary=4352)
        format_name = "lzma86"
    elif kind == "property":
        image = b"\xe1" + image[1:]
    elif kind == "range-init":
        image = image[:13] + b"\x01" + image[14:]
    elif kind == "header-short":
        image = image[:10]
    elif kind == "payload-short":
        image = image[:-3]
    elif kind in ("size-small", "size-large", "size-56bit"):
        size = len(payload) + (-1 if kind == "size-small" else 1)
        if kind == "size-56bit":
            size = 1 << 56
        image = image[:5] + struct.pack("<Q", size) + image[13:]
    elif kind == "corrupt":
        image = image[:14] + bytes((image[14] ^ 0x80,)) + image[15:]
    elif kind == "tail":
        image += b"unexpected tail"
    elif kind == "second-property":
        image += b"\xe1" + member(payload)[1:]
    elif kind == "filter":
        image = b"\x02" + image
        format_name = "lzma86"
    return image, payload, format_name


if __name__ == "__main__":
    image, payload, format_name = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps({"payload": payload.hex(), "format": format_name}))
