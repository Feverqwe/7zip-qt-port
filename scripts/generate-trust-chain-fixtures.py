#!/usr/bin/env python3
"""Build public trust-chain fixtures from published Linux RSA test keys.

The original Linux certificate supplies validity and signature-algorithm fields.
Independent Python integer signing and OpenSSL CMS verification check the data
before only public certificates/messages are emitted. No runtime tool dependency.
"""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import re
import runpy
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--linux", type=Path, default=Path("build/review/filesystems-linux/linux"))
    parser.add_argument("--openssl", required=True)
    args = parser.parse_args()
    helpers = runpy.run_path(str(Path(__file__).with_name("generate-asymmetric-certificate-fixtures.py")))
    element, der = helpers["element"], helpers["encode"]
    rsa = runpy.run_path(str(Path(__file__).with_name("generate-rsa-fixtures.py")))
    manifest = json.loads(Path("docs/ext-linux-baseline.json").read_text())
    sources = {}
    for name in ("crypto/testmgr.h", "crypto/asymmetric_keys/selftest_rsa.c"):
        data = (args.linux / name).read_bytes()
        assert hashlib.sha256(data).hexdigest() == manifest["files"][name]
        sources[name] = data.decode()
    source = sources["crypto/testmgr.h"]
    keys = {}
    for entry in rsa["entries"](source, "rsa_tv_template"):
        encoded = rsa["field"](entry, "key")
        values = rsa["integers"](encoded)
        if len(values) == 9:
            keys[values[1].bit_length()] = (encoded, values[1], values[2], values[3])
    template = sources["crypto/asymmetric_keys/selftest_rsa.c"]
    body = re.search(r"certs_selftest_rsa_keys\[\].*?= \{(.*?)\n\};", template, re.S).group(1)
    original = b"".join(ast.literal_eval("b" + v) for v in re.findall(r'"[^"]*"', body))

    def fields(data):
        result, offset = [], 0
        while offset < len(data):
            _, _, encoded, offset = element(data, offset)
            result.append(encoded)
        return result

    tbs = fields(element(original)[1])[0]
    original_fields = fields(element(tbs)[1])
    sig_algo, validity = original_fields[2], original_fields[4]
    rsa_algo = bytes.fromhex("300d06092a864886f70d0101010500")

    def name(label):
        return der(48, der(49, der(48, bytes.fromhex("0603550403") + der(12, label.encode()))))

    def sign(data, bits):
        _, n, _, d = keys[bits]
        digest = bytes.fromhex(rsa["PREFIXES"]["sha256"]) + hashlib.sha256(data).digest()
        size = (n.bit_length() + 7) // 8
        encoded = b"\0\1" + b"\xff" * (size - len(digest) - 3) + b"\0" + digest
        return pow(int.from_bytes(encoded, "big"), d, n).to_bytes(size, "big")

    def certificate(label, serial, bits, parent=None, ca=True, identity=None):
        _, n, e, _ = keys[bits]
        public = der(48, rsa["integer"](n) + rsa["integer"](e))
        subject = name(label)
        skid = identity if identity is not None else hashlib.sha1(public).digest()
        parent = parent or {"subject": subject, "issuer": subject,
                            "skid": skid, "serial": serial, "bits": bits}

        def extension(oid, value, critical=False):
            return der(48, der(6, bytes.fromhex(oid)) + (b"\x01\x01\xff" if critical else b"") + der(4, value))

        # Include both AKID components to cover the required dual-ID match.
        auth = der(48, der(128, parent["skid"]) +
                   der(161, der(164, parent["issuer"])) +
                   der(130, element(rsa["integer"](parent["serial"]))[1]))
        extensions = (
            extension("551d0e", der(4, skid)) + extension("551d23", auth) +
            extension("551d13", der(48, b"\x01\x01\xff" if ca else b""), True) +
            extension("551d0f", der(3, b"\x02\x84" if ca else b"\x07\x80")))
        tbs = der(48, original_fields[0] + rsa["integer"](serial) + sig_algo +
                  parent["subject"] + validity + subject +
                  der(48, rsa_algo + der(3, b"\0" + public)) + der(163, der(48, extensions)))
        return {"certificate": der(48, tbs + sig_algo + der(3, b"\0" + sign(tbs, parent["bits"]))),
                "subject": subject, "issuer": parent["subject"], "skid": skid,
                "serial": serial, "bits": bits}

    root = certificate("Linux test root", 101, 2048)
    intermediate = certificate("Linux test intermediate", 102, 1024, root)
    leaf = certificate("Linux test leaf", 103, 512, intermediate, False)
    collision = certificate("Linux test root", 101, 1024, identity=root["skid"])
    wrong_id = certificate("Linux test root", 101, 2048, identity=hashlib.sha1(b"different key identifier").digest())
    data = b"synthetic filesystem trust chain\n"
    with tempfile.TemporaryDirectory(prefix="sevenzip-trust-fixtures-") as temporary:
        temp = Path(temporary)

        def openssl(*command):
            return subprocess.run([args.openssl, *command], check=True, capture_output=True)

        for label, item in (("root", root), ("intermediate", intermediate), ("leaf", leaf), ("collision", collision)):
            (temp / (label + ".der")).write_bytes(item["certificate"])
            openssl("x509", "-inform", "DER", "-in", str(temp / (label + ".der")), "-out", str(temp / (label + ".pem")))
        openssl("verify", "-auth_level", "0", "-check_ss_sig", "-CAfile", str(temp / "collision.pem"), str(temp / "collision.pem"))
        (temp / "key.der").write_bytes(keys[512][0])
        (temp / "data").write_bytes(data)
        openssl("cms", "-sign", "-binary", "-noattr", "-nocerts", "-md", "sha256", "-in", str(temp / "data"),
                "-signer", str(temp / "leaf.pem"), "-inkey", str(temp / "key.der"), "-keyform", "DER",
                "-outform", "DER", "-out", str(temp / "message.der"))
        message = (temp / "message.der").read_bytes()
        (temp / "chain.pem").write_bytes((temp / "leaf.pem").read_bytes() + (temp / "intermediate.pem").read_bytes())
        openssl("cms", "-verify", "-binary", "-inform", "DER", "-in", str(temp / "message.der"),
                "-content", str(temp / "data"), "-CAfile", str(temp / "root.pem"), "-certfile", str(temp / "chain.pem"),
                "-auth_level", "0", "-purpose", "any", "-out", str(temp / "verified"))
        assert (temp / "verified").read_bytes() == data
        verity_signatures = {}
        for algorithm, size in ((1, 32), (2, 64)):
            formatted = b"FSVerity" + algorithm.to_bytes(2, "little") + size.to_bytes(2, "little") + bytes(range(size))
            (temp / "data").write_bytes(formatted)
            openssl("cms", "-sign", "-binary", "-noattr", "-nocerts", "-md", "sha256", "-in", str(temp / "data"),
                    "-signer", str(temp / "leaf.pem"), "-inkey", str(temp / "key.der"), "-keyform", "DER",
                    "-outform", "DER", "-out", str(temp / "message.der"))
            openssl("cms", "-verify", "-binary", "-inform", "DER", "-in", str(temp / "message.der"),
                    "-content", str(temp / "data"), "-CAfile", str(temp / "root.pem"), "-certfile", str(temp / "chain.pem"),
                    "-auth_level", "0", "-purpose", "any", "-out", str(temp / "verified"))
            assert (temp / "verified").read_bytes() == formatted
            verity_signatures[f"verity_{algorithm}"] = helpers["embed"](
                (temp / "message.der").read_bytes(), b"".join(sorted(x["certificate"] for x in (intermediate, leaf))))
    values = {"root": root["certificate"], "collision": collision["certificate"], "wrong_id": wrong_id["certificate"], "leaf": leaf["certificate"],
              "intermediate": intermediate["certificate"], "data": data, "detached": message,
              "full": helpers["embed"](message, b"".join(sorted(x["certificate"] for x in (root, intermediate, leaf)))),
              "no_root": helpers["embed"](message, b"".join(sorted(x["certificate"] for x in (intermediate, leaf))))}
    values.update(verity_signatures)
    notice = source[:source.index("#ifndef _CRYPTO_TESTMGR_H")]
    out = notice + template[:template.index("#include")]
    out += "/* Generated public chain/collision fixtures from the pinned Linux test inputs. */\n"
    for label, value in values.items():
        out += f"static const u8 trust_chain_{label}[] = {{\n"
        for i in range(0, len(value), 16):
            out += '\t"' + "".join(f"\\x{v:02x}" for v in value[i:i+16]) + '"\n'
        out += "};\n"
    Path("tests/fixtures/asn1/trust-chain.h").write_text(out)


if __name__ == "__main__":
    main()
