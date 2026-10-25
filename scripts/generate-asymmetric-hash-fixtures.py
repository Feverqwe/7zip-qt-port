#!/usr/bin/env python3
"""Generate synthetic certificate-hash answers via Linux AF_ALG, then a C fixture.

Run --native on Linux and save its JSON output. Pass that file to --native-input
to emit the fixture; hashlib independently checks algorithms it provides.
No keyring or system configuration is changed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import platform
import socket

LENGTHS = (0, 1, 55, 56, 63, 64, 65, 111, 112, 127, 128, 129,
           135, 136, 137, 143, 144, 145, 4097)
ALGORITHMS = ("sha1", "sha224", "sha256", "sha384", "sha512", "sha3-224",
              "sha3-256", "sha3-384", "sha3-512", "sm3", "streebog256", "streebog512")


def payload(size):
    return bytes((i * 37 + 11) & 255 for i in range(size))


def native():
    vectors = []
    for algorithm in ALGORITHMS:
        for size in LENGTHS:
            with socket.socket(socket.AF_ALG, socket.SOCK_SEQPACKET, 0) as parent:
                parent.bind(("hash", algorithm))
                child, _ = parent.accept()
                with child:
                    child.sendall(payload(size))
                    digest = child.recv(64).hex()
            vectors.append({"algorithm": algorithm, "size": size, "hex": digest})
    return {"oracle": "Linux AF_ALG", "kernel": platform.release(), "vectors": vectors}


def shake_vectors():
    lines = ["/* SPDX-License-Identifier: CC0-1.0 */",
             "/* Python hashlib SHAKE answers; input byte i is (i * 37 + 11) modulo 256. */"]
    for bits in (128, 256):
        for size in (0, 257):
            digest = getattr(hashlib, "shake_" + str(bits))(payload(size)).hexdigest(400)
            lines.append("{%d, %d," % (bits, size))
            lines.extend(' "' + digest[i:i + 80] + '"' for i in range(0, len(digest), 80))
            lines.append("},")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--native", action="store_true")
    mode.add_argument("--native-input", type=Path)
    mode.add_argument("--shake", action="store_true", help="emit SHAKE continuation answers via hashlib")
    args = parser.parse_args()
    if args.shake:
        print(shake_vectors())
        return
    if args.native:
        print(json.dumps(native(), indent=2))
        return
    data = json.loads(args.native_input.read_text())
    expected = [(algorithm, size) for algorithm in ALGORITHMS for size in LENGTHS]
    if [(item["algorithm"], item["size"]) for item in data["vectors"]] != expected:
        raise ValueError("Incomplete native hash answers")
    lines = ["/* SPDX-License-Identifier: CC0-1.0 */",
             f"/* Linux AF_ALG {data['kernel']}; hashlib cross-check where available.",
             " * Input byte i is (i * 37 + 11) modulo 256. */"]
    for item in data["vectors"]:
        name = item["algorithm"].replace("-", "_")
        if name in hashlib.algorithms_available:
            actual = hashlib.new(name, payload(item["size"])).hexdigest()
            if actual != item["hex"]:
                raise ValueError(f"Native/hashlib mismatch: {item['algorithm']} {item['size']}")
        lines.append('{"%s", %d, "%s"},' % (item["algorithm"], item["size"], item["hex"]))
    print("\n".join(lines))


if __name__ == "__main__":
    main()
