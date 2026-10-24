"""Reuse generated filesystem samples through the authorized-device transport."""
import base64
import json
from pathlib import Path
import sys
import zlib


def volume(kind):
    if kind == "apfs":
        image = zlib.decompress((Path(__file__).parent /
                                "fixtures/devices/apfs.img.zlib").read_bytes())
        return image, {"данные 空.txt": b"APFS generated device payload\n"}
    if kind.startswith("fat"):
        if kind == "fat-dirty":
            from fat_fixture import readonly_volume, LONG_NAME, PAYLOAD
            return bytes(readonly_volume("dirty16").data), {LONG_NAME: PAYLOAD}
        from fat_fixture import Volume, LONG_NAME, PAYLOAD
        fs = Volume(int(kind[3:]))
        return bytes(fs.data), {LONG_NAME: PAYLOAD}
    if kind in ("ntfs", "ntfs-dirty"):
        from ntfs_fixture import volume
        return volume("volume-dirty" if kind == "ntfs-dirty" else "base")
    if kind == "hfsplus":
        from hfs_fixture import volume
        return volume("base")
    if kind == "udf":
        from udf_fixture import volume
        return volume("native")
    if kind == "iso":
        from iso_fixture import volume
        return volume("joliet")
    if kind == "squashfs":
        from squashfs_fixture import volume
        return volume(compressed=True)
    if kind == "erofs":
        from erofs_fixture import FILES
        image = zlib.decompress((Path(__file__).parent /
                                "fixtures/erofs/lz4.erofs.zlib").read_bytes())
        return image, FILES
    raise ValueError(kind)


if __name__ == "__main__":
    image, files = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    # One real data stream is sufficient here; format suites check all entries.
    name, payload = next((name, data) for name, data in files.items() if data and ":" not in name)
    print(json.dumps({"path": name, "bytes": base64.b64encode(payload).decode()}))
