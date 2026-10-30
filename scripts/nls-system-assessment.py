#!/usr/bin/env python3
"""Compare a pre-conversion NLS snapshot with OS services; no product changes.

Builds the explicitly selected historical C owners using CMake/ccache.
The result is diagnostic evidence, not a new expected-value baseline.
"""
import argparse
import collections
import ctypes as C
import ctypes.util
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
sys.dont_write_bytecode = True
from lib.build_paths import checked_output, directory
from lib.toolchain import apply_environment

ROOT = Path(__file__).resolve().parents[1]
U16 = C.c_uint16
ENC = C.CFUNCTYPE(C.c_int, U16, C.c_void_p, C.c_int)
DEC = C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_int, C.POINTER(U16))


class Table(C.Structure):
    _fields_ = [("charset", C.c_char_p), ("encode", ENC), ("decode", DEC),
                ("lower", C.c_void_p), ("upper", C.c_void_p), ("alias", C.c_char_p)]


def reference(output, source_root, reuse=False):
    project = output / "reference"
    project.mkdir(parents=True, exist_ok=True)
    if not reuse and not (source_root / "src/text/nls/nls_cp437.c").is_file():
        raise ValueError("--reference-root must contain the pre-conversion NLS owners; "
                         "use the documented baseline snapshot or --reuse-reference")
    sources = sorted((source_root / "src/text/nls").glob("*.c"))
    paths = "\n".join(json.dumps(p.as_posix()) for p in sources)
    source_root = source_root.as_posix()
    cmake = f'''cmake_minimum_required(VERSION 3.21)
project(nls_probe LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
find_program(CACHE_PROGRAM ccache REQUIRED)
set(CMAKE_C_COMPILER_LAUNCHER "${{CACHE_PROGRAM}}")
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(nls_probe SHARED {paths})
target_include_directories(nls_probe PRIVATE "{source_root}/src" "{source_root}/third_party/linux/include")
'''
    build = project / "build"
    if not reuse:
        (project / "CMakeLists.txt").write_text(cmake)
        environment = dict(os.environ)
        subprocess.run(["cmake", "-S", str(project), "-B", str(build), "-G", "Ninja",
                        "-DCMAKE_BUILD_TYPE=Release"], check=True, env=environment)
        subprocess.run(["cmake", "--build", str(build), "--parallel", "2"], check=True, env=environment)
    name = "nls_probe.dll" if os.name == "nt" else (
        "libnls_probe.dylib" if sys.platform == "darwin" else "libnls_probe.so")
    lib = C.CDLL(str(build / name))
    lib.sevenzip_nls_table.argtypes = [C.c_int]
    lib.sevenzip_nls_table.restype = C.POINTER(Table)
    return lib


def candidates(name):
    special = {
        "default": ["ISO-8859-1"], "macroman": ["MACINTOSH", "MACROMAN"],
        "maccenteuro": ["MAC-CENTRALEUROPE", "MACCENTRALEUROPE"],
        "maciceland": ["MAC-ICELAND", "MACICELAND", "MAC-IS"],
        "macromanian": ["MAC-ROMANIA", "MACROMANIA"],
    }
    return special.get(name, [name, name.replace("mac", "MAC-", 1)])


class Iconv:
    def __init__(self, name):
        path = ctypes.util.find_library("iconv") if sys.platform == "darwin" else None
        self.lib = C.CDLL(path, use_errno=True)
        prefix = "" if hasattr(self.lib, "iconv_open") else "lib"
        self.open = getattr(self.lib, prefix + "iconv_open")
        self.run = getattr(self.lib, prefix + "iconv")
        self.close = getattr(self.lib, prefix + "iconv_close")
        self.open.argtypes = [C.c_char_p, C.c_char_p]
        self.open.restype = C.c_void_p
        self.close.argtypes = [C.c_void_p]
        self.run.argtypes = [C.c_void_p, C.POINTER(C.c_void_p), C.POINTER(C.c_size_t),
                             C.POINTER(C.c_void_p), C.POINTER(C.c_size_t)]
        self.run.restype = C.c_size_t
        self.label = None
        for label in candidates(name):
            e = self.open(label.encode(), b"UTF-16LE")
            d = self.open(b"UTF-16LE", label.encode())
            if e != C.c_void_p(-1).value and d != C.c_void_p(-1).value:
                self.enc, self.dec, self.label = e, d, label
                break
            for descriptor in (e, d):
                if descriptor != C.c_void_p(-1).value:
                    self.close(descriptor)

    def convert(self, descriptor, data, capacity):
        source = C.create_string_buffer(data)
        target = C.create_string_buffer(capacity)
        ip, op = C.c_void_p(C.addressof(source)), C.c_void_p(C.addressof(target))
        ni, no = C.c_size_t(len(data)), C.c_size_t(capacity)
        self.run(descriptor, None, None, None, None)
        n = self.run(descriptor, C.byref(ip), C.byref(ni), C.byref(op), C.byref(no))
        if n == 0 and not ni.value:
            n = self.run(descriptor, None, None, C.byref(op), C.byref(no))
        if n == C.c_size_t(-1).value or n != 0 or ni.value:
            return None
        return target.raw[:capacity - no.value]

    def encode(self, u):
        return self.convert(self.enc, u.to_bytes(2, "little"), 8)

    def decode(self, data):
        result = self.convert(self.dec, data, 8)
        return int.from_bytes(result, "little") if result and len(result) == 2 else None

    def finish(self):
        if self.label:
            self.close(self.enc)
            self.close(self.dec)


def windows_codepage(name):
    if name.startswith("cp"):
        return int(name[2:])
    if name.startswith("iso8859-"):
        return 28590 + int(name.split("-")[1])
    return {"ascii": 20127, "default": 28591, "euc-jp": 20932,
            "koi8-r": 20866, "koi8-u": 21866, "macroman": 10000,
            "macgreek": 10006, "maccyrillic": 10007, "macromanian": 10010,
            "maccenteuro": 10029, "maciceland": 10079, "macturkish": 10081,
            "maccroatian": 10082}.get(name)


class Windows:
    def __init__(self, name):
        self.lib = C.WinDLL("kernel32", use_last_error=True)
        self.mb = self.lib.MultiByteToWideChar
        self.mb.argtypes = [C.c_uint, C.c_uint, C.c_void_p, C.c_int, C.c_void_p, C.c_int]
        self.wc = self.lib.WideCharToMultiByte
        self.wc.argtypes = [C.c_uint, C.c_uint, C.c_void_p, C.c_int, C.c_void_p,
                            C.c_int, C.c_void_p, C.POINTER(C.c_int)]
        self.cp = windows_codepage(name)
        self.label = str(self.cp) if self.cp and self.lib.IsValidCodePage(self.cp) else None

    def encode(self, u):
        source, target, default = U16(u), C.create_string_buffer(8), C.c_int()
        n = self.wc(self.cp, 0x400, C.byref(source), 1, target, 8, None, C.byref(default))
        return target.raw[:n] if n and not default.value else None

    def decode(self, data):
        target = (U16 * 4)()
        n = self.mb(self.cp, 8, data, len(data), target, 4)
        return target[0] if n == 1 else None

    def finish(self):
        pass


class CFRange(C.Structure):
    _fields_ = [("location", C.c_long), ("length", C.c_long)]


class CoreFoundation:
    def __init__(self, name):
        self.lib = C.CDLL("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation")
        signatures = {
            "CFStringCreateWithBytes": ([C.c_void_p, C.c_void_p, C.c_long, C.c_uint, C.c_bool], C.c_void_p),
            "CFStringCreateWithCharacters": ([C.c_void_p, C.c_void_p, C.c_long], C.c_void_p),
            "CFStringGetLength": ([C.c_void_p], C.c_long),
            "CFStringGetCharacters": ([C.c_void_p, CFRange, C.c_void_p], None),
            "CFStringGetBytes": ([C.c_void_p, CFRange, C.c_uint, C.c_ubyte, C.c_bool,
                                  C.c_void_p, C.c_long, C.POINTER(C.c_long)], C.c_long),
            "CFRelease": ([C.c_void_p], None),
            "CFStringConvertWindowsCodepageToEncoding": ([C.c_uint], C.c_uint),
            "CFStringIsEncodingAvailable": ([C.c_uint], C.c_bool),
        }
        for method, (args, result) in signatures.items():
            fn = getattr(self.lib, method)
            fn.argtypes, fn.restype = args, result
        mac = {"macroman": 0, "macgreek": 6, "maccyrillic": 7, "maccenteuro": 29,
               "macturkish": 35, "maccroatian": 36, "maciceland": 37,
               "macromanian": 38, "macceltic": 39, "macgaelic": 40, "macinuit": 0xec}
        extra = {"euc-jp": 0x920, "koi8-r": 0xa02, "koi8-u": 0xa08,
                 "ascii": 0x600, "default": 0x201}
        self.encoding = mac.get(name, extra.get(name))
        if self.encoding is None:
            cp = windows_codepage(name)
            self.encoding = self.lib.CFStringConvertWindowsCodepageToEncoding(cp) if cp else 0xffffffff
        self.label = hex(self.encoding) if self.lib.CFStringIsEncodingAvailable(self.encoding) else None

    def encode(self, u):
        source, target, used = U16(u), C.create_string_buffer(8), C.c_long()
        string = self.lib.CFStringCreateWithCharacters(None, C.byref(source), 1)
        n = self.lib.CFStringGetBytes(string, CFRange(0, 1), self.encoding, 0, False,
                                      target, 8, C.byref(used))
        self.lib.CFRelease(string)
        return target.raw[:used.value] if n == 1 and used.value else None

    def decode(self, data):
        string = self.lib.CFStringCreateWithBytes(None, data, len(data), self.encoding, False)
        if not string:
            return None
        unit = U16()
        size = self.lib.CFStringGetLength(string)
        if size == 1:
            self.lib.CFStringGetCharacters(string, CFRange(0, 1), C.byref(unit))
        self.lib.CFRelease(string)
        return unit.value if size == 1 else None

    def finish(self):
        pass


class Adapter:
    library = None

    @classmethod
    def prepare(cls, output):
        project = output / "adapter"
        project.mkdir(parents=True, exist_ok=True)
        root = ROOT.as_posix()
        (project / "CMakeLists.txt").write_text(f'''cmake_minimum_required(VERSION 3.21)
project(nls_adapter LANGUAGES C)
set(CMAKE_C_STANDARD 11)
find_program(CACHE_PROGRAM ccache REQUIRED)
set(CMAKE_C_COMPILER_LAUNCHER "${{CACHE_PROGRAM}}")
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(nls_adapter SHARED "{root}/src/text/nls/registry.c" "{root}/src/text/nls/system.c")
target_include_directories(nls_adapter PRIVATE "{root}/src")
if(NOT WIN32)
    find_package(Iconv REQUIRED)
    target_link_libraries(nls_adapter PRIVATE Iconv::Iconv)
endif()
''')
        environment = dict(os.environ)
        build = project / "build"
        subprocess.run(["cmake", "-S", str(project), "-B", str(build), "-G", "Ninja",
                        "-DCMAKE_BUILD_TYPE=Release"], check=True, env=environment)
        subprocess.run(["cmake", "--build", str(build), "--parallel", "2"], check=True, env=environment)
        filename = "nls_adapter.dll" if os.name == "nt" else (
            "libnls_adapter.dylib" if sys.platform == "darwin" else "libnls_adapter.so")
        cls.library = C.CDLL(str(build / filename))
        cls.library.sevenzip_iso_charset.argtypes = [C.c_char_p]
        cls.library.sevenzip_nls_table.argtypes = [C.c_int]
        cls.library.sevenzip_nls_table.restype = C.POINTER(Table)

    def __init__(self, name):
        number = self.library.sevenzip_iso_charset(name.encode())
        pointer = self.library.sevenzip_nls_table(number)
        self.label = name if pointer else None
        self.table = pointer.contents if pointer else None

    def encode(self, u):
        output = C.create_string_buffer(8)
        n = self.table.encode(u, output, 8)
        return output.raw[:n] if n > 0 else None

    def decode(self, data):
        unit = U16()
        n = self.table.decode(data, len(data), C.byref(unit))
        return unit.value if n == len(data) else None

    def finish(self):
        pass


def record(stats, examples, category, value):
    stats[category] += 1
    if len(examples[category]) < 8:
        examples[category].append(value)


def compare(table, native):
    stats, examples = collections.Counter(), collections.defaultdict(list)
    if not native.label:
        return {"available": False}
    buffer, unit = C.create_string_buffer(8), U16()
    roundtrips = {}
    for u in range(1, 65536):
        if 0xd800 <= u <= 0xdfff:
            continue
        n = table.encode(u, buffer, 8)
        old = buffer.raw[:n] if n > 0 else None
        new = native.encode(u)
        if old != new:
            kind = "encode_lost" if old and not new else "encode_added" if new and not old else "encode_changed"
            record(stats, examples, kind, {"unicode": f"U+{u:04X}", "nls": old.hex() if old else None,
                                          "system": new.hex() if new else None})
        # Actual Joliet result: unrepresentable units become '?' before decode.
        old_bytes, new_bytes = old or b"?", new or b"?"
        r = table.decode(old_bytes, len(old_bytes), C.byref(unit))
        old_u = unit.value if r > 0 else None
        if new_bytes not in roundtrips:
            roundtrips[new_bytes] = native.decode(new_bytes)
        new_u = roundtrips[new_bytes]
        if old_u != new_u:
            record(stats, examples, "joliet_changed", {"input": f"U+{u:04X}", "nls": old_u, "system": new_u})
    # Contract: decode one character, returning consumed byte count, not a whole string.
    # Every first-byte/pair; EUC-JP also every 0x8f-prefixed pair, including malformed ones.
    one = {bytes([i]): native.decode(bytes([i])) for i in range(256)}
    for size in (1, 2, 3) if table.charset == b"euc-jp" else (1, 2):
        count = 256 if size == 1 else 65536
        for i in range(count):
            data = bytes([i]) if size == 1 else i.to_bytes(2, "big")
            if size == 3:
                data = b"\x8f" + data
            if data[0] == 0:
                continue  # NUL is not an on-disk filename character.
            n = table.decode(data, len(data), C.byref(unit))
            old = (n, unit.value) if n > 0 else None
            v = one[data[:1]]
            new = (1, v) if v is not None else None
            if new is None and size >= 2:
                v = native.decode(data[:2])
                if v is not None:
                    new = (2, v)
            if new is None and size == 3:
                v = native.decode(data)
                if v is not None:
                    new = (3, v)
            if old != new:
                kind = "decode_lost" if old and not new else "decode_added" if new and not old else "decode_changed"
                record(stats, examples, kind, {"bytes": data.hex(), "nls": old, "system": new})
    native.finish()
    return {"available": True, "converter": native.label, "counts": stats, "examples": examples}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--backend", choices=["iconv", "windows", "cf", "adapter"])
    parser.add_argument("--charset", action="append")
    reference_mode = parser.add_mutually_exclusive_group(required=True)
    reference_mode.add_argument("--reference-root", type=Path,
                                help="Snapshot containing the pre-conversion NLS owners")
    reference_mode.add_argument("--reuse-reference", action="store_true",
                                help="Reuse the reference library already built under --output")
    args = parser.parse_args()
    apply_environment(ROOT, require_qt=False)
    args.output = checked_output(ROOT, args.output or directory(ROOT, "artifacts") / "review/nls-system")
    args.output.mkdir(parents=True, exist_ok=True)
    lib = reference(args.output, (args.reference_root or ROOT).resolve(), args.reuse_reference)
    backend = args.backend or ("windows" if os.name == "nt" else "iconv")
    if backend == "adapter":
        Adapter.prepare(args.output)
    factory = {"iconv": Iconv, "windows": Windows, "cf": CoreFoundation, "adapter": Adapter}[backend]
    result = {"platform": platform.platform(), "python": platform.python_version(),
              "backend": backend, "libc": platform.libc_ver(), "charsets": {}}
    if backend == "adapter":
        original = lib.sevenzip_nls_table(1).contents
        lower = C.cast(original.lower, C.POINTER(C.c_ubyte))
        Adapter.library.fat_cp437_lower.argtypes = [C.c_ubyte]
        Adapter.library.fat_cp437_lower.restype = C.c_ubyte
        result["cp437_lower_differences"] = [
            {"byte": i, "nls": lower[i] or i, "system": Adapter.library.fat_cp437_lower(i)}
            for i in range(256) if (lower[i] or i) != Adapter.library.fat_cp437_lower(i)]
    i = 1
    while True:
        pointer = lib.sevenzip_nls_table(i)
        if not pointer:
            break
        table = pointer.contents
        name = table.charset.decode()
        if not args.charset or name in args.charset:
            row = compare(table, factory(name))
            result["charsets"][name] = row
            print(name, json.dumps({k: v for k, v in row.items() if k != "examples"}), flush=True)
            suffix = "-selected" if args.charset else ""
            (args.output / (backend + suffix + ".json")).write_text(json.dumps(result, indent=2) + "\n")
        i += 1
    print("NLS_SYSTEM_RESULT=" + json.dumps(result), flush=True)


if __name__ == "__main__":
    main()
