"""Reuse generated filesystem samples through the authorized-device transport."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[3]))
import base64
import json
from pathlib import Path
import sys
import zlib


def volume(kind):
    if kind == "apfs":
        image = zlib.decompress((Path(__file__).parents[2] /
                                "fixtures/devices/apfs.img.zlib").read_bytes())
        return image, {"данные 空.txt": b"APFS generated device payload\n"}
    if kind.startswith("fat"):
        if kind == "fat-dirty":
            from tests.formats.fat.support.fat_fixture import readonly_volume, LONG_NAME, PAYLOAD
            return bytes(readonly_volume("dirty16").data), {LONG_NAME: PAYLOAD}
        from tests.formats.fat.support.fat_fixture import Volume, LONG_NAME, PAYLOAD
        fs = Volume(int(kind[3:]))
        return bytes(fs.data), {LONG_NAME: PAYLOAD}
    if kind in ("ntfs", "ntfs-dirty"):
        from tests.formats.ntfs.support.ntfs_fixture import volume
        return volume("volume-dirty" if kind == "ntfs-dirty" else "base")
    if kind == "hfsplus":
        from tests.formats.hfs.support.hfs_fixture import volume
        return volume("base")
    if kind == "udf":
        from tests.formats.udf.support.udf_fixture import volume
        return volume("native")
    if kind == "iso":
        from tests.formats.iso.support.iso_fixture import volume
        return volume("joliet")
    if kind == "squashfs":
        from tests.formats.squashfs.support.squashfs_fixture import volume
        return volume(compressed=True)
    if kind == "erofs":
        from tests.formats.erofs.support.erofs_fixture import FILES
        image = zlib.decompress((Path(__file__).parents[2] /
                                "fixtures/erofs/lz4.erofs.zlib").read_bytes())
        return image, FILES
    raise ValueError(kind)


if __name__ == "__main__":
    image, files = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    # One real data stream is sufficient here; format suites check all entries.
    name, payload = next((name, data) for name, data in files.items() if data and ":" not in name)
    print(json.dumps({"path": name, "bytes": base64.b64encode(payload).decode()}))
