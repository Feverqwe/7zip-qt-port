#!/usr/bin/env python3
"""Generate RSA verification fixtures from published Linux test keys.

Python integer arithmetic signs independently constructed EMSA-PKCS1-v1_5
encodings. OpenSSL verifies every positive case before the header is written.
Private inputs are public test vectors; only public keys/signatures are emitted.
"""
import argparse
import ast
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess
import tempfile

PREFIXES = {
    "sha1": "3021300906052b0e03021a05000414",
    "sha224": "302d300d06096086480165030402040500041c",
    "sha256": "3031300d060960864801650304020105000420",
    "sha384": "3041300d060960864801650304020205000430",
    "sha512": "3051300d060960864801650304020305000440",
    "sha3-256": "3031300d060960864801650304020805000420",
    "sha3-384": "3041300d060960864801650304020905000430",
    "sha3-512": "3051300d060960864801650304020a05000440",
}


def der(tag, payload):
    size = len(payload)
    length = bytes((size,)) if size < 128 else size.to_bytes((size.bit_length() + 7) // 8, "big")
    if size >= 128:
        length = bytes((128 + len(length),)) + length
    return bytes((tag,)) + length + payload


def integer(value):
    data = value.to_bytes((value.bit_length() + 7) // 8, "big") or b"\0"
    return der(2, (b"\0" if data[0] & 128 else b"") + data)


def integers(data):
    def read(offset):
        tag, size = data[offset:offset + 2]
        offset += 2
        if size & 128:
            count = size & 127
            size = int.from_bytes(data[offset:offset + count], "big")
            offset += count
        return tag, data[offset:offset + size], offset + size
    tag, contents, end = read(0)
    assert tag == 48 and end == len(data)
    data = contents
    result, offset = [], 0
    while offset < len(data):
        tag, value, offset = read(offset)
        assert tag == 2
        result.append(int.from_bytes(value, "big"))
    return result


def field(entry, name):
    match = re.search(r'\.' + name + r'\s*=\s*((?:"(?:\\.|[^"\\])*"\s*)+)', entry)
    return b"".join(ast.literal_eval("b" + item) for item in
                    re.findall(r'"(?:\\.|[^"\\])*"', match[1])) if match else None


def entries(source, name):
    section = source.split(name + "[] = {", 1)[1].split("\n};", 1)[0]
    section = re.sub(r"/\*.*?\*/", "", section, flags=re.S)
    return re.split(r"\}\s*,\s*\{", section)


def emit_bytes(value):
    return "\n".join('"' + "".join(f"\\x{byte:02x}" for byte in value[pos:pos + 24]) + '"'
                     for pos in range(0, len(value), 24))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--linux", type=Path, required=True)
    parser.add_argument("--openssl", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    raw = (args.linux / "crypto/testmgr.h").read_bytes()
    baseline = json.loads((root / "docs/ext-linux-baseline.json").read_text())
    if hashlib.sha256(raw).hexdigest() != baseline["files"]["crypto/testmgr.h"]:
        raise ValueError("Linux fixture source differs from the pinned reference")
    source, keys = raw.decode(), {}
    for entry in entries(source, "rsa_tv_template"):
        values = integers(field(entry, "key"))
        if len(values) == 9:
            _, n, e, d, p, q, *_ = values
            keys[n.bit_length()] = (n, e, d, p, q)
    # Recombine published primes to cover the other Linux RSA modulus sizes.
    for bits, left, right in [(1536, 1024, 2048), (3072, 2048, 4096)]:
        for p in keys[left][3:5]:
            for q in keys[right][3:5]:
                phi, n = (p - 1) * (q - 1), p * q
                if n.bit_length() == bits and math.gcd(65537, phi) == 1:
                    keys[bits] = (n, 65537, pow(65537, -1, phi), p, q)
        if bits not in keys:
            raise ValueError(f"Published primes cannot produce a {bits}-bit key")
    assert sorted(keys) == [512, 1024, 1536, 2048, 3072, 4096]
    # Prove the independent encoding/math reproduces the published CAVS answer.
    cavs = entries(source, "pkcs1_rsa_tv_template")[0]
    _, n, e, d, *_ = integers(field(cavs, "key"))
    digest = field(cavs, "m")
    prefix = bytes.fromhex(PREFIXES["sha256"])
    encoded = b"\0\1" + b"\xff" * (256 - len(prefix) - len(digest) - 3) + b"\0" + prefix + digest
    assert pow(int.from_bytes(encoded, "big"), d, n).to_bytes(256, "big") == field(cavs, "c")

    public = {bits: der(48, integer(key[0]) + integer(key[1])) for bits, key in keys.items()}
    cases = []
    with tempfile.TemporaryDirectory(prefix="rsa-fixtures-") as temporary:
        temp = Path(temporary)
        for bits, key in sorted(keys.items()):
            n, e, d, *_ = key
            width = bits // 8
            algorithms = PREFIXES if bits == 2048 else {"sha256": PREFIXES["sha256"]}
            for algorithm, prefix_hex in algorithms.items():
                digest = hashlib.new(algorithm.replace("-", "_"), b"synthetic filesystem signature").digest()
                prefix = bytes.fromhex(prefix_hex)
                encoded = b"\0\1" + b"\xff" * (width - len(prefix) - len(digest) - 3) + b"\0" + prefix + digest
                signature = pow(int.from_bytes(encoded, "big"), d, n).to_bytes(width, "big")
                spki = der(48, bytes.fromhex("300d06092a864886f70d0101010500") + der(3, b"\0" + public[bits]))
                (temp / "key.der").write_bytes(spki)
                (temp / "signature.bin").write_bytes(signature)
                (temp / "digest.bin").write_bytes(digest)
                subprocess.run([args.openssl, "pkeyutl", "-verify", "-pubin", "-keyform", "DER",
                                "-inkey", str(temp / "key.der"), "-sigfile", str(temp / "signature.bin"),
                                "-in", str(temp / "digest.bin"), "-pkeyopt", "digest:" + algorithm],
                               check=True, capture_output=True)
                cases.append((f"{bits}-{algorithm}", bits, algorithm, digest, signature, "RSA_VALID"))
                if bits == 2048 and algorithm == "sha256":
                    for label, offset, value, expected in [("leading-byte", 0, 1, "RSA_INVALID"),
                            ("block-type", 1, 2, "RSA_MALFORMED"), ("short-padding", 2, 0, "RSA_MALFORMED"),
                            ("algorithm-prefix", width - len(digest) - 1, 1, "RSA_MALFORMED")]:
                        changed = bytearray(encoded)
                        changed[offset] = value
                        bad = pow(int.from_bytes(changed, "big"), d, n).to_bytes(width, "big")
                        cases.append((label, bits, algorithm, digest, bad, expected))
                    changed = bytes((digest[0] ^ 1,)) + digest[1:]
                    cases.append(("foreign-digest", bits, algorithm, changed, signature, "RSA_REJECTED"))
                    for value in [0, 1, n - 1, n]:
                        cases.append(("signature-range", bits, algorithm, digest,
                                      value.to_bytes(width, "big"), "RSA_INVALID"))
    notice = source[:source.index("#ifndef _CRYPTO_TESTMGR_H")]
    lines = [notice, "/* Generated published-key fixtures; see scripts/generate-rsa-fixtures.py. */"]
    for bits, value in sorted(public.items()):
        lines.append(f"static const u8 rsa_key_{bits}[] =\n{emit_bytes(value)};")
    lines.append("static const struct rsa_vector rsa_vectors[] = {")
    for name, bits, algorithm, digest, signature, expected in cases:
        lines.append('{"%s", "%s", rsa_key_%d, sizeof(rsa_key_%d) - 1, %d, %d, %s,\n%s,\n%s},' %
                     (name, algorithm, bits, bits, len(signature), len(digest), expected,
                      emit_bytes(digest), emit_bytes(signature)))
    lines.append("};")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines) + "\n")
    print(f"Generated {len(cases)} RSA answers; all positive cases verified independently by OpenSSL")


if __name__ == "__main__":
    main()
