"""UDF namespace variants: CS0 names and encoded symbolic link contents."""
import binascii
import struct

CASES = ("ns-latin1", "ns-utf16", "ns-slash", "ns-nul", "ns-surrogate",
         "ns-dot", "ns-dotdot", "ns-collapse", "ns-limit", "ns-spaces",
         "ns-bad-code", "ns-odd16", "ns-empty", "ns-hidden-file", "ns-hidden-dir",
         "ns-link-inline", "ns-link-external", "ns-link-long", "ns-link-chain",
         "ns-link-parent", "ns-link-root", "ns-link-reset", "ns-link-origin",
         "ns-link-unicode", "ns-link-mangle", "ns-link-unknown", "ns-link-empty",
         "ns-dstr-latin1", "ns-dstr-unicode", "ns-dstr-nul", "ns-dstr-long",
         "ns-dstr-invalid", "ns-dstr-odd")
COMMENTS = {"ns-dstr-latin1": "café", "ns-dstr-unicode": "空😀",
            "ns-dstr-nul": "a_b", "ns-dstr-long": "界" * 10,
            "ns-dstr-invalid": "", "ns-dstr-odd": ""}
BAD = ("ns-link-header", "ns-link-overrun", "ns-link-bad-code",
       "ns-link-large", "ns-link-hole")


def volume(kind):
    from udf_fixture import volume as base
    image, files = base("ad-zero" if kind == "ns-link-chain" else "native")
    image = bytearray(image)
    block = 2048

    def tag(pos, length):
        struct.pack_into("<HH", image, pos + 8,
                         binascii.crc_hqx(image[pos + 16:pos + length], 0), length - 16)
        image[pos + 4] = 0
        image[pos + 4] = sum(image[pos:pos + 16]) & 255

    def update(pos):
        tag(pos, 16 + struct.unpack_from("<H", image, pos + 10)[0])

    def cs0(text):
        return b"\x10" + text.encode("utf-16-be", errors="surrogatepass")

    def mangled(raw, prefix, extension=""):
        return prefix + "#%04X" % binascii.crc_hqx(raw[1:], 0) + extension

    if kind.startswith("ns-dstr-"):
        raw = {"ns-dstr-latin1": b"\x08caf\xe9",
               "ns-dstr-unicode": cs0("空😀"),
               "ns-dstr-nul": b"\x08a\x00b",
               "ns-dstr-long": cs0("界" * 15),
               "ns-dstr-invalid": b"\x09abc",
               "ns-dstr-odd": b"\x10\x00a\x00"}[kind]
        for sector in (32, 48):
            pos = sector * block
            field = bytearray(32)
            field[:len(raw)] = raw
            field[-1] = 255 if kind == "ns-dstr-long" else len(raw)
            image[pos + 24:pos + 56] = field
            update(pos)
        return bytes(image), files

    if kind.startswith("ns-link-"):
        def component(type, data=b""):
            return struct.pack("<BBH", type, len(data), 0) + data
        normal = component(5, b"\x08nested") + component(5, b"\x08tail.bin")
        data, target = normal, "nested/tail.bin"
        if kind == "ns-link-parent":
            data = component(4) + component(3) + component(5, b"\x08target")
            target = "./../target"
        elif kind == "ns-link-root":
            data, target = component(2), "/"
        elif kind == "ns-link-reset":
            data, target = normal + component(2) + component(5, b"\x08target"), "/target"
        elif kind == "ns-link-origin":
            data, target = component(1, b"origin") + normal, "nested/tail.bin"
        elif kind == "ns-link-unicode":
            target = "каталог/空😀.txt"
            data = component(5, cs0("каталог")) + component(5, cs0("空😀.txt"))
        elif kind == "ns-link-mangle":
            raw = b"\x08a/b.txt"
            data, target = component(5, raw), mangled(raw, "a_b", ".txt")
        elif kind == "ns-link-unknown":
            data = component(99) + normal
        elif kind == "ns-link-empty":
            data, target = b"", ""
        elif kind == "ns-link-header":
            data = b"\x05\x01\x00"
        elif kind == "ns-link-overrun":
            data = b"\x05\xff\x00\x00\x08a"
        elif kind == "ns-link-bad-code":
            data = component(5, b"\x09abc")
        elif kind == "ns-link-large":
            data = normal + bytes(block)
        pos = 263 * block
        image[pos + 27] = 12
        flags = struct.unpack_from("<H", image, pos + 34)[0] & ~3
        external = kind in ("ns-link-external", "ns-link-long", "ns-link-chain", "ns-link-large", "ns-link-hole")
        struct.pack_into("<H", image, pos + 34, flags | (1 if kind == "ns-link-long" else 0 if external else 3))
        struct.pack_into("<Q", image, pos + 56, len(data))
        struct.pack_into("<Q", image, pos + 64, int(external))
        if external:
            image[272 * block:272 * block + len(data)] = data
            if kind != "ns-link-chain":
                ad = struct.pack("<II", len(data) | ((2 << 30) if kind == "ns-link-hole" else 0), 15)
                if kind == "ns-link-long":
                    ad += bytes(8)
                struct.pack_into("<I", image, pos + 172, len(ad))
                image[pos + 176:pos + 176 + len(ad)] = ad
                tag(pos, 176 + len(ad))
            else:
                update(pos)
        else:
            struct.pack_into("<I", image, pos + 172, len(data))
            image[pos + 176:pos + block] = data + bytes(block - 176 - len(data))
            tag(pos, 176 + len(data))
        files["alpha.bin"] = target.encode()
        return bytes(image), files

    names = {
        "ns-latin1": (b"\x08caf\xe9.bin", "café.bin"),
        "ns-utf16": (cs0("каталог 空 😀.txt"), "каталог 空 😀.txt"),
        "ns-slash": (b"\x08a/b.txt", "a_b"),
        "ns-nul": (b"\x08a\x00b.txt", "a_b"),
        "ns-surrogate": (cs0("a\ud800.txt"), "a_txt"),
        "ns-dot": (b"\x08.", "."),
        "ns-dotdot": (b"\x08..", ".."),
        "ns-collapse": (b"\x08a//\x00b.txt", "a_b"),
        "ns-limit": (cs0("空" * 120 + ".txt"), "空" * 81),
        "ns-spaces": (b"\x08   ", "   "),
        "ns-bad-code": (b"\x09abc", None),
        "ns-odd16": (b"\x10\x00a\x00", None),
        "ns-empty": (b"", None),
        "ns-hidden-file": (b"\x08alpha.bin", None),
    }
    if kind == "ns-hidden-dir":
        image[260 * block + 132 + 18] |= 1
        update(260 * block + 132)
        del files["nested/tail.bin"]
        return bytes(image), files
    raw, expected = names[kind]
    if kind in ("ns-slash", "ns-nul", "ns-surrogate", "ns-collapse", "ns-limit", "ns-dot", "ns-dotdot"):
        expected = mangled(raw, expected, ".txt" if raw.endswith(b".txt") or kind in ("ns-surrogate", "ns-limit") else "")
    old = 260 * block
    records = image[old:old + 40]
    length = (38 + len(raw) + 3) & ~3
    record = bytearray(image[old + 40:old + 78]) + raw
    record[19] = len(raw)
    if kind == "ns-hidden-file":
        record[18] |= 1
    record.extend(bytes(length - len(record)))
    records += record + image[old + 88:old + 180]
    image[old:old + block] = records + bytes(block - len(records))
    tag(old + 40, length)
    root = 259 * block
    struct.pack_into("<Q", image, root + 56, len(records))
    struct.pack_into("<I", image, root + 176, len(records))
    update(root)
    payload = files.pop("alpha.bin")
    if expected is not None:
        files[expected] = payload
    return bytes(image), files
