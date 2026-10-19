"""Small Apple-produced HFS+ image and focused corrupt/fragmented variants."""
from pathlib import Path
import base64
import json
import struct
import sys
import zlib


def volume(kind="base"):
    if kind.startswith("classic"):
        from hfs_classic_fixture import volume as classic_volume
        return classic_volume(kind)
    if kind.startswith("modern"):
        from hfs_modern_fixture import volume as modern_volume
        return modern_volume(kind)
    image = bytearray(zlib.decompress(
        (Path(__file__).parent / "fixtures/hfs/hfsplus.img.zlib").read_bytes()))
    get16 = lambda off: struct.unpack_from(">H", image, off)[0]
    get32 = lambda off: struct.unpack_from(">I", image, off)[0]
    def put16(off, value):
        struct.pack_into(">H", image, off, value)
    def put32(off, value):
        struct.pack_into(">I", image, off, value)
    block = get32(1064)
    catalog = get32(1024 + 0x110 + 16) * block
    node_size = get16(catalog + 32)
    leaf = catalog + get32(catalog + 24) * node_size
    records = {}
    keys = {}
    for i in range(get16(leaf + 10)):
        key = leaf + get16(leaf + node_size - 2 * (i + 1))
        name_len = get16(key + 6)
        name = image[key + 8:key + 8 + name_len * 2].decode("utf-16-be")
        data = key + 2 + get16(key)
        if get16(data) in (1, 2):
            records[name] = data
            keys[name] = key
    alpha = records["alpha"]
    fork = alpha + 88
    start = get32(fork + 16)
    files = {
        "HfsFixture/alpha": bytes(range(256)) * 513,
        "HfsFixture/данные 空": b"Second HFS+ stream\n" * 4097,
        "HfsFixture/alpha:rsrc": b"Resource fork\n" * 173,
        "HfsFixture/alpha:com.sevenzip.fixture": b"inline attribute content",
    }
    if kind in ("preallocated", "empty-preallocated"):
        size = 1024 if kind == "preallocated" else 0
        struct.pack_into(">Q", image, fork, size)
        files["HfsFixture/alpha"] = files["HfsFixture/alpha"][:size]
        # A separate allocated extent after logical EOF is also valid.
        put32(fork + 20, 8)
        put32(fork + 24, start + 8)
        put32(fork + 28, 25)
    elif kind == "overflow-leaves":
        for i in range(8):
            struct.pack_into(">II", image, fork + 16 + i * 8, start + i, 1)
        extents = get32(1024 + 0xc0 + 16) * block
        ns = get16(extents + 32)
        struct.pack_into(">HIIII", image, extents + 14, 2, 1, 2, 2, 3)
        put32(extents + 40, 28)
        bitmap = extents + get16(extents + ns - 6)
        image[bitmap] = 0xf0

        def key(start_block):
            return struct.pack(">HBBII", 10, 0, 0, get32(alpha + 8), start_block)

        def node(number, forward, backward, kind, height, entries):
            pos = extents + number * ns
            image[pos:pos + ns] = bytes(ns)
            struct.pack_into(">IIBBHH", image, pos, forward, backward,
                             kind, height, len(entries), 0)
            offset = 14
            for i, entry in enumerate(entries):
                put16(pos + ns - 2 * (i + 1), offset)
                image[pos + offset:pos + offset + len(entry)] = entry
                offset += len(entry)
            put16(pos + ns - 2 * (len(entries) + 1), offset)

        node(1, 0, 0, 0, 2, [key(8) + struct.pack(">I", 2),
                             key(16) + struct.pack(">I", 3)])
        node(2, 3, 0, 255, 1, [key(8) + struct.pack(">II", start + 8, 8) + bytes(56)])
        node(3, 0, 2, 255, 1, [key(16) + struct.pack(">II", start + 16, 17) + bytes(56)])
    elif kind == "boot-blocks":
        image[:128] = b"Boot loader data" * 8
    elif kind in ("hfsx", "wrapped", "wrapped-hfsx", "wrapped-short"):
        if kind in ("hfsx", "wrapped-hfsx"):
            image[1024:1028] = b"HX\0\5"
            image[-1024:-1020] = b"HX\0\5"
            image[catalog + 51] = 0xbc  # HFSX binary catalog comparison.
        if kind.startswith("wrapped"):
            wrapper = bytearray(6144)
            wrapper[:128] = b"Boot loader data" * 8
            wrapper[1024:1026] = b"BD"
            struct.pack_into(">H", wrapper, 1024 + 10, 0x8200)
            struct.pack_into(">H", wrapper, 1024 + 18, len(image) // block + 1)
            struct.pack_into(">I", wrapper, 1024 + 20, block)
            struct.pack_into(">H", wrapper, 1024 + 28, 4)
            wrapper[1024 + 124:1024 + 126] = image[1024:1026]
            struct.pack_into(">HH", wrapper, 1024 + 126, 1,
                             len(image) // block - (kind == "wrapped-short"))
            image = wrapper + image + bytes(1024)
    elif kind == "extent-over-volume":
        put32(fork + 16, get32(1068))
        image.extend(b"outside volume!" * 20000)
    elif kind == "allocation-tail-over-volume":
        struct.pack_into(">Q", image, fork, 1)
        put32(fork + 16, get32(1068) - 1)
        files["HfsFixture/alpha"] = b"\0"
    elif kind == "metadata-short-map":
        put32(1024 + 0x110 + 20, 31)
    elif kind == "leaf-height":
        image[leaf + 9] = 2
    elif kind == "leaf-backlink":
        put32(leaf + 4, 1)
    elif kind == "leaf-offset":
        put16(leaf + node_size - 2, 15)
    elif kind == "leaf-tail":
        put32(catalog + 28, 2)
    elif kind == "leaf-count":
        put32(catalog + 20, get32(catalog + 20) + 1)
    elif kind == "root-depth":
        put16(catalog + 14, 2)
    elif kind == "key-format":
        put32(catalog + 52, 0)
    elif kind == "parent-cycle":
        put32(keys["HfsFixture"] + 2, get32(records["empty"] + 8))
    elif kind == "volume-truncated":
        del image[-block:]
    elif kind != "base":
        raise ValueError(kind)
    return bytes(image), files


if __name__ == "__main__":
    data, files = volume(sys.argv[1] if len(sys.argv) > 1 else "base")
    print(json.dumps({"image": base64.b64encode(zlib.compress(data)).decode(),
                      "size": len(data),
                      "files": {name: base64.b64encode(value).decode()
                                for name, value in files.items()}}))
