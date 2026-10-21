"""Fixed MFT frame isolation and validated attribute-prefix recovery."""
from ntfs_fixture import attributes, put, record, save_record, u32
from ntfs_ea_fixture import resident, set_attributes

CASES = ("record-base", "record-total", "record-usa-count", "record-usa-tail",
         "record-signature", "record-baad", "record-offset", "record-before-data",
         "record-after-data", "record-unicode", "record-root", "record-bitmap",
         "record-security", "record-extension", "record-boot-total", "record-boot-fixup", "record-number",
         "record-usa-zero", "record-usa-overrun")


def unavailable(kind):
    if kind == "record-unicode":
        return ["данные 空"]
    if kind in ("record-total", "record-signature",
                "record-baad", "record-offset", "record-before-data", "record-usa-zero", "record-usa-overrun"):
        return ["gamma"]
    return []


def volume(kind):
    from ntfs_fixture import volume as base
    if kind not in CASES:
        raise ValueError(kind)
    if kind == "record-extension":
        from ntfs_directory_fixture import volume as directory
        source, files = directory("dir-extension")
    else:
        source, files = base()
    image = bytearray(source)
    if kind == "record-base":
        return bytes(image), files
    number = {"record-boot-total": 0, "record-boot-fixup": 0, "record-root": 5, "record-bitmap": 6,
              "record-security": 9, "record-extension": 16, "record-unicode": 65}.get(kind, 66)
    offset, rec = record(image, number)
    if kind in ("record-before-data", "record-after-data"):
        attrs = [a for _, a in attributes(rec)]
        if kind == "record-before-data":
            data = next(a for a in attrs if u32(a, 0) == 0x80)
            put(data, 4, "I", 0xfffffff8)
        else:
            bad = resident(0xd0, bytes(8), 50)
            put(bad, 4, "I", 0)
            attrs.append(bad)
        set_attributes(image, offset, rec, attrs)
    else:
        if kind == "record-usa-count":
            put(rec, 6, "H", 1)
        elif kind in ("record-usa-zero", "record-usa-overrun"):
            put(rec, 6, "H", 0 if kind == "record-usa-zero" else 10)
        elif kind == "record-signature":
            put(rec, 0, "I", 0x12345678)
        elif kind == "record-baad":
            put(rec, 0, "I", 0x44414142)
        elif kind == "record-offset":
            put(rec, 20, "H", 1040)
        elif kind == "record-number":
            put(rec, 44, "I", 67)
        elif kind not in ("record-usa-tail", "record-boot-fixup"):
            put(rec, 28, "I", 512)
        save_record(image, offset, rec)
        if kind in ("record-usa-tail", "record-boot-fixup"):
            image[offset + 510] ^= 1
    for name in unavailable(kind):
        files.pop(name)
    return bytes(image), files
