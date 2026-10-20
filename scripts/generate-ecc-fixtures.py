#!/usr/bin/env python3
"""Extract published Linux ECDSA/ECRDSA known answers at the audited pin.

ECDSA cases are also emitted in P1363 encoding to exercise both read wrappers.
No cryptographic implementation or private key is part of this generator.
"""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import re


def string_field(body, name):
    value = re.search(r"\." + name + r"\s*=\s*(.*?),", body, re.S).group(1)
    value = re.sub(r"/\*.*?\*/", "", value, flags=re.S)
    return b"".join(ast.literal_eval("b" + s) for s in re.findall(r'"[^"]*"', value))


def p1363(signature, size):
    # The input is a published DER test vector. Accept only its definite-length
    # SEQUENCE and two positive INTEGERs; this isn't a production BER parser.
    assert signature[0] == 0x30
    offset = 3 if signature[1] == 0x81 else 2
    coordinates = []
    for _ in range(2):
        assert signature[offset] == 2
        length = signature[offset + 1]
        offset += 2
        coordinates.append(int.from_bytes(signature[offset:offset + length], "big").to_bytes(size, "big"))
        offset += length
    assert offset == len(signature)
    return b"".join(coordinates)


def literal(data):
    return "\n".join('\t"' + "".join(f"\\x{v:02x}" for v in data[i:i+16]) + '"'
                     for i in range(0, len(data), 16)) if data else '""'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--linux", type=Path, default=Path("build/review/filesystems-linux/linux"))
    parser.add_argument("--output", type=Path, default=Path("tests/fixtures/asn1/ecc-vectors.h"))
    args = parser.parse_args()
    source = args.linux / "crypto/testmgr.h"
    manifest = json.loads(Path("docs/ext-linux-baseline.json").read_text())
    assert hashlib.sha256(source.read_bytes()).hexdigest() == manifest["files"]["crypto/testmgr.h"]
    text = source.read_text()
    cases = []
    for bits in (192, 256, 384, 521):
        name = f"x962_ecdsa_nist_p{bits}_tv_template"
        array = re.search(r"\b" + name + r"\[\] = \{(.*?)\n\};", text, re.S).group(1)
        for body in re.findall(r"\{(.*?)\}", array, re.S):
            key, digest, signature = (string_field(body, f) for f in ("key", "m", "c"))
            assert len(key) == 1 + 2 * ((bits + 7) // 8)
            cases.append((f"x962(ecdsa-nist-p{bits})", key, digest, signature, b"", "0"))
            cases.append((f"p1363(ecdsa-nist-p{bits})", key, digest,
                          p1363(signature, (bits + 7) // 8), b"", "0"))
    array = re.search(r"\becrdsa_tv_template\[\] = \{(.*?)\n\};", text, re.S).group(1)
    for body in re.findall(r"\{(.*?)\}", array, re.S):
        key, digest, signature, params = (string_field(body, f) for f in ("key", "m", "c", "params"))
        algo = re.search(r"\.algo\s*=\s*(\w+)", body).group(1)
        cases.append(("ecrdsa", key, digest, signature, params, algo))
    out = [text[:text.index("#ifndef _CRYPTO_TESTMGR_H")].rstrip(),
           "/* Generated from Linux crypto/testmgr.h, revision " + manifest["revision"] + ". */",
           "/* Published ECDSA answers and gost-engine ECRDSA answers; Linux testmgr notices apply. */",
           "static const struct ecc_vector ecc_vectors[] = {"]
    for name, key, digest, sig, params, algo in cases:
        out.append('{ "' + name + '",\n' + ",\n".join(literal(v) for v in (key, digest, sig, params)) +
                   f",\n{len(key)}, {len(digest)}, {len(sig)}, {len(params)}, {algo} }},")
    out.append("};\n")
    args.output.write_text("\n".join(out))
    print(f"Wrote {len(cases)} Linux ECC known answers to {args.output}")


if __name__ == "__main__":
    main()
