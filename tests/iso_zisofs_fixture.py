"""Native mkzftree ISO and focused zisofs pointer/codec variants."""
from pathlib import Path
import random
import struct
import zlib


def volume(kind="zisofs"):
    image = bytearray(zlib.decompress((Path(__file__).parent / "fixtures/iso/zisofs.iso.zlib").read_bytes()))
    files = {"alpha.bin": bytes(range(256)) * 513,
             "noise.bin": random.Random(37).randbytes(150321),
             "mixed.bin": bytes(32768) + (bytes(range(251)) * 131)[:32768] + bytes(32768) + b"tail" * 883,
             "nested/данные 空.txt": b"zisofs unicode payload\n" * 2049,
             "empty": b""}
    def get32(pos):
        return struct.unpack_from("<I", image, pos)[0]
    def both32(buffer, pos, value):
        struct.pack_into("<I", buffer, pos, value)
        struct.pack_into(">I", buffer, pos + 4, value)
    def entries(start, size):
        pos = start
        while pos < start + size:
            length = image[pos]
            if not length:
                pos = (pos // 2048 + 1) * 2048
                continue
            yield pos, bytearray(image[pos:pos + length])
            pos += length
    def susp(record):
        pos = (33 + record[32] + 1) & ~1
        while pos + 4 <= len(record) and record[pos + 2] >= 4:
            yield pos, bytes(record[pos:pos + 2])
            pos += record[pos + 2]
    primary = 16 * 2048
    root = get32(primary + 158) * 2048
    size = get32(primary + 166)
    records = list(entries(root, size))
    named = {bytes(record[33:33 + record[32]]): (pos, record) for pos, record in records}
    alpha_pos, alpha_record = named[b"ALPHA.BIN;1"]
    alpha_zf = next(pos for pos, tag in susp(alpha_record) if tag == b"ZF")
    alpha_start = struct.unpack_from("<I", alpha_record, 2)[0] * 2048
    alpha_size = struct.unpack_from("<I", alpha_record, 10)[0]
    original = bytes(image[alpha_start:alpha_start + alpha_size])
    def append(data):
        start = len(image) // 2048
        image.extend(data)
        image.extend(bytes((-len(image)) % 2048))
        both32(image, primary + 80, len(image) // 2048)
        return start
    def root_records(updated):
        result = bytearray()
        for record in updated:
            if len(record) > 2048 - len(result) % 2048:
                result += bytes((-len(result)) % 2048)
            result += record
        assert len(result) <= size
        image[root:root + size] = result + bytes(size - len(result))
    custom = {"zisofs-shift0": 0, "zisofs-pointer-boundary": 8, "zisofs-expanding": 8, "zisofs-small-sparse": 10,
              "zisofs-shift17": 17, "zisofs-padding": 15, "zisofs-crc": 15,
              "zisofs-short-zlib": 15, "zisofs-short-output": 15, "zisofs-long-output": 15,
              "zisofs-header0": 15}
    if kind in custom:
        shift = custom[kind]
        data = files["alpha.bin"]
        if kind == "zisofs-pointer-boundary":
            data = b"table crossing\0" * 9000
        if kind == "zisofs-shift0":
            data = b"small range fixture"
        if kind == "zisofs-small-sparse":
            data = bytes(1024) + data[1024:]
        files["alpha.bin"] = data
        header = bytearray(original[:16]) if kind != "zisofs-header0" else bytearray()
        header_words = len(header) // 4
        if header:
            struct.pack_into("<I", header, 8, len(data))
            header[13] = shift
        blocks = []
        for offset in range(0, len(data), 1 << shift):
            chunk = data[offset:offset + (1 << shift)]
            if offset == 0 and kind == "zisofs-short-output":
                chunk = chunk[:-1]
            if offset == 0 and kind == "zisofs-long-output":
                chunk += b"X"
            packed = zlib.compress(chunk)
            if kind == "zisofs-padding":
                packed += bytes(3)
            if kind == "zisofs-small-sparse" and not any(chunk):
                packed = b""
            if offset == 0 and kind == "zisofs-crc":
                packed = packed[:-1] + bytes((packed[-1] ^ 1,))
            if offset == 0 and kind == "zisofs-short-zlib":
                packed = packed[:-4]
            blocks.append(packed)
        position = len(header) + (len(blocks) + 1) * 4
        pointers = []
        for block in blocks:
            pointers.append(position)
            position += len(block)
        pointers.append(position)
        packed = header + b"".join(struct.pack("<I", value) for value in pointers) + b"".join(blocks)
        both32(image, alpha_pos + 2, append(packed))
        both32(image, alpha_pos + 10, len(packed))
        image[alpha_pos + alpha_zf + 6] = header_words
        image[alpha_pos + alpha_zf + 7] = shift
        both32(image, alpha_pos + alpha_zf + 8, len(data))
    elif kind == "zisofs-embedded-header":
        image[alpha_start:alpha_start + 16] = b"ignored by Linux"
    elif kind == "zisofs-empty":
        both32(image, alpha_pos + alpha_zf + 8, 0)
        both32(image, alpha_pos + 10, 0)
        files["alpha.bin"] = b""
    elif kind == "zisofs-sparse-marker":
        struct.pack_into("<II", image, alpha_start + 16, 0xffffffff, 0xffffffff)
        both32(image, alpha_pos + alpha_zf + 8, 32768)
        files["alpha.bin"] = bytes(32768)
    elif kind == "zisofs-pointer-order":
        struct.pack_into("<I", image, alpha_start + 20, 0)
    elif kind == "zisofs-pointer-outside":
        struct.pack_into("<I", image, alpha_start + 20, 0xffffffff)
    elif kind == "zisofs-pointer-header":
        struct.pack_into("<II", image, alpha_start + 16, 0, 16)
    elif kind == "zisofs-short-table":
        both32(image, alpha_pos + 10, 16)
    elif kind == "zisofs-multi":
        pos, record = named[b"NOISE.BIN;1"]
        start = struct.unpack_from("<I", record, 2)[0] * 2048
        length = struct.unpack_from("<I", record, 10)[0]
        first, second = bytearray(record), bytearray(record)
        first[25] |= 128
        both32(first, 10, 2048)
        both32(second, 2, append(image[start + 2048:start + length]))
        both32(second, 10, length - 2048)
        updated = []
        for offset, entry in records:
            updated += [first, second] if offset == pos else [entry]
        root_records(updated)
    elif kind == "zisofs-block512":
        # Three root records fit without padding; this also mounts on Linux6.8,
        # whose directory-padding behavior predates the pinned kernel changes.
        selected = [records[0][1], records[1][1], alpha_record]
        for record in selected:
            extent = struct.unpack_from("<I", record, 2)[0]
            both32(record, 2, extent * 4)
            if record[25] & 2:
                both32(record, 10, 512)
            for offset, tag in susp(record):
                if tag == b"CE":
                    both32(record, offset + 4, struct.unpack_from("<I", record, offset + 4)[0] * 4)
        root_records(selected)
        for location, order in ((140, "<"), (148, ">")):
            table = struct.unpack_from(order + "I", image, primary + location)[0]
            struct.pack_into(order + "I", image, primary + location, table * 4)
            pos = table * 2048
            end = pos + get32(primary + 132)
            while pos < end:
                struct.pack_into(order + "I", image, pos + 2, struct.unpack_from(order + "I", image, pos + 2)[0] * 4)
                pos += 8 + image[pos] + image[pos] % 2
        struct.pack_into("<H", image, primary + 128, 512)
        struct.pack_into(">H", image, primary + 130, 512)
        both32(image, primary + 80, len(image) // 512)
        both32(image, primary + 158, root // 512)
        both32(image, primary + 166, 512)
        files = {"alpha.bin": files["alpha.bin"]}
    elif kind != "zisofs":
        raise ValueError(kind)
    return bytes(image), files
