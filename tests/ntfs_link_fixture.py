"""NTFS reparse link read scenarios over the official creator volume."""
import struct
from ntfs_fixture import attributes, nonresident, put, record, u16, u32, u64
from ntfs_ea_fixture import allocated, resident, set_attributes
from ntfs_security_fixture import named_resident

VALID = ("link-symlink", "link-print-name", "link-unicode", "link-surrogate", "link-nul",
         "link-backslashes", "link-junction", "link-junction-lost", "link-nonresident",
         "link-fragmented", "link-extension", "link-directory", "link-readonly", "link-ads",
         "link-user", "link-named-reparse", "link-odd-offset", "link-cloud", "link-limit-large")
UNSUPPORTED = ("link-bad-offset", "link-bad-length", "link-no-print-name", "link-unknown-ms",
               "link-user-short", "link-junction-no-drive", "link-dedup",
               "link-limit-small", "link-limit-large-rejected")


def target(kind):
    return {"link-unicode": "данные 空", "link-surrogate": "folder/😀",
            "link-backslashes": "folder/alpha", "link-junction": "./alpha",
            "link-junction-lost": "./../alpha", "link-fragmented": "a" * 2100,
            "link-named-reparse": None, "link-cloud": "OneDrive"}.get(kind, "alpha")


def path(kind):
    return "[LOST]/gamma" if kind == "link-junction-lost" else "gamma"


def volume(kind):
    from ntfs_fixture import volume as base
    original, files = base("ea-resident" if kind == "link-cloud" else "base")
    image = bytearray(original)
    offset, gamma = record(image, 66)
    attrs = [a for _, a in attributes(gamma) if u32(a, 0) != 0xc0]
    wanted = target(kind)
    name = wanted or "alpha"
    if kind in ("link-junction", "link-junction-lost", "link-junction-no-drive"):
        name = "C:\\alpha" if kind != "link-junction-no-drive" else "alpha"
    elif kind == "link-backslashes":
        name = "folder\\alpha"
    if kind == "link-nul":
        name += "\0"
    if kind == "link-user":
        encoded = name.encode("utf-16-le")
        payload = struct.pack("<IHH16s", 0x2000000d, 24 + len(encoded), 0, b"user-tag-guid1234") + encoded
    else:
        encoded = name.encode("utf-16-le")
        substitute = "wrong".encode("utf-16-le") if kind == "link-print-name" else b""
        if kind == "link-odd-offset":substitute = b"x"
        fields = struct.pack("<HHHH", 0, len(substitute), len(substitute), len(encoded))
        junction = kind.startswith("link-junction")
        data = fields + (b"" if junction else struct.pack("<I", 1)) + substitute + encoded
        payload = struct.pack("<IHH", 0xa0000003 if junction else 0xa000000c, len(data), 0) + data
    payload = bytearray(payload)
    if kind.startswith("link-limit-"):
        if kind != "link-limit-small":
            payload += bytes(32768 - len(payload))
        _, defs = record(image, 4)
        attr = next(a for _, a in attributes(defs) if u32(a, 0) == 0x80)
        rp = u16(attr, 32)
        lcn = int.from_bytes(attr[rp + 2:rp + 2 + (attr[rp] >> 4)], "little")
        limit = {"link-limit-small": len(payload) - 1, "link-limit-large": len(payload),
                 "link-limit-large-rejected": len(payload) - 1}[kind]
        for pos in range(lcn * 4096, lcn * 4096 + u64(attr, 48), 0xa0):
            if u32(image, pos + 0x80) == 0xc0:
                put(image, pos + 0x98, "Q", limit)
                break
    if kind == "link-bad-offset":put(payload, 12, "H", 0xffff)
    elif kind == "link-bad-length":put(payload, 14, "H", 0xfffe)
    elif kind == "link-no-print-name":put(payload, 14, "H", 0)
    elif kind == "link-unknown-ms":put(payload, 0, "I", 0xa00000fe)
    elif kind == "link-cloud":payload = bytearray(struct.pack("<IHH16x", 0x9000001a, 16, 0))
    elif kind == "link-dedup":payload = bytearray(struct.pack("<IHH16x", 0x80000013, 16, 0))
    elif kind == "link-user-short":
        payload = bytearray(struct.pack("<IHH16s", 0x2000000d, 2, 0, bytes(16)) + bytes(8))
    if kind in ("link-nonresident", "link-fragmented", "link-limit-large", "link-limit-large-rejected"):
        template = next(a for a in attrs if u32(a, 0) == 0x80 and a[8])
        lcns = list(range(1400, 1400 + (len(payload) + 4095) // 4096 * 2, 2))
        for i, lcn in enumerate(lcns):
            part = payload[i * 4096:(i + 1) * 4096]
            image[lcn * 4096:(lcn + 1) * 4096] = part + bytes(4096 - len(part))
        allocated(image, 6, 0x80, lcns)
        reparse = nonresident(template, [(1, lcn) for lcn in lcns], len(payload))
        put(reparse, 0, "I", 0xc0);put(reparse, 14, "H", 5)
    elif kind == "link-named-reparse":
        reparse = named_resident(0xc0, "ignored", payload, 5)
    else:
        reparse = resident(0xc0, payload, 5)
    if kind == "link-directory":put(gamma, 22, "H", u16(gamma, 22) | 2)
    for a in attrs:
        if u32(a, 0) == 0x10:
            pos = u16(a, 20) + 32
            put(a, pos, "I", u32(a, pos) | 0x400 |
                (0x10 if kind == "link-directory" else 0) | (1 if kind == "link-readonly" else 0))
        if kind == "link-cloud" and u32(a, 0) == 0xe0:
            pos = a.index(b"$LXMOD\0") + len(b"$LXMOD\0")
            put(a, pos, "I", 0o120751)
        if kind == "link-junction-lost" and u32(a, 0) == 0x30:
            put(a, u16(a, 20), "Q", 999)
    if kind == "link-ads":
        attrs.append(named_resident(0x80, "side", b"alternate data", 6))
        files["gamma:side"] = b"alternate data"
    if kind == "link-extension":
        extension = bytearray(gamma)
        put(extension, 16, "H", 2);put(extension, 32, "Q", 66 | (u16(gamma, 16) << 48))
        put(extension, 44, "I", 16)
        set_attributes(image, u64(image, 0x30) * 4096 + 16 * 1024, extension, [reparse])
        allocated(image, 0, 0xb0, [16])
        rows = []
        for a, number, sequence in [(a, 66, u16(gamma, 16)) for a in attrs] + [(reparse, 16, 2)]:
            rows.append(struct.pack("<IHBBQQH6x", u32(a, 0), 32, 0, 26, 0,
                                    number | (sequence << 48), u16(a, 14)))
        attrs.append(resident(0x20, b"".join(rows), 7))
    else:
        attrs.append(reparse)
    attrs.sort(key=lambda a: u32(a, 0))
    set_attributes(image, offset, gamma, attrs)
    files.pop("gamma")
    if kind not in UNSUPPORTED:
        files[path(kind)] = wanted.encode() if wanted is not None else b"B" * 65536
    return bytes(image), files
