"""Generated ISO payloads projected to the Linux High Sierra disk layout."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import struct


def volume(kind="sierra"):
    from tests.formats.iso.support.iso_fixture import volume as iso_volume
    base = {"sierra-xar": "xar", "sierra-multi": "multi", "sierra-block1024": "block-1024",
            "sierra-block1024-sector": "block-1024"}.get(kind, "plain")
    original, files = iso_volume(base)
    files.pop("[BOOT]/Boot-NoEmul.img")
    image = bytearray(original)
    primary = 16 * 2048
    source = original[primary:primary + 2048]
    block = struct.unpack_from("<H", source, 128)[0]
    root = struct.unpack_from("<I", source, 158)[0] + source[157]
    root_size = struct.unpack_from("<I", source, 166)[0]
    visited = set()

    def convert(pos):
        image[pos + 24] = image[pos + 25]
        image[pos + 25] = 0xa5  # Reserved byte must never become flags/timezone.

    def directory(start, size):
        if start in visited:
            return
        visited.add(start)
        pos = start
        while pos < start + size:
            length = image[pos]
            if not length:
                pos = (pos // block + 1) * block
                continue
            flags = image[pos + 25]
            if image[pos + 32] > 1 or image[pos + 33] > 1:
                if flags & 2:
                    extent = struct.unpack_from("<I", image, pos + 2)[0] + image[pos + 1]
                    directory(extent * block, struct.unpack_from("<I", image, pos + 10)[0])
            convert(pos)
            pos += length

    directory(root * block, root_size)
    descriptor = bytearray(2048)
    struct.pack_into("<I", descriptor, 0, 16)
    struct.pack_into(">I", descriptor, 4, 16)
    descriptor[8:15] = b"\1CDROM\1"
    descriptor[16:152] = source[8:144]
    descriptor[180:214] = source[156:190]
    image[primary:primary + 2048] = descriptor
    convert(primary + 180)
    image[primary + 2048:primary + 4096] = bytes(2048)
    return bytes(image), files
