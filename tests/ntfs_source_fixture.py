"""Bounded NTFS source prefixes and boot-copy availability."""
from ntfs_fixture import attributes, put, record, u16, u32, u64

CASES = ("source-prefix", "source-partial-data", "source-partial-cluster",
         "source-after-mft", "source-no-backup", "source-backup-used",
         "source-backup-mismatch", "source-backup-invalid", "source-bootstrap-short",
         "source-header-short", "source-no-valid-boot")
FATAL = ("source-bootstrap-short", "source-header-short", "source-no-valid-boot")


def volume(kind):
    from ntfs_fixture import volume as base
    image, files = base()
    image = bytearray(image)
    if kind == "source-prefix":
        image = image[:12 * 1024 * 1024]
    elif kind in ("source-partial-data", "source-partial-cluster"):
        _, rec = record(image, 66)
        data = next(a for _, a in attributes(rec) if u32(a, 0) == 0x80)
        start = u16(data, 32)
        length, width = data[start] & 15, data[start] >> 4
        lcn = int.from_bytes(data[start + 1 + length:start + 1 + length + width], "little", signed=True)
        image = image[:lcn * 4096 + 32768 + (123 if kind.endswith("cluster") else 0)]
        files.pop("gamma")
    elif kind == "source-after-mft":
        image = image[:128 * 1024]
        files = {"данные 空": files["данные 空"]}
    elif kind == "source-no-backup":
        image = image[:(u64(image, 40) // 8) * 4096]
    elif kind == "source-backup-used":
        put(image, 11, "H", 0)
    elif kind == "source-backup-mismatch":
        image[-512 + 72] ^= 1
    elif kind == "source-backup-invalid":
        put(image, len(image) - 512 + 11, "H", 0)
    elif kind == "source-bootstrap-short":
        image = image[:u64(image, 48) * 4096 + 512]
    elif kind == "source-header-short":
        image = image[:256]
    elif kind == "source-no-valid-boot":
        put(image, 11, "H", 0)
        put(image, len(image) - 512 + 11, "H", 0)
    else:
        raise ValueError(kind)
    return bytes(image), files
