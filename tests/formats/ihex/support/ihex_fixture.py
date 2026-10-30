"""Intel HEX address records and Linux IQS5xx settings-map fixtures."""
from pathlib import Path
import json
import sys

CASES = ("normal", "linear", "segment", "start", "ignored-address", "lowercase",
         "max-record", "top-address", "empty-data", "empty", "eof-address", "long-gap",
         "whitespace", "separate", "iqs", "iqs-settings", "iqs-overlap", "iqs-separators",
         "iqs-eof-data")
BAD = ("magic", "digit", "checksum", "truncated", "no-eof", "record-type", "linear-size",
       "start-size", "wrap", "eof-data", "separator", "iqs-low-checksum", "iqs-bounds",
       "iqs-extended", "iqs-type", "late-checksum")


def record(address, data=b"", typ=0, bad=False):
    content = bytes((len(data), address >> 8, address & 255, typ)) + data
    checksum = (-sum(content) + int(bad)) & 255
    return b":" + content.hex().upper().encode() + f"{checksum:02X}".encode()


def volume(kind):
    payload = b"Intel HEX payload\0" + bytes(range(96))
    address, index, target = 0x1234, 0, None
    profile = kind.startswith("iqs")
    delimiter = b"\n"
    records = []
    if profile:
        settings = b"user settings\0"
        records = [record(0x83c0, payload), record(0xbe00, settings, bad=kind in ("iqs-settings", "iqs-separators"))]
        pmap = bytearray(0xc000 - 0x83c0)
        pmap[:len(payload)] = payload
        pmap[0xbe00 - 0x83c0:0xbe00 - 0x83c0 + len(settings)] = settings
        if kind == "iqs-overlap":
            records += [record(0x83c2, b"last wins")]
            pmap[2:11] = b"last wins"
        if kind == "iqs-low-checksum": records[0] = record(0x83c0, payload, bad=True)
        if kind == "iqs-bounds": records[0] = record(0xbfff, b"overflow")
        if kind == "iqs-extended": records.insert(0, record(0, b"\0\1", 4))
        if kind == "iqs-type": records.insert(0, record(0, b"\0" * 4, 5))
        if kind == "iqs-separators": delimiter = b"\r\n# vendor settings\n" + bytes(70000)
        records += [record(0xffff, b"ignored" if kind == "iqs-eof-data" else b"", 1)]
        payload, address = bytes(pmap), 0x83c0
    else:
        if kind in ("linear", "ignored-address", "wrap", "top-address"):
            high = 0xffff if kind in ("wrap", "top-address") else 0x1234
            records += [record(0x4321 if kind == "ignored-address" else 0, high.to_bytes(2, "big"), 4)]
            address += high << 16
        if kind == "segment":
            records += [record(0, b"\x12\x34", 2)]
            address += 0x12340
        if kind == "start": records += [record(0, b"\0\1\0\2", 3), record(0, b"\0\1\0\2", 5)]
        if kind == "max-record": payload = bytes(range(255))
        if kind in ("wrap", "top-address"):
            address, payload = 0xfffffffd, b"end!" if kind == "wrap" else b"end"
        if kind in ("empty", "empty-data"): payload = b""
        if kind != "empty": records += [record(0x1234 if kind == "segment" else address & 0xffff, payload)]
        if kind == "separate":
            records += [record(0x11, b"separate block")]
            index, target, address, payload = 1, "1", 0x11, b"separate block"
        records += [record(0xffff if kind == "eof-address" else 0, b"x" if kind == "eof-data" else b"", 1)]
        if kind == "long-gap": delimiter = b"\r\n" * 35000
        if kind == "whitespace": delimiter = b" \t\r\n" * 23
        if kind == "checksum": records[0] = record(address, payload, bad=True)
        if kind == "late-checksum":
            records.insert(-1, record(0x2000, b"late failure", bad=True))
            delimiter = b"\n" * 70000
        if kind == "linear-size": records.insert(0, record(0, b"\0", 4))
        if kind == "start-size": records.insert(0, record(0, b"\0", 5))
        if kind == "record-type": records.insert(0, record(0, b"", 6))
        if kind == "no-eof": records.pop()
        if kind == "separator": delimiter = b"garbage"
    image = delimiter.join(records) + b"\r\n"
    if kind == "magic": image = b"!" + image[1:]
    if kind == "digit": image = image[:5] + b"G" + image[6:]
    if kind == "truncated": image = image[:-7]
    if kind == "lowercase": image = image.lower()
    return image, dict(payload=payload.hex(), index=index, target=target, address=str(address),
                       profile=profile, count=0 if kind in ("empty", "empty-data") else 2 if kind == "separate" else 1)


if __name__ == "__main__":
    image, metadata = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps(metadata))
