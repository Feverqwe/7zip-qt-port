#!/usr/bin/env python3
"""Retain Linux public certificate fixtures and embed their signer certificates.

Only outer DER container lengths change. Original signed bytes, public keys and
signatures stay intact. These files contain no private key material.
"""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import re


def element(data, offset=0):
    start = offset
    tag, size = data[offset:offset + 2]
    offset += 2
    if size & 128:
        count = size & 127
        size = int.from_bytes(data[offset:offset + count], "big")
        offset += count
    end = offset + size
    assert end <= len(data)
    return tag, data[offset:end], data[start:end], end


def encode(tag, data):
    size = len(data)
    length = bytes([size]) if size < 128 else size.to_bytes((size.bit_length() + 7) // 8, "big")
    if size >= 128:
        length = bytes([128 + len(length)]) + length
    return bytes([tag]) + length + data


def embed(signature, certificate):
    tag, content, _, end = element(signature)
    assert tag == 48 and end == len(signature)
    _, _, oid, offset = element(content)
    tag, context, _, end = element(content, offset)
    assert tag == 160 and end == len(content)
    tag, signed, _, end = element(context)
    assert tag == 48 and end == len(context)
    offset = 0
    for _ in range(3):
        _, _, _, offset = element(signed, offset)
    assert signed[offset] == 49  # signerInfos, no existing certificates
    return encode(48, oid + encode(160, encode(48, signed[:offset] + encode(160, certificate) + signed[offset:])))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--linux", type=Path, default=Path("build/review/filesystems-linux/linux"))
    args = parser.parse_args()
    manifest = json.loads(Path("docs/ext-linux-baseline.json").read_text())
    for algorithm in ("rsa", "ecdsa"):
        rel = f"crypto/asymmetric_keys/selftest_{algorithm}.c"
        data = (args.linux / rel).read_bytes()
        assert hashlib.sha256(data).hexdigest() == manifest["files"][rel]
        text = data.decode()
        start = text.index("/*\n * Set of X.509")
        end = text.index("void __init fips_signature_selftest_", start)
        output = text[:text.index("#include")] + text[start:end].replace(" __initconst", "")
        values = {}
        for name, body in re.findall(r"static const u8 (\w+)\[\] __initconst = \{(.*?)\n\};", text, re.S):
            values[name] = b"".join(ast.literal_eval("b" + token) for token in re.findall(r'"[^"]*"', body))
        prefix = f"certs_selftest_{algorithm}"
        embedded = embed(values[prefix + "_sig"], values[prefix + "_keys"])
        output += "/* Signer embedded for internal-chain verification without external trust. */\n"
        output += f"static const u8 {prefix}_embedded[] = {{\n"
        for i in range(0, len(embedded), 16):
            output += '\t"' + "".join(f"\\x{v:02x}" for v in embedded[i:i+16]) + '"\n'
        output += "};\n"
        Path(f"tests/fixtures/asn1/selftest_{algorithm}.h").write_text(output)


if __name__ == "__main__":
    main()
