"""Original classic HFS fixture retained to verify unsupported-format behavior."""
from pathlib import Path
import zlib


def volume(kind="classic"):
    if kind != "classic":
        raise ValueError(kind)
    return zlib.decompress((Path(__file__).parents[3] / "fixtures/hfs/hfs-classic.img.zlib").read_bytes()), {}
