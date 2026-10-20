"""RFC 4648 standard/URL and IMAP alphabets with canonical tail bits."""
from pathlib import Path
import base64
import json
import sys

CASES = tuple(f"{alphabet}-{padding}-{tail}" for alphabet in ("std", "url", "imap")
              for padding in ("padded", "bare") for tail in range(3)) + (
    "empty", "whitespace", "wrapped", "long", "rfc-one", "rfc-two", "rfc-three")
BAD = ("one-symbol", "bad-bits-one", "bad-bits-two", "bare-bits-one", "bare-bits-two",
       "extra-padding", "pad-first", "pad-middle", "mixed-url", "mixed-imap",
       "nul", "high-byte", "long-tail", "long-bad-bits")


def volume(kind):
    payload = bytes(range(256)) * 6
    image = base64.b64encode(payload)
    if kind.startswith(("std-", "url-", "imap-")):
        alphabet, padding, tail = kind.split("-")
        payload += bytes(range(int(tail)))
        image = base64.b64encode(payload)
        if alphabet == "url":
            image = image.translate(bytes.maketrans(b"+/", b"-_"))
        elif alphabet == "imap":
            image = image.replace(b"/", b",")
        if padding == "bare":
            image = image.rstrip(b"=")
    elif kind == "empty":
        image, payload = b"", b""
    elif kind == "whitespace":
        image, payload = b" \r\n\t\xa0", b""
    elif kind == "wrapped":
        image = b"\t\r\n" + b" \r\n\t\xa0".join(image[p:p + 76] for p in range(0, len(image), 76))
    elif kind == "long":
        payload = bytes(range(256)) * 400 + b"\xfb"
        image = base64.urlsafe_b64encode(payload).rstrip(b"=")
    elif kind.startswith("rfc-"):
        payload = {"rfc-one": b"f", "rfc-two": b"fo", "rfc-three": b"foo"}[kind]
        image = base64.b64encode(payload)
    elif kind in ("one-symbol", "bad-bits-one", "bad-bits-two", "bare-bits-one", "bare-bits-two",
                  "extra-padding", "pad-first", "pad-middle", "mixed-url", "mixed-imap",
                  "nul", "high-byte"):
        image = {"one-symbol": b"A", "bad-bits-one": b"AB==", "bad-bits-two": b"ABC=",
                 "bare-bits-one": b"AB", "bare-bits-two": b"ABC", "extra-padding": b"TQ===",
                 "pad-first": b"=AAA", "pad-middle": b"TW=F", "mixed-url": b"+_8=",
                 "mixed-imap": b"/,8=", "nul": b"TW\0u", "high-byte": b"TW\xffu"}[kind]
    elif kind in ("long-tail", "long-bad-bits"):
        payload = bytes(range(256)) * 402 + b"f"
        image = base64.b64encode(payload)
        if kind == "long-tail":
            image += b"not padding"
        else:
            # The final sextet carries four unused bits, after the initial probe.
            image = image[:-3] + bytes((image[-3] + 1,)) + image[-2:]
    return image, payload


if __name__ == "__main__":
    image, payload = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps({"payload": payload.hex()}))
