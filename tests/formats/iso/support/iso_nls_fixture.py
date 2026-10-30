"""Joliet names spanning Linux NLS families and multibyte boundaries."""
import sys as _test_sys
from pathlib import Path as _TestPath
_test_sys.path.insert(0, str(_TestPath(__file__).resolve().parents[4]))
import struct


SAMPLES = {
    "ascii": ("café-空", "caf?-?"), "default": ("café-空", "café-?"),
    "cp437": ("é", "é"), "cp737": ("Ω", "Ω"), "cp775": ("Ā", "Ā"),
    "cp850": ("ø", "ø"), "cp852": ("Ł", "Ł"), "cp855": ("Ж", "Ж"),
    "cp857": ("ğ", "ğ"), "cp860": ("ã", "ã"), "cp861": ("ð", "ð"),
    "cp862": ("א", "א"), "cp863": ("É", "É"), "cp864": ("ﻻ", "ﻻ"),
    "cp865": ("ø", "ø"), "cp866": ("Я", "Я"), "cp869": ("Ω", "Ω"),
    "cp874": ("ก", "ก"), "cp1250": ("Ł", "Ł"), "cp1251": ("Я", "Я"),
    "cp1255": ("א", "א"), "cp932": ("日本語", "日本語"),
    "cp936": ("汉字€", "汉字€"), "cp949": ("한글", "한글"),
    "cp950": ("漢字", "漢字"), "euc-jp": ("日本語ｶ", "日本語ｶ"),
    "iso8859-1": ("é", "é"), "iso8859-2": ("Ł", "Ł"),
    "iso8859-3": ("Ħ", "Ħ"), "iso8859-4": ("Ā", "Ā"),
    "iso8859-5": ("Я", "Я"), "iso8859-6": ("ش", "ش"),
    "iso8859-7": ("Ω", "Ω"), "iso8859-9": ("ğ", "ğ"),
    "iso8859-13": ("Ģ", "Ģ"), "iso8859-14": ("Ŵ", "Ŵ"),
    "iso8859-15": ("€", "€"), "koi8-r": ("Ж", "Ж"),
    "koi8-u": ("ї", "ї"), "koi8-ru": ("Ўў", "Ўў"),
    "macceltic": ("Ŵ", "Ŵ"), "maccenteuro": ("Ł", "Ł"),
    "maccroatian": ("Č", "Č"), "maccyrillic": ("Я", "Я"),
    "macgaelic": ("Ḃ", "Ḃ"), "macgreek": ("Ω", "Ω"),
    "maciceland": ("ð", "ð"), "macinuit": ("ᐃ", "ᐃ"),
    "macroman": ("é", "é"), "macromanian": ("Ă", "Ă"),
    "macturkish": ("ğ", "ğ"),
}
ALIASES = {"sjis": "cp932", "gb2312": "cp936", "euc-kr": "cp949",
           "big5": "cp950", "tis-620": "cp874", "iso8859-8": "cp1255"}


def cases():
    return tuple(SAMPLES) + tuple(ALIASES) + ("euc-jp-extended", "euc-jp-long")


def charset(case):
    return "euc-jp" if case.startswith("euc-jp-") else case


def volume(kind):
    from tests.formats.iso.support.iso_fixture import volume as native_volume
    case = kind.removeprefix("names-joliet-nls-")
    if case == "euc-jp-extended":
        sample, expected = "ⅰ髙①\ue000\ue3ac", "ⅰ髙①??"
    elif case == "euc-jp-long":
        sample = expected = "ⅰ" * 90
    else:
        sample, expected = SAMPLES[ALIASES.get(case, case)]
    # Each surrogate is unrepresentable in the legacy NLS path. UTF-8 uses
    # the separate original utf16s_to_utf8s path and is covered independently.
    long_name = case == "euc-jp-long"
    sample = "case-" + sample + ("" if long_name else "-😀")
    expected = "case-" + expected + ("" if long_name else "-??")
    image, files = native_volume("joliet")
    image = bytearray(image)
    descriptor = 16 * 2048
    while image[descriptor] != 2:
        descriptor += 2048

    def number(pos):
        return struct.unpack_from("<I", image, pos)[0]

    def records(start, size):
        pos = start
        result = []
        while pos < start + size:
            length = image[pos]
            if not length:
                pos = (pos // 2048 + 1) * 2048
                continue
            result.append(bytearray(image[pos:pos + length]))
            pos += length
        return result

    def write(start, size, entries):
        data = bytearray(size)
        pos = 0
        for entry in entries:
            if len(entry) > 2048 - pos % 2048:
                pos = (pos // 2048 + 1) * 2048
            assert pos + len(entry) <= size
            data[pos:pos + len(entry)] = entry
            pos += len(entry)
        image[start:start + size] = data

    root = number(descriptor + 158) * 2048
    size = number(descriptor + 166)
    entries = records(root, size)
    for i, entry in enumerate(entries):
        if entry[32] <= 1:
            continue
        name = entry[33:33 + entry[32]].decode("utf-16-be")
        if name == "nested":
            child = struct.unpack_from("<I", entry, 2)[0] * 2048
            child_size = struct.unpack_from("<I", entry, 10)[0]
            # Keep unrelated payload names ASCII, so this fixture isolates
            # one encoding's representable and fallback characters.
            children = records(child, child_size)
            children = [r for r in children if r[32] <= 1 or
                        r[33:33 + r[32]].decode("utf-16-be").isascii()]
            write(child, child_size, children)
            replacement = sample + "-dir"
        elif name == "alpha.bin":
            replacement = sample + ".bin;1"
        else:
            continue
        suffix = entry[(33 + entry[32] + 1) & ~1:]
        raw = replacement.encode("utf-16-be")
        entry = entry[:33]
        entry[32] = len(raw)
        entry += raw
        entry += bytes(len(entry) % 2)
        entry += suffix
        assert len(entry) <= 255
        entry[0] = len(entry)
        entries[i] = entry
    write(root, size, entries)
    files = {name: data for name, data in files.items() if name.isascii()}
    alpha = files.pop("alpha.bin")
    files = {name.replace("nested/", expected + "-dir/"): data for name, data in files.items()}
    files[expected + ".bin"] = alpha
    return bytes(image), files
