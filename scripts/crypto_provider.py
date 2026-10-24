#!/usr/bin/env python3
"""Pass the configured crypto dependency to the original native makefiles."""

import argparse
import hashlib
import json
from pathlib import Path
import platform
import re
import shlex


ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_config(root):
    path = root / "build/crypto-provider.json"
    if not path.is_file():
        raise ValueError("Configure the required OpenSSL dependency before preparing the native runtime")
    data = json.loads(path.read_text())
    if (data.get("schema") != 1 or data.get("provider") != "OPENSSL" or
            data.get("system") != platform.system()):
        raise ValueError("Crypto configuration is incompatible; configure this platform with CMake")
    for field in ("definitions", "includes", "libraries"):
        values = data.get(field)
        if not isinstance(values, list) or any(not isinstance(v, str) or not v or
                any(c in v for c in '\r\n\0') for v in values):
            raise ValueError("Invalid crypto " + field)
    if any(not re.fullmatch(r"[A-Za-z_]\w*(=[^\r\n\0]*)?", v)
           for v in data["definitions"]):
        raise ValueError("Invalid crypto definition")
    if not re.fullmatch(r"[0-9.]*", data.get("deployment_target", "")):
        raise ValueError("Invalid crypto deployment target")
    return data


def fingerprint(root):
    data = load_config(root)
    libraries = {}
    for name in data["libraries"]:
        path = Path(name)
        if path.is_file():
            libraries[name] = sha(path)
        elif path.is_absolute():
            raise ValueError("Missing configured crypto library: " + name)
    headers = {}
    for directory in data["includes"]:
        # A package can replace API headers without changing its version or
        # library bytes. These headers compile into the local adapters too.
        for path in sorted((Path(directory) / "openssl").rglob("*.h")):
            if path.is_file():
                headers[str(path)] = sha(path)
    return {"configuration": data, "library_sha256": libraries, "header_sha256": headers}


def public_record(root):
    record = fingerprint(root)
    config = record["configuration"]
    return {"provider": config["provider"], "version": config["version"],
            "definitions": config["definitions"],
            "library_sha256": {Path(name).name: digest
                               for name, digest in record["library_sha256"].items()}}


def write_if_changed(path, content):
    if path.is_symlink():
        raise ValueError("Refusing a linked crypto runtime input: " + str(path))
    if not path.exists() or path.read_text() != content:
        path.write_text(content)


def argument(value, windows):
    if windows:
        if any(c in value for c in '"%'):
            raise ValueError("Unsupported quote/percent in native crypto argument")
        # Always quote: cmd metacharacters need protection even without spaces.
        trailing = len(value) - len(value.rstrip("\\"))
        quoted = '"' + value + "\\" * trailing + '"'
        return quoted.replace("$", "$$")
    else:
        quoted = shlex.quote(value)
    # Make expands dollar signs even inside shell quotes.
    return quoted.replace("$", "$$").replace("#", "\\#")


def prerequisite(value, windows):
    if windows:
        return argument(value, True)
    return value.replace("$", "$$").replace("#", "\\#").replace(" ", "\\ ")


def stage(root, snapshot):
    record = fingerprint(root)
    config = record["configuration"]
    native_rules = snapshot / "CPP/Build.mak"
    if native_rules.is_file():
        # Nmake's upstream compile/assembler macros pass all prerequisites as
        # input files. Retain their commands and batching, but exclude metadata
        # and headers from that argument list. Keep ARM64 .S inputs too.
        if native_rules.is_symlink():
            raise ValueError("Refusing linked native build rules")
        original = native_rules.read_bytes()
        # Apply this before Build.mak reads its link rule. No original AES/SHA
        # optimized objects are part of the required-OpenSSL core.
        drop = " ".join("%/" + name + ".obj %\\" + name + ".obj"
                        for name in ("AesOpt", "Sha1Opt", "Sha256Opt", "Sha512Opt"))
        prefix = ("OBJS = $(filterout " + drop + ",$(OBJS))\n").encode()
        adapted = original.replace(b"$**", b"$(SEVENZIP_CRYPTO_SOURCES)")
        anchor = b"PROGPATH ="
        if anchor in adapted:
            adapted = adapted.replace(anchor, prefix + anchor, 1)
        else:
            adapted = prefix + adapted
        if prefix in original:
            adapted = original
        if adapted != original:
            native_rules.write_bytes(adapted)
    state = snapshot / "crypto-provider-inputs.json"
    write_if_changed(state, json.dumps(record, sort_keys=True, indent=2) + "\n")
    header_state = snapshot / "crypto-header-inputs.json"
    write_if_changed(header_state, json.dumps(record["header_sha256"], sort_keys=True, indent=2) + "\n")
    for directory in ("CPP/7zip/UI/Console", "CPP/7zip/Bundles/Format7zF"):
        bundle = snapshot / directory
        for windows, suffix in ((False, "gcc.mak"), (True, "mak")):
            main = bundle / ("sevenzip-runtime." + suffix)
            if not main.is_file():
                continue
            name = "sevenzip-crypto." + suffix
            include = ('!include "' + name + '"' if windows else 'include ' + name) + "\n"
            post_include = '!include "sevenzip-crypto-deps.mak"\n' if windows else ""
            flags = ["-I../../../..", "-I../../.."]
            flags += ["-I" + path for path in config["includes"]]
            flags += ["-D" + definition for definition in config["definitions"]]
            if not windows and config["deployment_target"]:
                flags.append("-mmacosx-version-min=" + config["deployment_target"])
            links = []
            for value in config["libraries"]:
                if Path(value).is_absolute() or value.startswith("-"):
                    links.append(value)
                else:
                    links.append(value if windows and value.lower().endswith(".lib") else
                                 value + ".lib" if windows else "-l" + value)
            if not windows and config["deployment_target"]:
                links.append("-mmacosx-version-min=" + config["deployment_target"])
            compile_text = " ".join(argument(v, windows) for v in flags)
            link_text = " ".join(argument(v, windows) for v in links)
            dependency = prerequisite("../../../../crypto-provider-inputs.json", windows)
            lines = (["SEVENZIP_CRYPTO_SOURCES = $(filteri %.c %.cpp %.asm %.s %.rc,$**)",
                      "CFLAGS = $(CFLAGS) " + compile_text, "LIBS = $(LIBS) " + link_text]
                     if windows else ["LOCAL_FLAGS += " + compile_text, "MY_LIBS += " + link_text])
            # Consumers may include provider declarations too. Header changes
            # invalidate their objects, while library-only changes just relink
            # the engine and refresh the adapters below.
            dependencies = ["$(OBJS): " + name + " " +
                            prerequisite("../../../../crypto-header-inputs.json", windows)]
            dependencies.append("$(PROGPATH): " + dependency + " " + " ".join(
                prerequisite(path, windows) for path in record["library_sha256"]))
            # Header/library changes can alter the adapter ABI without changing
            # the native callers. Rebuild adapters and relink, retaining outputs.
            for source in ("opensslaes", "opensslblock", "opensslkdf", "opensslhash"):
                target = ("$(O)\\reference_crypto_" + source + ".obj" if windows else
                          "$(O)/reference_crypto_" + source + ".o")
                dependencies.append(target + ": " + dependency)
            if windows:
                # OBJS is assigned by the upstream include, while LIBS/CFLAGS
                # must be visible before it reads the corresponding commands.
                write_if_changed(bundle / "sevenzip-crypto-deps.mak", "\n".join(dependencies) + "\n")
            else:
                lines += dependencies
            write_if_changed(bundle / name, "\n".join(lines) + "\n")
            content = main.read_text().replace(include, "")
            if post_include:
                content = content.replace(post_include, "")
            # Nmake consumes LIBS when reading the upstream link rule. GNU
            # make's recursively expanded flags are appended after its defaults.
            content = include + content + post_include if windows else content + include
            write_if_changed(main, content)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("stage", "record"))
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--snapshot", type=Path)
    args = parser.parse_args()
    if args.command == "stage":
        if not args.snapshot:
            parser.error("stage requires --snapshot")
        stage(args.root, args.snapshot)
    else:
        print(json.dumps(public_record(args.root)))


if __name__ == "__main__":
    main()
