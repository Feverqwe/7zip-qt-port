"""Native genisoimage volumes and focused ISO extent/descriptor mutations."""
from pathlib import Path
import base64
import json
import struct
import sys
import zlib


def volume(kind="plain"):
    if kind.startswith("policy-"):
        from iso_policy_fixture import volume as policy_volume
        return policy_volume(kind)
    if kind.startswith("names-joliet-nls-"):
        from iso_nls_fixture import volume as nls_volume
        return nls_volume(kind)
    if kind.startswith("directory-"):
        from iso_directory_fixture import volume as directory_volume
        return directory_volume(kind)
    if kind.startswith("tree-"):
        from iso_tree_fixture import volume as tree_volume
        return tree_volume(kind)
    if kind.startswith("names-"):
        from iso_name_fixture import volume as name_volume
        return name_volume(kind)
    if kind.startswith("sierra"):
        from iso_sierra_fixture import volume as sierra_volume
        return sierra_volume(kind)
    if kind.startswith("zisofs"):
        from iso_zisofs_fixture import volume as compressed_volume
        return compressed_volume(kind)
    if kind.startswith("rock"):
        from iso_rock_fixture import volume as rock_volume
        return rock_volume(kind)
    joliet = kind == "joliet"
    image = bytearray(zlib.decompress(
        (Path(__file__).parent / "fixtures/iso" / ("joliet.iso.zlib" if joliet else "plain.iso.zlib")).read_bytes()))
    alpha = bytes(range(256)) * 513
    beta = b"ISO nested stream\n" * 4097
    boot = (b"El Torito fixture\n" * 128)[:2048]
    files = {"ALPHA.BIN": alpha, "NESTED/BETA.BIN": beta, "EMPTY": b"",
             "BOOT.BIN": boot, "[BOOT]/Boot-NoEmul.img": boot}
    if joliet:
        files = {"alpha.bin": alpha, "nested/beta.bin": beta, "empty": b"",
                 "boot.bin": boot, "nested/данные 空.txt": b"Unicode Joliet data\n" * 193,
                 "[BOOT]/Boot-NoEmul.img": boot}
    block = 2048
    def get32(offset, order="<"):
        return struct.unpack_from(order+"I", image, offset)[0]
    def both32(buffer, offset, value):
        struct.pack_into("<I", buffer, offset, value)
        struct.pack_into(">I", buffer, offset + 4, value)
    def both16(buffer, offset, value):
        struct.pack_into("<H", buffer, offset, value)
        struct.pack_into(">H", buffer, offset + 2, value)
    descriptors = []
    offset = 16 * 2048
    while image[offset] != 255:
        if image[offset] in (1, 2):
            descriptors.append(offset)
        offset += 2048
    terminator = offset
    primary = descriptors[0]
    root = get32(primary + 158) * block
    root_size = get32(primary + 166)
    def records(start, size):
        result = []
        pos = start
        while pos < start + size:
            length = image[pos]
            if not length:
                pos = (pos // block + 1) * block
                continue
            result.append((pos, bytearray(image[pos:pos + length])))
            pos += length
        return result
    root_records = records(root, root_size)
    named = {bytes(record[33:33 + record[32]]): pos for pos, record in root_records}
    alpha_record = named[b"ALPHA.BIN;1"]
    nested_record = named[b"NESTED"]
    nested = get32(nested_record + 2) * block
    nested_size = get32(nested_record + 10)
    nested_records = records(nested, nested_size)
    def write_records(start, size, entries):
        data = bytearray(size)
        pos = 0
        for entry in entries:
            if len(entry) > block - pos % block:
                pos = (pos // block + 1) * block
            assert pos + len(entry) <= size
            data[pos:pos + len(entry)] = entry
            pos += len(entry)
        image[start:start + size] = data
    def append_data(data):
        start = (len(image) + block - 1) // block * block
        image.extend(bytes(start - len(image)))
        image.extend(data)
        image.extend(bytes((-len(image)) % block))
        return start // block
    def update_volume():
        for descriptor in descriptors:
            both32(image, descriptor + 80, len(image) // block)
    def update_path_tables(old_extent, new_extent, xar=0):
        for descriptor in descriptors:
            table_size = get32(descriptor + 132)
            for location, order in ((140, "<"), (144, "<"), (148, ">"), (152, ">")):
                table = get32(descriptor + location, order) * block
                if not table:
                    continue
                pos = table
                while pos < table + table_size:
                    length = image[pos]
                    extent = get32(pos + 2, order)
                    if extent == old_extent:
                        struct.pack_into(order+"I", image, pos + 2, new_extent)
                        image[pos + 1] = xar
                    pos += 8 + length + length % 2
    if kind.startswith("date-"):
        offset = {"date-east": 52, "date-west": -52, "date-invalid-east": 53,
                  "date-invalid-west": -53, "date-normalize": 0}[kind]
        image[alpha_record + 18:alpha_record + 25] = bytes((141, 1, 32 if kind == "date-normalize" else 2,
                                                          3, 4, 5, offset & 255))
    elif kind in ("block-512", "block-1024", "block-512-tail"):
        block = 512 if kind.startswith("block-512") else 1024
        scale = 2048 // block
        for descriptor in descriptors:
            both16(image, descriptor + 128, block)
            both32(image, descriptor + 80, get32(descriptor + 80) * scale)
            both32(image, descriptor + 158, get32(descriptor + 158) * scale)
            for location, order in ((140, "<"), (144, "<"), (148, ">"), (152, ">")):
                old_table = get32(descriptor + location, order)
                struct.pack_into(order+"I", image, descriptor + location, old_table * scale)
                if old_table:
                    pos = old_table * 2048
                    end = pos + get32(descriptor + 132)
                    while pos < end:
                        length = image[pos]
                        struct.pack_into(order+"I", image, pos + 2, get32(pos + 2, order) * scale)
                        pos += 8 + length + length % 2
        for pos, record in root_records + nested_records:
            both32(image, pos + 2, get32(pos + 2) * scale)
        # Exercise padding at the logical block boundary, rather than 2048.
        moved = nested_records[-1][1]
        both32(moved, 2, struct.unpack_from("<I", moved, 2)[0] * scale)
        pos = nested_records[-1][0]
        image[pos:pos + len(moved)] = bytes(len(moved))
        image[nested + block:nested + block + len(moved)] = moved
        if kind == "block-512-tail":
            old_extent = root // block
            new_extent = len(image) // block
            for pos, _ in root_records + nested_records:
                if image[pos + 25] & 2 and get32(pos + 2) == old_extent:
                    both32(image, pos + 2, new_extent)
                    both32(image, pos + 10, block)
            both32(image, primary + 158, new_extent)
            both32(image, primary + 166, block)
            update_path_tables(old_extent, new_extent)
            append_data(image[root:root + block])
            update_volume()
    elif kind == "xar":
        old = get32(alpha_record + 2) * block
        new = append_data(b"Extended attribute record".ljust(block, b"\0") + image[old:old + len(alpha)])
        both32(image, alpha_record + 2, new)
        image[alpha_record + 1] = 1
        update_volume()
    elif kind == "xar-directory":
        old_extent = root // block
        new_extent = len(image) // block
        for pos, _ in root_records + nested_records:
            if image[pos + 25] & 2 and get32(pos + 2) == old_extent:
                both32(image, pos + 2, new_extent)
                image[pos + 1] = 1
        both32(image, primary + 158, new_extent)
        image[primary + 157] = 1
        update_path_tables(old_extent, new_extent, 1)
        append_data(bytes(block) + image[root:root + root_size])
        update_volume()
    elif kind in ("multi", "missing-final", "misaligned-section"):
        old = get32(alpha_record + 2) * block
        first = bytearray(image[alpha_record:alpha_record + image[alpha_record]])
        second = bytearray(first)
        first[25] |= 128
        both32(first, 10, block)
        both32(second, 2, append_data(image[old + block:old + len(alpha)]))
        both32(second, 10, len(alpha) - block)
        if kind == "missing-final":
            second[25] |= 128
        if kind == "misaligned-section":
            both32(first, 10, block - 1)
        entries = []
        for pos, record in root_records:
            entries += [first, second] if pos == alpha_record else [record]
        write_records(root, root_size, entries)
        update_volume()
    elif kind == "multi-directory":
        late = bytearray(image[alpha_record:alpha_record + image[alpha_record]])
        late[32] = len(b"LATE.BIN;1")
        late[33:] = b"LATE.BIN;1" + bytes((33 + len(b"LATE.BIN;1")) % 2)
        late[0] = len(late)
        payload = b"Second directory extent\n" * 89
        both32(late, 2, append_data(payload))
        both32(late, 10, len(payload))
        second_extent = append_data(late.ljust(block, b"\0"))
        first = bytearray(image[nested_record:nested_record + image[nested_record]])
        second = bytearray(first)
        first[25] |= 128
        both32(first, 10, block)
        both32(second, 2, second_extent)
        both32(second, 10, block)
        dot = bytearray(nested_records[0][1])
        dot_last = bytearray(dot)
        dot[25] |= 128
        both32(dot, 10, block)
        both32(dot_last, 2, second_extent)
        both32(dot_last, 10, block)
        write_records(nested, block, [dot, dot_last] + [record for _, record in nested_records[1:]])
        write_records(root, root_size, [record for pos, entry in root_records
                                       for record in ([first, second] if pos == nested_record else [entry])])
        files["NESTED/LATE.BIN"] = payload
        update_volume()
    elif kind in ("unknown-descriptor", "foreign-descriptor"):
        boot_descriptor = primary + 2048
        if kind == "unknown-descriptor":
            image[boot_descriptor] = 3
        else:
            image[boot_descriptor + 1:boot_descriptor + 6] = b"BEA01"
        files.pop("[BOOT]/Boot-NoEmul.img")
    elif kind == "no-terminator":
        image[terminator] = 3
    elif kind == "big-endian-mirror":
        struct.pack_into(">I", image, alpha_record + 6, 0x11223344)
    elif kind == "iso-hs-collision":
        image[primary + 9:primary + 14] = b"CDROM"
    elif kind == "unit-no-gap":
        image[alpha_record + 26] = 1
    elif kind == "interleave":
        image[alpha_record + 26:alpha_record + 28] = bytes((1, 1))
    elif kind == "outside-volume":
        both32(image, alpha_record + 2, append_data(alpha))
    elif kind == "xar-outside-volume":
        both32(image, alpha_record + 2, len(image) // block - 1)
        image[alpha_record + 1] = 255
    elif kind == "short-source":
        image = image[:get32(alpha_record + 2) * block + len(alpha) - 1]
    elif kind == "bad-directory":
        both32(image, nested_record + 2, len(image) // block + 1)
    elif kind == "cycle":
        both32(image, nested_record + 2, root // block)
    elif kind == "bad-name-length":
        image[alpha_record + 32] = 255
    elif kind == "short-record":
        image[alpha_record] = 12
    elif kind not in ("plain", "joliet"):
        raise ValueError(kind)
    return bytes(image), files


if __name__ == "__main__":
    image, files = volume(sys.argv[1] if len(sys.argv) > 1 else "plain")
    print(json.dumps({"size": len(image), "image": base64.b64encode(zlib.compress(image)).decode(),
                      "files": {name: base64.b64encode(data).decode() for name, data in files.items()}}))
