"""Rock Ridge producer layouts over native generated directory records."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import struct


CASES = ("producer-xa", "producer-skip", "producer-nm-parts", "producer-nm-nul",
         "producer-nm-overflow", "producer-sl-parts", "producer-sl-root",
         "producer-tf-short", "producer-tf-truncated", "producer-tf-malformed-fraction")


def volume(kind):
    from tests.formats.iso.support.iso_fixture import volume as native_volume
    if kind == "producer-directory-ce-outside":
        original, files = volume("producer-xa")
        image = bytearray(original)
        root = struct.unpack_from("<I", image, 16 * 2048 + 158)[0] * 2048
        pos = root
        while bytes(image[pos + 33:pos + 33 + image[pos + 32]]) != b"NESTED":
            assert pos < root + struct.unpack_from("<I", image, 16 * 2048 + 166)[0]
            pos = pos + image[pos] if image[pos] else (pos // 2048 + 1) * 2048
        ce = pos + ((33 + image[pos + 32] + 1) & ~1) + 14
        assert image[ce:ce + 2] == b"CE"
        struct.pack_into("<I", image, ce + 4, len(image) // 2048)
        struct.pack_into(">I", image, ce + 8, len(image) // 2048)
        return bytes(image), files
    original, files = native_volume("rock")
    image = bytearray(original)
    block = 2048
    primary = 16 * block
    def number(pos):
        return struct.unpack_from("<I", image, pos)[0]
    def both(buffer, pos, value):
        struct.pack_into("<I", buffer, pos, value)
        struct.pack_into(">I", buffer, pos + 4, value)
    def entries(start, size):
        pos = start
        result = []
        while pos < start + size:
            length = image[pos]
            if not length:
                pos = (pos // block + 1) * block
                continue
            result.append(bytearray(image[pos:pos + length]))
            pos += length
        return result
    def start_su(record):
        return (33 + record[32] + 1) & ~1
    def tags(record):
        pos = start_su(record)
        while pos + 4 <= len(record) and record[pos + 2] >= 4:
            length = record[pos + 2]
            assert pos + length <= len(record)
            yield bytes(record[pos:pos + length])
            pos += length
    def append(data):
        extent = len(image) // block
        image.extend(data)
        image.extend(bytes((-len(image)) % block))
        return extent
    def replace_su(record, data, prefix=b""):
        # Continuations keep long/changed attributes within one logical block,
        # without depending on room inside the original directory record.
        ce = bytearray(b"CE\x1c\x01" + bytes(24))
        both(ce, 4, append(data))
        both(ce, 20, len(data))
        result = record[:start_su(record)] + prefix + ce
        assert len(result) <= 255
        result[0] = len(result)
        return result
    def write(start, size, records):
        data = bytearray(size)
        pos = 0
        for record in records:
            if len(record) > block - pos % block:
                pos = (pos // block + 1) * block
            assert pos + len(record) <= size
            data[pos:pos + len(record)] = record
            pos += len(record)
        image[start:start + size] = data
    root = number(primary + 158) * block
    size = number(primary + 166)
    records = entries(root, size)
    if kind in ("producer-xa", "producer-skip"):
        # Include the physical RE tree: CL targets need the same prefix rules
        # as ordinary directories even though RE entries are hidden in VFS.
        pending = [(root, size)]
        seen = set()
        skip = 14 if kind == "producer-xa" else 7
        while pending:
            start, length = pending.pop()
            if start in seen:
                continue
            seen.add(start)
            output = []
            for record in entries(start, length):
                name = bytes(record[33:33 + record[32]])
                if record[25] & 2 and name not in (b"\0", b"\1"):
                    pending.append((struct.unpack_from("<I", record, 2)[0] * block,
                                    struct.unpack_from("<I", record, 10)[0]))
                data = bytearray(record[start_su(record):])
                root_dot = start == root and name == b"\0"
                if root_dot:
                    assert data[:2] == b"SP"
                    data[6] = skip
                if root_dot and kind == "producer-skip":
                    # Root SP establishes skip; a later lookup skips that SP.
                    output.append(record[:start_su(record)] + data)
                else:
                    output.append(replace_su(record, data, bytes(skip)))
            write(start, length, output)
    else:
        target_name = b"LINK.;1" if kind.startswith("producer-sl") else b"ALPHA.BIN;1"
        index = next(i for i, r in enumerate(records) if bytes(r[33:33 + r[32]]) == target_name)
        record = records[index]
        replaced = b"SL" if kind.startswith("producer-sl") else b"TF" if kind.startswith("producer-tf") else b"NM"
        data = b"".join(tag for tag in tags(record) if tag[:2] != replaced)
        def nm(text, flags):
            return b"NM" + bytes((5 + len(text), 1, flags)) + text
        if kind == "producer-nm-parts":
            data += nm(b"split-", 1) + nm(b"name.bin", 0)
            new_name = "split-name.bin"
        elif kind == "producer-nm-nul":
            data += nm(b"early\0ignored", 1) + nm(b"-tail.bin", 0)
            new_name = "early-tail.bin"
        elif kind == "producer-nm-overflow":
            new_name = "prefix-" + "n" * 220
            data += nm(new_name.encode(), 1) + nm(b"x" * 40, 0)
        elif kind.startswith("producer-sl"):
            def sl(parts, flags):
                content = b"".join(bytes((flag, len(value))) + value for flag, value in parts)
                return b"SL" + bytes((5 + len(content), 1, flags)) + content
            if kind == "producer-sl-parts":
                data += sl([(1, b"joined")], 1)
                data += sl([(0, b"-leaf"), (2, b""), (4, b""), (0, b"target")], 0)
                target = b"joined-leaf/./../target"
            else:
                data += sl([(8, b""), (0, b"absolute"), (2, b""), (4, b""), (0, b"target")], 0)
                target = b"/absolute/./../target"
            files["link"] = target
        elif kind.startswith("producer-tf"):
            if kind == "producer-tf-malformed-fraction":
                data += b"TF\x16\x01\x82" + b"20410102030405\xff\xff\0"
            else:
                dates = b"".join(bytes((year, 1, 2, 3, 4, 5, 204)) for year in (100, 101, 102, 103))
                if kind == "producer-tf-truncated":
                    dates = dates[:17]
                data += b"TF" + bytes((5 + len(dates), 1, 15)) + dates
        else:
            raise ValueError(kind)
        records[index] = replace_su(record, data)
        write(root, size, records)
        if kind.startswith("producer-nm"):
            files[new_name] = files.pop("alpha.bin")
    both(image, primary + 80, len(image) // block)
    return bytes(image), files
