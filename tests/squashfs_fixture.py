"""Small v4 volumes for stream, metadata and malformed-block regressions."""
import base64
import json
import struct
import sys
import zlib

BLOCK = 8192
INVALID = (1 << 64) - 1


def creator_payload():
    return bytes(range(256)) * 1000 + bytes(131072) + b"tail"


def metadata(data, compressed=False):
    packed = zlib.compress(data) if compressed else data
    return struct.pack("<H", len(packed) | (0 if compressed else 0x8000)) + packed


def volume(fault="", compressed=False, no_fragments=False, codec="zlib", xattrs=False, export=False):
    if fault.startswith("inode-inventory-"):
        return inode_inventory_volume(fault)
    if fault == "legacy-large":
        image, payload = legacy_volume(large=True)
        return image, {"alpha": payload}
    if fault.startswith("blocklist-"):
        return blocklist_volume(fault)
    if fault.startswith("inode-block-"):
        return inode_metadata_volume(fault)
    if fault.startswith("directory-block-"):
        return directory_volume(fault)
    image = bytearray(96)
    first = b"A" * BLOCK + b"first-tail"
    if codec == "lz4":
        image += metadata(struct.pack("<II", 1, 0))
    elif codec == "lzo-rle":
        first = b"X" + bytes(BLOCK - 1) + b"first-tail"
    second = b"B" * BLOCK + bytes(BLOCK) + b"second-tail"
    healthy_second = fault in ("fragment-metadata-zero", "fragment-index-invalid", "metadata-combined",
                               "fragment-table-index-outside", "fragment-table-start-outside",
                               "fragment-count-large", "table-indexes-combined")
    if healthy_second:
        second = second[:BLOCK * 2]
    fragment = b"first-tailsecond-tail"
    descriptors = []
    for i, data in enumerate((first[:BLOCK], second[:BLOCK])):
        start = len(image)
        packed = zlib.compress(data) if compressed else data
        raw = not compressed
        if codec == "lzo-rle" and i == 0:
            # Linux LZO v1: initial literal, four zero runs, end marker.
            packed = bytes((17, 1, 18, ord('X')))
            for count in (2048, 2048, 2048, 2047):
                value = count - 4
                packed += bytes((0x18 | (value & 7), 0xfc, 0xff, value >> 3))
            packed += bytes((17, 0, 0))
            raw = False
        elif codec == "lz4":
            # One literal, offset-one match and the required five final literals.
            extra = BLOCK - 6 - 19
            packed = bytes((0x1f, data[0], 1, 0)) + bytes((255,)) * (extra // 255)
            packed += bytes((extra % 255, 0x50)) + data[-5:]
            raw = False
        image += packed
        descriptors.append((start, len(packed) | ((1 << 24) if raw else 0)))
    fragment_start = len(image)
    packed = zlib.compress(fragment) if compressed else fragment
    image += packed
    fragment_size = len(packed) | (0 if compressed else 1 << 24)
    alternate_fragment = None
    if fault == "fragment-remap":
        alternate_fragment = len(image)
        image += b"0123456789second-tail"
    inodes = bytearray()

    def base(kind, number):
        return struct.pack("<6HI", kind, 0o755, 0, 0, 0, 0, number)

    offsets = []
    for i, data in enumerate((first, second)):
        offsets.append(len(inodes))
        start, size = descriptors[i]
        if xattrs and i == 0:
            inodes += base(9, 1) + struct.pack("<QQQ4I", start, len(data), 0, 2, 0, 0, 0)
        else:
            inodes += base(2, i + 1)
            inodes += struct.pack("<4I", start, 0xffffffff if healthy_second and i == 1 else 0,
                                  0 if i == 0 else 10, len(data))
        inodes += struct.pack("<I", size)
        if i == 1:
            inodes += bytes(4)  # sparse block
    offsets.append(len(inodes))
    inodes += base(10 if fault == "inode-split-link" else 3, 3) + struct.pack("<2I", 1, 5) + b"alpha"
    if fault == "inode-split-link":
        inodes += struct.pack("<I", 0xffffffff)
    names = ["alpha", "данные 空", "link"]
    if xattrs:
        names.append("hardlink")
    directory = bytearray(struct.pack("<3I", len(names) - 1, 0, 1))
    entry_offsets = {}
    for i in sorted(range(len(names)), key=lambda i: names[i].encode()):
        entry_offsets[names[i]] = len(directory)
        name = names[i]
        name = name.encode()
        node = 0 if i == 3 else i
        directory += struct.pack("<4H", offsets[node], node, 3 if node == 2 else 2, len(name) - 1) + name
    root_offset = len(inodes)
    directory_index = fault.startswith("directory-index-")
    extended_root = fault == "inode-split-directory" or directory_index
    if extended_root:
        inodes += base(8, 4) + struct.pack("<IIIIHHI", 2, len(directory) + 3, 0, 1, 1, 0, 0xffffffff)
        inodes += struct.pack("<III", 0, 0, 0) + b"a"
        if fault in ("directory-index-name", "directory-index-inline-name"):
            struct.pack_into("<I", inodes, root_offset + 48, 0xffffffff)
        elif fault == "directory-index-count":
            struct.pack_into("<H", inodes, root_offset + 32, 0xffff)
    else:
        inodes += base(1, 4) + struct.pack("<2I2HI", 0, 2, len(directory) + 3, 0, 1)
    unused_inode = fault in ("inode-unused-fragment", "inode-unused-owner", "inode-unused-xattr")
    if unused_inode:
        unused_offset = len(inodes)
        start, size = descriptors[0]
        if fault == "inode-unused-xattr":
            inodes += base(9, 5) + struct.pack("<QQQ4I", start, len(first), 0, 1, 0, 0, 1)
        else:
            record = bytearray(base(2, 5))
            if fault == "inode-unused-owner":
                struct.pack_into("<HH", record, 4, 2048, 2048)
            inodes += record + struct.pack("<4I", start, 512 if fault == "inode-unused-fragment" else 0, 0, len(first))
        inodes += struct.pack("<I", size)
    if fault == "inode-unused":
        inodes += base(0xffff, 5) + bytes(4)
    split_inodes = fault.startswith("inode-split") or (directory_index and "-inline-" not in fault)

    def inode_ref(offset):
        return (((offset // 17) * 19) << 16) + offset % 17 if split_inodes else offset

    if split_inodes:
        directory = bytearray()
        for i in sorted(range(len(names)), key=lambda i: names[i].encode()):
            name = names[i].encode()
            node = 0 if i == 3 else i
            reference = inode_ref(offsets[node])
            inode_offset = reference & 0xffff
            if fault.startswith("directory-index-recovery") and i == 0:
                inode_offset = 0xffff
            directory += struct.pack("<IIIHHHH", 0, reference >> 16, node + 1,
                                     inode_offset, 0, 3 if node == 2 else 2, len(name) - 1) + name
        struct.pack_into("<I" if extended_root else "<H", inodes,
                         root_offset + (20 if extended_root else 24), len(directory) + 3)
    inode_start = len(image)
    if split_inodes:
        for offset in range(0, len(inodes), 17):
            last = offset + 17 >= len(inodes)
            packed = bytearray(metadata(inodes[offset:offset + 17], last and fault == "directory-index-zlib"))
            if last and fault in ("directory-index-header", "directory-index-recovery-bad"):
                struct.pack_into("<H", packed, 0, 0x8000)
            elif last and fault == "directory-index-zlib":
                packed[-1] ^= 0xff
            image += packed
    else:
        image += metadata(inodes, compressed)
    dir_start = len(image)
    image += metadata(directory, compressed)
    fragment_metadata = len(image)
    if no_fragments:
        # A valid empty filesystem with no fragment index at all.
        inodes = base(1, 1) + struct.pack("<2I2HI", 0, 2, 3, 0, 1)
        image = image[:inode_start] + metadata(inodes, compressed)
        dir_start = len(image)
        root_offset = 0
        fragment_table = INVALID
    else:
        image += metadata(struct.pack("<QII", alternate_fragment if alternate_fragment is not None else fragment_start,
                                      fragment_size, 0), compressed)
        fragment_count = 1
        fragment_indexes = [fragment_metadata]
        if fault in ("fragment-unused-block", "fragment-remap", "inode-unused-fragment"):
            fragment_count = 513
            fragment_indexes.append(len(image))
            image += (metadata(struct.pack("<QII", fragment_start, fragment_size, 0))
                      if fault == "fragment-remap" else struct.pack("<H", 0x8000))
        elif fault == "fragment-high-index":
            fragment_count = (1 << 28) + 1
            fragment_indexes *= (fragment_count + 511) // 512
        fragment_table = len(image)
        image += b"".join(struct.pack("<Q", block) for block in fragment_indexes)
    lookup_table = INVALID
    if export:
        lookup_metadata = len(image)
        refs = offsets + [root_offset] + ([unused_offset] if unused_inode else [])
        image += metadata(struct.pack("<" + "Q" * len(refs), *(inode_ref(ref) for ref in refs)))
        lookup_table = len(image)
        image += struct.pack("<Q", lookup_metadata)
    ids_metadata = len(image)
    image += metadata(struct.pack("<I", 1000), compressed)
    id_indexes = [ids_metadata]
    if fault == "inode-unused-owner":
        id_indexes.append(len(image))
        image += struct.pack("<H", 0x8000)
    ids_table = len(image)
    image += b"".join(struct.pack("<Q", block) for block in id_indexes)
    xattr_id_table = INVALID
    if xattrs:
        attributes = [(0, b"inline", b"v\0a\0lue"), (2, b"label", b"test-label"), (0, b"empty", b"")]
        entries = bytearray()
        for kind, name, value in attributes:
            entries += struct.pack("<HH", kind, len(name)) + name
            entries += struct.pack("<I", len(value)) + value
        references = []
        for kind, name in ((1, b"large"), (0, b"duplicate")):
            entries += struct.pack("<HH", kind | 256, len(name)) + name + struct.pack("<I", 8)
            references.append(len(entries))
            entries += bytes(8)
        entries += struct.pack("<HH", 0x8000, 6) + b"future" + struct.pack("<I", 1) + b"?"
        pointer = ((len(entries) // BLOCK * (BLOCK + 2)) << 16) | (len(entries) % BLOCK)
        for reference in references:
            struct.pack_into("<Q", entries, reference, pointer)
        value = bytes(range(251)) * 36
        entries += struct.pack("<I", len(value)) + value
        xattr_start = len(image)
        for offset in range(0, len(entries), BLOCK):
            image += metadata(entries[offset:offset + BLOCK])
        unused_xattr = None
        if fault == "inode-unused-xattr":
            unused_xattr = (len(image) - xattr_start) << 16
            image += metadata(struct.pack("<HH", 0, 6) + b"unused" + struct.pack("<I", 6) + b"unused")
        xattr_ids_metadata = len(image)
        xattr_ids = struct.pack("<QII", 0, len(attributes) + 3, len(entries))
        if unused_xattr is not None:
            xattr_ids += struct.pack("<QII", unused_xattr, 1, 20)
        image += metadata(xattr_ids)
        xattr_id_table = len(image)
        image += struct.pack("<QIIQ", xattr_start, 2 if unused_xattr is not None else 1, 0, xattr_ids_metadata)
    struct.pack_into("<5I6H8Q", image, 0, 0x73717368, 1 if no_fragments else 5 if fault == "inode-unused" or unused_inode else 4,
                     0, BLOCK, 0 if no_fragments else fragment_count, 5 if codec == "lz4" else 3 if codec == "lzo-rle" else 1,
                     13, 1 << 10 if codec == "lz4" else 0, 2049 if fault == "inode-unused-owner" else 1, 4, 0,
                     inode_ref(root_offset), len(image), ids_table, xattr_id_table, inode_start, dir_start,
                     fragment_table, lookup_table)
    # Structural mutations use uncompressed metadata so each targets a single boundary.
    if fault == "fragment-large":
        struct.pack_into("<I", image, fragment_metadata + 2 + 8, BLOCK + 1)
    elif fault == "fragment-zero":
        struct.pack_into("<I", image, fragment_metadata + 2 + 8, 1 << 24)
    elif fault == "data-zero":
        struct.pack_into("<I", image, inode_start + 2 + 32, 1 << 24)
    elif fault == "fragment-outside":
        struct.pack_into("<Q", image, fragment_metadata + 2, len(image) - 1)
    elif fault == "data-outside":
        struct.pack_into("<I", image, inode_start + 2 + 16, len(image) - 1)
    elif fault == "cycle":
        struct.pack_into("<H", image, dir_start + 2 + 12, root_offset)
    elif fault == "count-wrap":
        struct.pack_into("<I", image, dir_start + 2, 0xffffffff)
    elif fault == "name-nul":
        image[dir_start + 2 + 20] = 0
    elif fault == "name-slash":
        image[dir_start + 2 + 20] = ord('/')
    elif fault == "root-not-directory":
        struct.pack_into("<Q", image, 32, 0)
    elif fault == "uid-index":
        struct.pack_into("<H", image, inode_start + 2 + 4, 1)
    elif fault == "gid-index":
        struct.pack_into("<H", image, inode_start + 2 + 6, 1)
    elif fault == "inode-record":
        struct.pack_into("<H", image, inode_start + 2, 0xffff)
    elif fault == "inode-root":
        struct.pack_into("<H", image, inode_start + 2 + root_offset, 0xffff)
    elif fault == "inode-count-short":
        struct.pack_into("<I", image, 4, 3)
    elif fault == "inode-count-large":
        struct.pack_into("<I", image, 4, 0xffffffff)
    elif fault == "inode-length":
        # An earlier inode claims bytes belonging to later records. Root and
        # directory references still identify those later records independently.
        blocks = (len(inodes) - 56) // 4
        struct.pack_into("<Q", image, inode_start + 2 + 24, blocks * BLOCK + 10)
    elif fault in ("directory-reference", "directory-unavailable", "directory-ambiguous"):
        struct.pack_into("<H", image, dir_start + 2 + entry_offsets["alpha"], 0xffff)
        if fault == "directory-unavailable":
            struct.pack_into("<H", image, dir_start + 2 + entry_offsets["alpha"] + 2, 4)
        elif fault == "directory-ambiguous":
            struct.pack_into("<I", image, inode_start + 2 + offsets[1] + 12, 1)
    elif fault == "directory-tail":
        struct.pack_into("<H", image, dir_start + 2 + entry_offsets["данные 空"] + 6, 0xffff)
    elif fault == "id-metadata-zero":
        struct.pack_into("<H", image, ids_metadata, 0x8000)
    elif fault in ("fragment-metadata-zero", "metadata-combined"):
        struct.pack_into("<H", image, fragment_metadata, 0x8000)
        if fault == "metadata-combined":
            struct.pack_into("<H", image, inode_start + 2 + 4, 1)
    elif fault == "fragment-index-invalid":
        struct.pack_into("<I", image, inode_start + 2 + (44 if xattrs else 20), 0xfffffffe)
    elif fault == "negative-size":
        struct.pack_into("<Q", image, 40, 1 << 63)
    elif fault == "xattr-ids-zero":
        struct.pack_into("<I", image, xattr_id_table + 8, 0)
    elif fault == "xattr-count-negative":
        struct.pack_into("<I", image, xattr_ids_metadata + 2 + 8, 0x80000000)
    elif fault == "xattr-index-outside":
        struct.pack_into("<Q", image, xattr_id_table + 16, len(image))
    elif fault == "xattr-id-invalid":
        struct.pack_into("<I", image, inode_start + 2 + 52, 1)
    elif fault == "xattr-ool-outside":
        struct.pack_into("<Q", image, xattr_start + 2 + references[0], INVALID)
    elif fault == "export-reference":
        struct.pack_into("<Q", image, lookup_metadata + 2, INVALID)
    elif fault == "export-metadata-zero":
        struct.pack_into("<H", image, lookup_metadata, 0x8000)
    elif fault == "export-compressed-invalid":
        struct.pack_into("<H", image, lookup_metadata, 32)
        image[lookup_metadata + 2:lookup_metadata + 34] = bytes(32)
    elif fault == "export-index-outside":
        struct.pack_into("<Q", image, lookup_table, len(image))
    elif fault in ("id-table-index-outside", "fragment-table-index-outside"):
        struct.pack_into("<Q", image, ids_table if fault.startswith("id-") else fragment_table, len(image))
    elif fault in ("id-table-start-outside", "fragment-table-start-outside", "xattr-table-start-outside", "export-table-start-outside"):
        field = {"id-table-start-outside": 48, "fragment-table-start-outside": 80,
                 "xattr-table-start-outside": 56, "export-table-start-outside": 88}[fault]
        struct.pack_into("<Q", image, field, len(image) + 1)
    elif fault == "table-indexes-combined":
        for table in (ids_table, fragment_table, lookup_table,
                      xattr_id_table + 16 if xattrs else INVALID):
            if table != INVALID:
                struct.pack_into("<Q", image, table, len(image))
    elif fault == "fragment-count-large":
        struct.pack_into("<I", image, 16, 0xffffffff)
    elif fault in ("fragment-high-index", "fragment-remap"):
        struct.pack_into("<I", image, inode_start + 2 + (44 if xattrs else 20),
                         (1 << 28) if fault == "fragment-high-index" else 512)
    elif fault == "id-count-zero":
        struct.pack_into("<H", image, 26, 0)
    elif fault == "id-metadata-gap":
        image[ids_table:ids_table] = bytes(BLOCK)
        struct.pack_into("<Q", image, 48, ids_table + BLOCK)
        struct.pack_into("<Q", image, 40, len(image))
        if xattrs:
            struct.pack_into("<Q", image, 56, xattr_id_table + BLOCK)
            struct.pack_into("<Q", image, xattr_id_table + BLOCK, xattr_start + BLOCK)
            struct.pack_into("<Q", image, xattr_id_table + BLOCK + 16, xattr_ids_metadata + BLOCK)
    elif fault in ("metadata-bomb", "cross-block-inode"):
        if fault == "metadata-bomb":
            # The inode block expands beyond the 8 KiB metadata contract.
            packed = metadata(inodes + bytes(BLOCK), True)
        else:
            # Directory/root offsets refer beyond the first short metadata block.
            # Concatenating metadata must not make those invalid references valid.
            packed = metadata(inodes[:36]) + metadata(inodes[36:])
        end = dir_start
        delta = len(packed) - (end - inode_start)
        image[inode_start:end] = packed
        for field, value in ((40, len(image)), (48, ids_table + delta),
                             (72, dir_start + delta), (80, fragment_table + delta)):
            struct.pack_into("<Q", image, field, value)
        struct.pack_into("<Q", image, fragment_table + delta, fragment_metadata + delta)
        struct.pack_into("<Q", image, ids_table + delta, ids_metadata + delta)
    image += bytes((-len(image)) % 4096)
    return bytes(image), dict(zip(names, (first, second, b"alpha", first))) if not no_fragments else {}


def inode_inventory_volume(fault):
    image = bytearray(96)
    payload = b"X" if fault == "inode-inventory-overlap" else b"B" * 333
    image += payload

    def base(kind, number):
        return struct.pack("<HHHHII", kind, 0o755 if kind == 1 else 0o644,
                           0, 0, 1700000000 + number, number)

    def regular(number, size, descriptors):
        return base(2, number) + struct.pack("<IIII", 96, 0xffffffff, 0, size) + descriptors

    if fault == "inode-inventory-overlap":
        fake = regular(2, 1, struct.pack("<I", (1 << 24) | 1))
        first = regular(1, 9 * BLOCK, fake)
        root_offset = len(first)
        entries = [("alpha", 0, 1, 2), ("observed", 32, 2, 2), ("recover", 0xffff, 2, 2)]
        inodes = bytearray(first)
        files = {"observed": payload}
        count, root_number = 2, 3
    else:
        first = regular(1, 20000 * BLOCK + 7, bytes(20001 * 4))
        inodes = bytearray(first)
        entries = [("alpha", 0 if fault.endswith("-clean") else 0xffff, 1, 2)]
        entries.append(("данные 空", len(inodes), 2, 2))
        inodes += regular(2, len(payload), struct.pack("<I", (1 << 24) | len(payload)))
        target = b"al\0pha" + b"T" * 3000
        other = "данные 空".encode()
        for name, number, body in (("link", 3, target), ("otherlink", 4, other)):
            entries.append((name, len(inodes), number, 3))
            inodes += base(10, number) + struct.pack("<II", 1, len(body)) + body
            inodes += struct.pack("<I", 0xffffffff)
        root_offset = len(inodes)
        files = {"данные 空": payload, "link": target, "otherlink": other}
        count, root_number = 5, 5
    inodes += base(1, root_number) + struct.pack("<IIHHI", 0, 2, 3, 0, root_number)
    chunks, positions = [], []
    position = 0
    for offset in range(0, len(inodes), BLOCK):
        positions.append(position)
        packed = metadata(inodes[offset:offset + BLOCK], True)
        chunks.append(packed)
        position += len(packed)

    def reference(offset):
        return (positions[offset // BLOCK] << 16) | (offset % BLOCK)

    directory = bytearray()
    for name, offset, number, kind in sorted(entries):
        encoded = name.encode()
        ref = reference(offset) if offset != 0xffff else 0xffff
        directory += struct.pack("<IIIHHHH", 0, ref >> 16, number, ref & 0xffff,
                                 0, kind, len(encoded) - 1) + encoded
    struct.pack_into("<H", inodes, root_offset + 24, len(directory) + 3)
    # Only the root size changed; block starts before its final metadata block remain stable.
    chunks[-1] = metadata(inodes[(len(chunks) - 1) * BLOCK:], True)
    inode_start = len(image)
    image += b"".join(chunks)
    directory_start = len(image)
    image += metadata(directory)
    ids_metadata = len(image)
    image += metadata(struct.pack("<I", 1000))
    ids_table = len(image)
    image += struct.pack("<Q", ids_metadata)
    struct.pack_into("<5I6H8Q", image, 0, 0x73717368, count, 0, BLOCK, 0, 1, 13, 0, 1, 4, 0,
                     reference(root_offset), len(image), ids_table, INVALID, inode_start,
                     directory_start, INVALID, INVALID)
    image += bytes((-len(image)) % 4096)
    return bytes(image), files


def blocklist_volume(fault):
    image = bytearray(96)
    first = b"A" * BLOCK + b"B" * BLOCK + bytes(BLOCK) + b"tailend"
    second = b"healthy" * 47
    first_start = len(image)
    image += first[:BLOCK * 2] + first[-7:]
    second_start = len(image)
    image += second

    def base(kind, number):
        return struct.pack("<HHHHII", kind, 0o755 if kind == 1 else 0o644, 0, 0, 0, number)

    header = base(2, 1) + struct.pack("<IIII", first_start, 0xffffffff, 0, len(first))
    prefix = metadata(header) + metadata(struct.pack("<I", (1 << 24) | BLOCK))
    tail = bytearray(metadata(struct.pack("<III", (1 << 24) | BLOCK, 0, (1 << 24) | 7), True))
    if fault == "blocklist-header":
        struct.pack_into("<H", tail, 0, 0x8000)
    elif fault == "blocklist-zlib":
        tail[-1] ^= 0xff
    healthy = base(2, 2) + struct.pack("<IIIII", second_start, 0xffffffff, 0, len(second),
                                     (1 << 24) | len(second))
    healthy_block = len(prefix) + len(tail)
    directory = bytearray()
    for name, block, number in (("alpha", 0, 1), ("данные 空", healthy_block, 2)):
        encoded = name.encode()
        directory += struct.pack("<IIIHHHH", 0, block, number, 0, 0, 2, len(encoded) - 1) + encoded
    root = base(1, 3) + struct.pack("<IIHHI", 0, 2, len(directory) + 3, 0, 3)
    inode_start = len(image)
    image += prefix + tail + metadata(healthy + root)
    directory_start = len(image)
    image += metadata(directory)
    ids_metadata = len(image)
    image += metadata(struct.pack("<I", 1000))
    ids_table = len(image)
    image += struct.pack("<Q", ids_metadata)
    struct.pack_into("<5I6H8Q", image, 0, 0x73717368, 3, 0, BLOCK, 0, 1, 13, 0, 1, 4, 0,
                     (healthy_block << 16) | len(healthy), len(image), ids_table, INVALID,
                     inode_start, directory_start, INVALID, INVALID)
    image += bytes((-len(image)) % 4096)
    return bytes(image), {"alpha": first, "данные 空": second}


def inode_metadata_volume(fault):
    image = bytearray(96)
    first, second = b"A" * 237, b"B" * 333
    starts = (len(image), len(image) + len(first))
    image += first + second

    def base(kind, number):
        return struct.pack("<HHHHII", kind, 0o755 if kind == 1 else 0o644, 0, 0, 0, number)

    records = [base(2, i + 1) + struct.pack("<IIIII", starts[i], 0xffffffff, 0, len(body),
                                           (1 << 24) | len(body))
               for i, body in enumerate((first, second))]
    # A valid file record crosses a block boundary. A failure in its blocklist
    # must not borrow bytes from the healthy inode after the gap.
    prefix = metadata(records[0][:32])
    broken = bytearray(metadata(records[0][32:], fault not in ("inode-block-clean", "inode-block-io", "inode-block-unused")))
    good = metadata(records[1], True)
    if fault == "inode-block-header":
        struct.pack_into("<H", broken, 0, 0x8000)
    elif fault not in ("inode-block-clean", "inode-block-io", "inode-block-unused"):
        broken[-1] ^= 0xff
    good_position = len(prefix) + len(broken)
    directory = bytearray()
    for name, block, number in (("alpha", 0, 1), ("данные 空", good_position, 2)):
        encoded = name.encode()
        directory += struct.pack("<IIIHHHH", 0, block, number, 0, 0, 2, len(encoded) - 1) + encoded
    root = metadata(base(1, 3) + struct.pack("<IIHHI", 0, 2, len(directory) + 3, 0, 3), True)
    inode_start = len(image)
    root_position = good_position + len(good)
    if fault == "inode-block-root":
        struct.pack_into("<H", root := bytearray(root), 0, 0x8000)
    image += prefix + broken + good + root
    if fault == "inode-block-unused":
        image += struct.pack("<H", 0x8000)
    directory_start = len(image)
    image += metadata(directory)
    ids_metadata = len(image)
    image += metadata(struct.pack("<I", 1000))
    ids_table = len(image)
    image += struct.pack("<Q", ids_metadata)
    struct.pack_into("<5I6H8Q", image, 0, 0x73717368, 3, 0, BLOCK, 0, 1, 13, 0, 1, 4, 0,
                     root_position << 16, len(image), ids_table, INVALID, inode_start,
                     directory_start, INVALID, INVALID)
    image += bytes((-len(image)) % 4096)
    return bytes(image), {"alpha": first, "данные 空": second}


def directory_volume(fault):
    image = bytearray(96)
    first, second = b"A" * 237, b"B" * 333
    first_start = len(image)
    image += first
    second_start = len(image)
    image += second
    index_recovery = fault == "directory-block-index-recovery"
    offsets = (0, 36, 72, 125, 157) if index_recovery else (0, 36, 72, 104, 136)

    def entries(items):
        result = bytearray(struct.pack("<III", len(items) - 1, 0, 1))
        for name, node, kind in items:
            encoded = name.encode()
            offset = 0xffff if index_recovery and name == "alpha" else offsets[node]
            result += struct.pack("<HHHH", offset, node, kind, len(encoded) - 1) + encoded
        return bytes(result)

    root = entries((("alpha", 0, 2), ("broken", 2, 1), ("good 空", 3, 1)))
    child = entries((("payload", 1, 2),))
    root_block, good_block = metadata(root), metadata(child, True)
    bad_block = bytearray(metadata(child, fault not in ("directory-block-clean", "directory-block-io", "directory-block-unused", "directory-block-index-recovery")))
    shared = fault == "directory-block-shared"
    if fault in ("directory-block-short", "directory-block-short-bad"):
        def short_blocks(data):
            return b"".join(metadata(data[offset:offset + 7], offset % 2 != 0)
                            for offset in range(0, len(data), 7))
        root_block = short_blocks(root)
        bad_block = short_blocks(child)
        good_block = short_blocks(child)
        if fault == "directory-block-short-bad":
            # Only the tail of the final root filename is unavailable.
            tail = len(root) // 7 * 7
            root_block = short_blocks(root[:tail]) + struct.pack("<H", 0x8000)
    elif shared:
        half = len(child) // 2
        root_block = metadata(root + child[:half])
        bad_block = bytearray()
        good_block = metadata(child[half:] + child, True)
    elif fault in ("directory-block-header", "directory-block-inode"):
        struct.pack_into("<H", bad_block, 0, 0x8000)
    elif fault not in ("directory-block-clean", "directory-block-io", "directory-block-unused", "directory-block-index-recovery"):
        bad_block[-1] ^= 0xff  # Output can precede the failing Adler checksum.

    def base(kind, number):
        return struct.pack("<HHHHII", kind, 0o755 if kind in (1, 8) else 0o644, 0, 0, 0, number)

    inodes = bytearray()
    for number, start, body in ((1, first_start, first), (2, second_start, second)):
        inodes += base(2, number) + struct.pack("<IIIII", start, 0xffffffff, 0, len(body), (1 << 24) | len(body))
    for number, start, size, offset in ((3, 0 if shared else len(root_block), len(child), len(root) if shared else 0),
                                       (4, len(root_block) + len(bad_block), len(child), len(child) - half if shared else 0),
                                       (5, 0, len(root) + (len(child) if fault == "directory-block-span" else 0), 0)):
        if index_recovery and number == 3:
            inodes += base(8, number) + struct.pack("<IIIIHHI", 2, size + 3, start, 5, 1, offset, 0xffffffff)
            inodes += struct.pack("<III", 0, start, 256) + b"a"
        else:
            inodes += base(1, number) + struct.pack("<IIHHI", start, 2, size + 3, offset, 5)
    if fault == "directory-block-inode":
        struct.pack_into("<H", inodes, 0, 0xffff)
    inode_start = len(image)
    image += metadata(inodes)
    directory_start = len(image)
    image += root_block + bad_block + good_block
    if fault == "directory-block-unused":
        image += struct.pack("<H", 0x8000)
    ids_metadata = len(image)
    image += metadata(struct.pack("<I", 1000))
    ids_table = len(image)
    image += struct.pack("<Q", ids_metadata)
    struct.pack_into("<5I6H8Q", image, 0, 0x73717368, 5, 0, BLOCK, 0, 1, 13, 0, 1, 4, 0,
                     offsets[4], len(image), ids_table, INVALID, inode_start, directory_start, INVALID, INVALID)
    image += bytes((-len(image)) % 4096)
    return bytes(image), {"alpha": first, "broken/payload": second, "good 空/payload": second}


def legacy_volume(endian="<", compressed=False, large=False):
    """v3 remains a 7-Zip read extension; current Linux only mounts v4."""
    image = bytearray(119)
    payload = b"legacy" + bytes(range(256)) * 3
    if large:
        payload = b"L" * (BLOCK * 3 + 7)
    image += payload

    def base(kind, number):
        mode = (kind << 12 | 0o755) if endian == ">" else (0o755 << 4 | kind)
        return struct.pack(endian + "HBBII", mode, 0, 255, 0, number)

    file_inode = base(2, 1) + struct.pack(endian + "QIII", 119, 0xffffffff, 0, len(payload))
    for offset in range(0, len(payload), BLOCK):
        file_inode += struct.pack(endian + "I", (1 << 24) | len(payload[offset:offset + BLOCK]))
    root_offset = len(file_inode)
    entry = (2 if endian == ">" else 2 << 13)
    directory = bytes(1) + struct.pack(endian + "IIHBH", 0, 1, entry, 4, 0) + b"alpha"
    root_inode = base(1, 2) + struct.pack(endian + "IIII", 2,
                        ((len(directory) + 3) << 13) if endian == ">" else len(directory) + 3, 0, 1)

    def block(data):
        packed = zlib.compress(data) if compressed else data
        return struct.pack(endian + "H", len(packed) | (0 if compressed else 0x8000)) + packed

    inode_start = len(image)
    image += block(file_inode + root_inode)
    dir_start = len(image)
    image += block(directory)
    ids_start = len(image)
    image += struct.pack(endian + "I", 1000)
    image[:4] = b"sqsh" if endian == ">" else b"hsqs"
    struct.pack_into(endian + "I", image, 4, 2)
    struct.pack_into(endian + "4H", image, 28, 3, 0, BLOCK, 13)
    image[37] = 1  # one UID, GID 255 reuses the inode UID
    struct.pack_into(endian + "Q", image, 43, root_offset)
    struct.pack_into(endian + "II", image, 51, BLOCK, 0)
    struct.pack_into(endian + "7Q", image, 63, len(image), ids_start, 0, inode_start,
                     dir_start, ids_start, INVALID)
    image += bytes((-len(image)) % 4096)
    return bytes(image), payload


if __name__ == "__main__":
    codec = next((arg.split("=", 1)[1] for arg in sys.argv if arg.startswith("--codec=")), "zlib")
    fault = next((arg.split("=", 1)[1] for arg in sys.argv if arg.startswith("--fault=")), "")
    data, files = volume(fault=fault, compressed="--compressed" in sys.argv, codec=codec, xattrs="--xattrs" in sys.argv,
                         export="--export" in sys.argv)
    print(json.dumps({"size": len(data), "image": base64.b64encode(zlib.compress(data)).decode(),
                      "files": {name: base64.b64encode(value).decode() for name, value in files.items()}}))
