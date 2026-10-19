"""Generated deep directory trees using native ISO records and payload storage."""
import struct


def volume(kind):
    from iso_fixture import volume as iso_volume
    image, _ = iso_volume("plain")
    image = bytearray(image)
    primary = 16 * 2048
    old_root = struct.unpack_from("<I", image, primary + 158)[0] * 2048
    pos = old_root
    while image[pos + 33:pos + 33 + image[pos + 32]] != b"ALPHA.BIN;1":
        pos += image[pos]
    template = image[pos:pos + 33]
    source_extent = struct.unpack_from("<I", template, 2)[0]
    payload = b"Deep ISO directory payload\n" * 193
    image[source_extent * 2048:source_extent * 2048 + len(payload)] = payload
    depth = 4096 if kind == "tree-clear" else 768
    if kind not in ("tree-deep", "tree-clear", "tree-cycle"):
        raise ValueError(kind)
    first = len(image) // 2048

    def both32(buffer, offset, value):
        struct.pack_into("<I", buffer, offset, value)
        struct.pack_into(">I", buffer, offset + 4, value)

    def record(name, extent, size, directory):
        result = bytearray(template)
        result[1] = 0
        both32(result, 2, extent)
        both32(result, 10, size)
        result[25] = 2 if directory else 0
        result[26:28] = bytes(2)
        result[32] = len(name)
        result += name
        result += bytes(len(result) % 2)
        result[0] = len(result)
        return result

    for level in range(depth + 1):
        sector = first + level
        entries = record(b"\0", sector, 2048, True)
        entries += record(b"\1", sector - 1 if level else sector, 2048, True)
        if level < depth:
            entries += record(b"D", sector + 1, 2048, True)
        elif kind == "tree-cycle":
            entries += record(b"D", first, 2048, True)
        else:
            entries += record(b"END.BIN;1", source_extent, len(payload), False)
        image += entries + bytes(2048 - len(entries))
    image[primary + 156:primary + 190] = record(b"\0", first, 2048, True)
    both32(image, primary + 80, len(image) // 2048)
    image[primary + 2048] = 3  # No virtual El Torito entries in this generated tree.
    files = {} if kind == "tree-cycle" else {"D/" * depth + "END.BIN": payload}
    return bytes(image), files
