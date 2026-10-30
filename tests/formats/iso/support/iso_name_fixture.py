"""Focused ISO/RR/Joliet name mutations over generated native payloads."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import struct


def volume(kind):
    from tests.formats.iso.support.iso_fixture import volume as iso_volume
    joliet = kind.startswith("names-joliet")
    base = "joliet" if joliet else "rock" if kind == "names-rr" else "plain"
    original, files = iso_volume(base)
    image = bytearray(original)
    primary = 16 * 2048
    descriptor = primary
    if joliet:
        while image[descriptor] != 2:
            descriptor += 2048
    root = struct.unpack_from("<I", image, descriptor + 158)[0] * 2048
    size = struct.unpack_from("<I", image, descriptor + 166)[0]

    def records(start, length):
        result = []
        pos = start
        while pos < start + length:
            record_size = image[pos]
            if not record_size:
                pos = (pos // 2048 + 1) * 2048
                continue
            result.append(bytearray(image[pos:pos + record_size]))
            pos += record_size
        return result

    def name(record):
        raw = record[33:33 + record[32]]
        return raw.decode("utf-16-be") if joliet and len(raw) > 1 else bytes(raw).decode("ascii")

    def replace(record, new_name, system=None):
        old_start = (33 + record[32] + 1) & ~1
        suffix = record[old_start:] if system is None else system
        result = record[:33]
        result[32] = len(new_name)
        result += new_name
        result += bytes(len(result) % 2)
        result += suffix
        assert len(result) <= 255
        result[0] = len(result)
        return result

    root_records = records(root, size)
    alpha_name = "alpha.bin" if joliet else "ALPHA.BIN;1"
    alpha_index = next(i for i, record in enumerate(root_records) if name(record) == alpha_name)
    alpha = root_records[alpha_index]
    nested_name = "nested" if joliet else "NESTED"
    nested_index = next(i for i, record in enumerate(root_records) if name(record) == nested_name)
    expected_alpha = "alpha.bin" if base == "rock" or joliet else "ALPHA.BIN"
    replacement = None
    if kind == "names-joliet-components":
        replacement = "alpha....;1".encode("utf-16-be")
        new_path = "alpha"
        root_records[nested_index] = replace(root_records[nested_index], "Nested...;1".encode("utf-16-be"))
        files = {key.replace("nested/", "Nested/"): value for key, value in files.items()}
    elif kind == "names-joliet-surrogates":
        replacement = "face-😀.bin;1".encode("utf-16-be")
        new_path = "face-😀.bin"
    elif kind == "names-joliet-traversal":
        replacement = "../escape.bin;1".encode("utf-16-be")
        new_path = "../escape.bin"
    elif kind == "names-joliet-malformed":
        replacement = "a\ud800b\udc00c.bin;1".encode("utf-16-be", errors="surrogatepass")
        new_path = "abc.bin"
    elif kind == "names-joliet-nul":
        replacement = "early\0ignored;1".encode("utf-16-be")
        new_path = "early"
    elif kind == "names-joliet-empty":
        replacement = b""
        new_path = None
    elif kind == "names-joliet-long":
        new_path = "界" * 90 + ".bin"
        replacement = new_path.encode("utf-16-be")
    elif kind in ("names-joliet-cp437", "names-joliet-utf8"):
        replacement = "café-空.bin;1".encode("utf-16-be")
        new_path = "café-?.bin" if kind == "names-joliet-cp437" else "café-空.bin"
    elif kind in ("names-map-normal", "names-map-off"):
        replacement = b"VERS;2"
        new_path = "vers.2" if kind == "names-map-normal" else "VERS;2"
        files = {key.lower() if not key.startswith("[BOOT]") else key: value
                 for key, value in files.items()} if kind == "names-map-normal" else files
        expected_alpha = "alpha.bin" if kind == "names-map-normal" else "ALPHA.BIN"
        if kind == "names-map-off":
            files = {(key if key.startswith("[BOOT]") or key == "ALPHA.BIN" else
                      "EMPTY.;1" if key == "EMPTY" else key + ";1"): value
                     for key, value in files.items()}
    elif kind.startswith("names-acorn"):
        replacement = b"_APP;1"
        acorn = bytearray(32)
        acorn[:10] = b"ARCHIMEDES"
        acorn[11:14] = bytes((0xbc, 0xfa, 0xff))
        acorn[19] = 1
        if kind == "names-acorn-short":
            acorn = acorn[:-1]
        elif kind == "names-acorn-marker":
            acorn[0] = 0
        elif kind != "names-acorn":
            raise ValueError(kind)
        root_records[alpha_index] = replace(alpha, replacement, acorn)
        files = {key.lower() if not key.startswith("[BOOT]") else key: value for key, value in files.items()}
        expected_alpha = "alpha.bin"
        new_path = "!app,abc" if kind == "names-acorn" else "_app"
    elif kind == "names-rr":
        replacement = b"ALPHA;2.BIN;1"
        system_start = (33 + alpha[32] + 1) & ~1
        pos = system_start
        system = bytearray()
        while pos + 4 <= len(alpha) and alpha[pos + 2] >= 4:
            length = alpha[pos + 2]
            if alpha[pos:pos + 2] != b"NM":
                system += alpha[pos:pos + length]
            pos += length
        root_records[alpha_index] = replace(alpha, replacement, system)
        new_path = "alpha.2.bin"
    else:
        raise ValueError(kind)
    if not kind.startswith("names-acorn") and kind != "names-rr":
        root_records[alpha_index] = replace(alpha, replacement)
    if kind == "names-joliet-cp437":
        files = {key.encode("cp437", errors="replace").decode("cp437"): value
                 for key, value in files.items()}
    data = files.pop(expected_alpha)
    if new_path is not None:
        files[new_path] = data
    output = bytearray()
    for record in root_records:
        if len(record) > 2048 - len(output) % 2048:
            output += bytes((-len(output)) % 2048)
        output += record
    assert len(output) <= size
    image[root:root + size] = output + bytes(size - len(output))
    return bytes(image), files
