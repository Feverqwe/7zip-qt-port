"""Native Rock Ridge names, relocation and focused continuation mutations."""
from pathlib import Path
import zlib
import struct


def volume(kind="rock"):
    fixture = "rock-joliet" if kind in ("rock-joliet", "rock-empty-primary", "rock-bad-primary", "rock-bad-primary-name", "rock-no-er-joliet") else "rock"
    image = bytearray(zlib.decompress(
        (Path(__file__).parent / "fixtures/iso" / (fixture + ".iso.zlib")).read_bytes()))
    deep = "/".join("level" + str(i) for i in range(11)) + "/end.bin"
    files = {"alpha.bin": bytes(range(256)) * 513,
             "nested/данные 空.txt": b"RR unicode data\n" * 193,
             "long-" + "n" * 230 + ".bin": b"RR continuation name\n" * 131,
             "Case.Name.;1": b"Preserve RR punctuation\n",
             deep: b"RR relocated directory\n" * 127}
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
    root = get32(16 * 2048 + 158) * 2048
    size = get32(16 * 2048 + 166)
    records = list(entries(root, size))
    alpha = next(record for _, record in records if record[33:33 + record[32]] == b"ALPHA.BIN;1")
    if kind == "rock-root-mode":
        px = next(offset for offset, tag in susp(records[0][1]) if tag == b"PX") + root
        both32(image, px + 4, 0o100640)
    elif kind in ("rock-no-er", "rock-no-er-joliet"):
        ce = next(offset for offset, tag in susp(records[0][1]) if tag == b"CE") + root
        target = get32(ce + 4) * 2048 + get32(ce + 12)
        assert image[target:target + 2] == b"ER"
        image[target:target + 2] = b"ES"
        if kind == "rock-no-er-joliet":
            files = {name: data for name, data in files.items() if name in ("alpha.bin", "nested/данные 空.txt")}
    elif kind.startswith("rock-ce-"):
        ce = next(offset for offset, tag in susp(records[0][1]) if tag == b"CE") + root
        if kind == "rock-ce-outside":
            both32(image, ce + 4, len(image) // 2048)
        elif kind == "rock-ce-crossblock":
            both32(image, ce + 12, 2047)
        elif kind == "rock-ce-cycle":
            target = get32(ce + 4) * 2048
            both32(image, ce + 12, 0)
            both32(image, ce + 20, 28)
            image[target:target + 28] = image[ce:ce + 28]
    elif kind == "rock-sl-overrun":
        pos, record = next((p, r) for p, r in records if r[33:33 + r[32]] == b"LINK.;1")
        sl = next(offset for offset, tag in susp(record) if tag == b"SL")
        image[pos + sl + 6] = 255
    elif kind.startswith("rock-cl-"):
        pending = [(root, size)]
        seen = set()
        found = False
        while pending and not found:
            start, length = pending.pop()
            if start in seen:
                continue
            seen.add(start)
            for pos, record in entries(start, length):
                for offset, tag in susp(record):
                    if tag == b"CL":
                        if kind == "rock-cl-same-extent":
                            both32(image, pos + 2, get32(pos + offset + 4))
                        else:
                            both32(image, pos + offset + 4, 0 if kind == "rock-cl-cycle" else len(image) // 2048)
                        found = True
                        break
                if found:
                    break
                if record[25] & 2 and record[33:34] not in (b"\0", b"\1"):
                    pending.append((struct.unpack_from("<I", record, 2)[0] * 2048,
                                    struct.unpack_from("<I", record, 10)[0]))
        assert found
    elif kind == "rock-shared-ce":
        metadata = b"".join(alpha[offset:offset + alpha[offset + 2]]
                            for offset, tag in susp(alpha) if tag in (b"PX", b"TF"))
        extent = len(image) // 2048
        image += metadata + bytes((-len(metadata)) % 2048)
        ce = bytearray(b"CE\x1c\x01" + bytes(24))
        both32(ce, 4, extent)
        both32(ce, 20, len(metadata))
        output_records = [records[0][1], records[1][1], alpha]
        for i in range(5000):
            name = ("ITEM%05d.BIN;1" % i).encode()
            record = bytearray(alpha[:33])
            record[32] = len(name)
            record += name
            record += bytes(len(record) % 2)
            record += ce
            record[0] = len(record)
            both32(record, 10, 0)
            output_records.append(record)
        def pack():
            output = bytearray()
            for record in output_records:
                if len(record) > 2048 - len(output) % 2048:
                    output += bytes((-len(output)) % 2048)
                output += record
            return output + bytes((-len(output)) % 2048)
        length = len(pack())
        new_root = len(image) // 2048
        for record in output_records[:2]:
            both32(record, 2, new_root)
            both32(record, 10, length)
        image += pack()
        both32(image, 16 * 2048 + 158, new_root)
        both32(image, 16 * 2048 + 166, length)
        both32(image, 16 * 2048 + 80, len(image) // 2048)
        for location, order in ((140, "<"), (148, ">")):
            table = get32(16 * 2048 + location) if order == "<" else struct.unpack_from(">I", image, 16 * 2048 + location)[0]
            struct.pack_into(order + "I", image, table * 2048 + 2, new_root)
        files = {"alpha.bin": files["alpha.bin"]}
    elif kind in ("rock-metadata", "rock-pn-legacy", "rock-pn-wide"):
        su = (33 + alpha[32] + 1) & ~1
        data = bytearray(alpha[su:])
        if kind == "rock-metadata":
            dates = [b"2030010203040537\0", b"2041010203040537\0",
                     b"2042010203040537\0", b"2043010203040537\0"]
            data += b"TF" + bytes((5 + 17 * 4, 1, 0x8f)) + b"".join(dates)
        else:
            px = next(offset for offset, tag in susp(alpha) if tag == b"PX") - su
            both32(data, px + 4, 0o20640)
            pn = bytearray(b"PN\x14\x01" + bytes(16))
            both32(pn, 4, 0 if kind == "rock-pn-legacy" else 0xabc)
            both32(pn, 12, 0x1234 if kind == "rock-pn-legacy" else 0x54321)
            data += pn
        # A native record already has a final padding byte. Replace it before
        # adding another SUSP record, rather than manufacture an early terminator.
        if alpha[-1] == 0:
            data = data[:len(alpha) - su - 1] + data[len(alpha) - su:]
        extent = len(image) // 2048
        image += data + bytes((-len(data)) % 2048)
        ce = bytearray(b"CE\x1c\x01" + bytes(24))
        both32(ce, 4, extent)
        both32(ce, 20, len(data))
        alpha[su:] = ce
        alpha[0] = len(alpha)
        output = b"".join(record for _, record in records)
        assert len(output) <= size
        image[root:root + size] = output + bytes(size - len(output))
        both32(image, 16 * 2048 + 80, len(image) // 2048)
    elif kind == "rock-empty-primary":
        offset = records[2][0]
        image[offset:root + size] = bytes(root + size - offset)
        files = {name: data for name, data in files.items() if name in ("alpha.bin", "nested/данные 空.txt")}
    elif kind == "rock-bad-primary":
        both32(image, 16 * 2048 + 158, len(image) // 2048)
        files = {name: data for name, data in files.items() if name in ("alpha.bin", "nested/данные 空.txt")}
    elif kind == "rock-bad-primary-name":
        image[root + 32] = 255
        files = {name: data for name, data in files.items() if name in ("alpha.bin", "nested/данные 空.txt")}
    elif kind not in ("rock", "rock-joliet"):
        raise ValueError(kind)
    return bytes(image), files
