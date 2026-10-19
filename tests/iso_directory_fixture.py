"""Directory inode mutations of native ISO/Rock Ridge records."""
import struct


def volume(kind):
    from iso_fixture import volume as native_volume
    rock = kind == "directory-metadata"
    image, files = native_volume("rock" if rock else "plain")
    image = bytearray(image)
    block = 2048
    primary = 16 * block

    def number(pos):
        return struct.unpack_from("<I", image, pos)[0]

    def both(buffer, pos, value):
        struct.pack_into("<I", buffer, pos, value)
        struct.pack_into(">I", buffer, pos + 4, value)

    def records(start, size):
        pos = start
        result = []
        while pos < start + size:
            length = image[pos]
            if not length:
                pos = (pos // block + 1) * block
                continue
            result.append((pos, bytearray(image[pos:pos + length])))
            pos += length
        return result

    def pack(entries, size):
        data = bytearray(size)
        pos = 0
        for entry in entries:
            if len(entry) > block - pos % block:
                pos = (pos // block + 1) * block
            assert pos + len(entry) <= size
            data[pos:pos + len(entry)] = entry
            pos += len(entry)
        return data

    def append(data):
        extent = len(image) // block
        image.extend(data)
        image.extend(bytes((-len(image)) % block))
        return extent

    root = number(primary + 158) * block
    root_size = number(primary + 166)
    root_entries = records(root, root_size)
    parent, entry = next((p, r) for p, r in root_entries
                         if r[33:33 + r[32]] == b"NESTED")
    child = number(parent + 2) * block
    child_size = number(parent + 10)
    if kind in ("directory-root-size", "directory-parent-size"):
        both(image, primary + 166 if kind == "directory-root-size" else parent + 10, 0)
    elif kind == "directory-root-flag":
        image[primary + 181] &= ~2
    elif kind == "directory-metadata":
        def px(pos):
            offset = (33 + image[pos + 32] + 1) & ~1
            while offset + 4 <= image[pos] and image[pos + offset + 2] >= 4:
                if image[pos + offset:pos + offset + 2] == b"PX":
                    return pos + offset
                offset += image[pos + offset + 2]
            raise AssertionError("native PX record missing")
        # The parent still supplies NM; its PX must not replace the dot inode.
        both(image, px(parent) + 4, 0o100600)
        own = px(child)
        for offset, value in ((4, 0o40750), (12, 7), (20, 2345), (28, 3456)):
            both(image, own + offset, value)
    elif kind in ("directory-sections-limit", "directory-sections-overflow"):
        count = 101 if kind.endswith("limit") else 102
        own = records(child, child_size)[0][1]
        chain = []
        for i in range(count):
            section = bytearray(own)
            section[25] = 2 | (128 if i + 1 < count else 0)
            both(section, 2, child // block if i == 0 else append(bytes(block)))
            both(section, 10, block)
            chain.append(section)
        both(image, parent + 2, append(pack(chain, 2 * block)))
    elif kind.startswith("directory-multi"):
        is_root = "root" in kind
        start, size = (root, root_size) if is_root else (child, child_size)
        original = records(start, size)
        dot = bytearray(original[0][1])
        terminal = bytearray(dot)
        dot[25] |= 128
        both(dot, 10, block)
        late = bytearray(next(r for _, r in root_entries if r[33:33 + r[32]] == b"ALPHA.BIN;1"))
        late[32] = len(b"LATE.BIN;1")
        late[33:] = b"LATE.BIN;1" + bytes((33 + len(b"LATE.BIN;1")) % 2)
        late[0] = len(late)
        payload = b"Noncontiguous directory inode section\n" * 83
        both(late, 2, append(payload))
        both(late, 10, len(payload))
        # Leave a physical gap between the two directory data sections.
        append(b"Unmapped section gap\n".ljust(block, b"\0"))
        extent = append(pack([late], block))
        both(terminal, 2, extent)
        both(terminal, 10, block)
        if kind.endswith("outside"):
            both(terminal, 2, len(image) // block)
        elif kind.endswith("unaligned"):
            both(dot, 10, block - 1)
        elif kind.endswith("unfinished"):
            terminal[25] |= 128
        elif kind.endswith("bad-dot"):
            terminal[33] = 1
        if kind.endswith("padding"):
            # Zero padding between the inode records exercises the kernel scan.
            first = pack([dot], block)
            chain_extent = append(first + pack([terminal] + [r for _, r in original[1:]], block))
            both(image, parent + 2, chain_extent)
        elif kind.endswith("split"):
            # Valid records fill the block up to offset 2020; a dot record
            # crossing the block boundary must be assembled before decoding.
            fillers = []
            remaining = block - 28 - len(dot)
            while remaining:
                length = min(254, remaining)
                if remaining - length in range(1, 34):
                    length -= 34 - (remaining - length)
                record = bytearray(original[1][1][:34])
                record.extend(bytes(length - len(record)))
                record[0] = length
                record[25] |= 128
                both(record, 2, append(bytes(block)))
                both(record, 10, block)
                record[33] = 0
                fillers.append(record)
                remaining -= length
            # Intermediate sections contain padding only. Kernel accepts the
            # terminal split record without treating it as a short inode.
            data = dot + b"".join(fillers) + terminal
            assert len(data) > block
            chain_extent = append(data.ljust(2 * block, b"\0"))
            both(image, parent + 2, chain_extent)
            # Data remains in the original child block; replace its dot chain
            # with ordinary dot entries for directory enumeration.
        else:
            image[start:start + block] = pack([dot, terminal] + [r for _, r in original[1:]], block)
        files[("" if is_root else "NESTED/") + "LATE.BIN"] = payload
    elif kind in ("directory-bad-dot", "directory-file-dot"):
        image[child + 33 if kind == "directory-bad-dot" else child + 25] = 1 if kind == "directory-bad-dot" else 0
    else:
        raise ValueError(kind)
    both(image, primary + 80, len(image) // block)
    return bytes(image), files
