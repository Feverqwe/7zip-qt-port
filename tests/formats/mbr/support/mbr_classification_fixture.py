"""AIX/MBR disambiguation and protective GPT records."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
from pathlib import Path
import json
import struct
import sys
from tests.formats.mbr.support.mbr_fixture import volume as mbr_volume
from tests.formats.gpt.support.gpt_fixture import volume as gpt_volume

LINUX_TYPES = (0x82, 0xfd, 0x83, 0x8e, 5, 15, 133)
CASES = ("aix", "aix-no-signature", "aix-no-lvm", "aix-ascii", "aix-short",
         "protective", "protective-empty", "gpt-valid", "gpt-bad-crc") + tuple(
             "aix-linux-" + str(typ) for typ in LINUX_TYPES)


def volume(kind):
    if kind.startswith("gpt-"):
        image, _, _ = gpt_volume("sector-512" if kind == "gpt-valid" else "crc")
        return image, [], True, "GPT" if kind == "gpt-valid" else None
    raw, _ = mbr_volume("zero-lba" if kind == "aix-short" else "primary")
    image = bytearray(raw)
    image[450] = 1
    rejected = False
    if kind.startswith("aix"):
        image[:4] = b"ibma" if kind == "aix-ascii" else b"\xc9\xc2\xd4\xc1"
        image[7 * 512:8 * 512] = bytes(512)
        image[7 * 512:7 * 512 + 4] = b"_BAD" if kind == "aix-no-lvm" else b"_LVM"
        rejected = kind in ("aix", "aix-no-signature")
        if kind == "aix-no-signature":
            image[510:512] = b"\0\0"
        if kind == "aix-short":
            image = image[:4 * 512]
        if kind.startswith("aix-linux-"):
            image[446 + 3 * 16 + 4] = int(kind[10:])
    elif kind == "protective":
        image[450] = 0xee
        rejected = True
    elif kind == "protective-empty":
        image[446 + 3 * 16 + 4] = 0xee
        rejected = True
    start, size = struct.unpack_from("<II", image, 454)
    parts = [] if rejected else [dict(offset=start * 512, size=size * 512,
                                     payload=bytes(image[start * 512:(start + size) * 512]).hex())]
    return bytes(image), parts, rejected, None


if __name__ == "__main__":
    image, parts, _, _ = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps(parts))
