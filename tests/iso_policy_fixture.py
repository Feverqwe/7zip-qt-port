"""Generated images for ISO read choices, legacy sizes and optional dates."""
import struct


PROFILES = {
    "policy-hidden": (), "policy-hide": ("hide",),
    "policy-associated": (), "policy-showassoc": ("showassoc",),
    "policy-unhide": ("unhide",), "policy-hidden-associated": ("hide", "showassoc"),
    "policy-associated-bad": (), "policy-nojoliet": ("nojoliet",),
    "policy-rock-nojoliet": ("nojoliet",),
    "policy-norock": ("norock",), "policy-norock-nojoliet": ("norock", "nojoliet"),
    "policy-norock-bad-px": ("norock",),
    "policy-nocompress": ("nocompress",), "policy-nocompress-bad": ("nocompress",),
    "policy-cruft": ("cruft",), "policy-cruft-multi": ("cruft",),
    "policy-sbsector": ("sbsector=512",), "policy-sbsector-sierra": ("sbsector=512",),
    "policy-date-east": (), "policy-date-invalid-zone": (),
    "policy-date-normalize": (), "policy-date-malformed": (), "policy-date-empty": (),
    "policy-leading-supplementary": ("nojoliet",), "policy-joliet-only": (),
    "policy-joliet-flags": (), "policy-first-joliet": (), "policy-unknown-joliet": (),
    "policy-descriptor-optional": (), "policy-norock-bad-primary": ("norock",),
    "policy-late-boot": (),
}


def volume(kind):
    from iso_fixture import volume as native_volume
    base = ("joliet" if kind in ("policy-nojoliet", "policy-leading-supplementary", "policy-joliet-only",
                                "policy-joliet-flags", "policy-first-joliet", "policy-unknown-joliet",
                                "policy-late-boot") else
            "rock-joliet" if kind in ("policy-norock", "policy-norock-nojoliet", "policy-rock-nojoliet") else
            "rock-joliet" if kind == "policy-norock-bad-primary" else
            "rock-root-mode" if kind == "policy-norock-bad-px" else
            "zisofs-crc" if kind == "policy-nocompress-bad" else
            "zisofs" if kind == "policy-nocompress" else
            "multi" if kind == "policy-cruft-multi" else
            "sierra" if kind == "policy-sbsector-sierra" else "plain")
    original, files = native_volume(base)
    image = bytearray(original)
    primary = 16 * 2048
    root_offset = 180 if base == "sierra" else 156
    def number(pos):
        return struct.unpack_from("<I", image, pos)[0]
    def both(pos, value):
        struct.pack_into("<I", image, pos, value)
        struct.pack_into(">I", image, pos + 4, value)
    root = number(primary + root_offset + 2) * 2048
    root_size = number(primary + root_offset + 10)
    def entries(start, size):
        pos = start
        result = []
        while pos < start + size:
            length = image[pos]
            if not length:
                pos = (pos // 2048 + 1) * 2048
                continue
            result.append(pos)
            pos += length
        return result
    positions = entries(root, root_size)
    named = {bytes(image[pos + 33:pos + 33 + image[pos + 32]]): pos for pos in positions}
    alpha = named[b"ALPHA.BIN;1"]
    nested = named[b"NESTED"]
    if kind in ("policy-hidden", "policy-hide", "policy-hidden-associated"):
        image[alpha + 25] |= 1
        image[nested + 25] |= 1
        if kind == "policy-hidden-associated":
            image[alpha + 25] |= 4
            image[nested + 25] |= 4
        if kind != "policy-hidden":
            files = {name: data for name, data in files.items()
                     if name != "ALPHA.BIN" and not name.startswith("NESTED/")}
    elif kind in ("policy-associated", "policy-showassoc", "policy-unhide", "policy-associated-bad"):
        image[alpha + 25] |= 4
        image[nested + 25] |= 4
        if kind in ("policy-associated", "policy-associated-bad"):
            files = {name: data for name, data in files.items()
                     if name != "ALPHA.BIN" and not name.startswith("NESTED/")}
        if kind == "policy-associated-bad":
            child = number(nested + 2) * 2048
            image[child + 33] = 1
    elif kind == "policy-nojoliet":
        files = native_volume("plain")[1]
    elif kind == "policy-rock-nojoliet":
        pass
    elif kind in ("policy-norock", "policy-norock-bad-primary"):
        files = {name: data for name, data in files.items()
                 if name in ("alpha.bin", "nested/данные 空.txt")}
        if kind == "policy-norock-bad-primary":
            both(primary + 80, 0xfffffffd)
            both(primary + 158, len(image) // 2048 + 100)
            struct.pack_into("<H", image, primary + 128, 511)
    elif kind in ("policy-norock-nojoliet", "policy-norock-bad-px"):
        files = {"ALPHA.BIN": files["alpha.bin"]}
    elif kind.startswith("policy-nocompress"):
        # nocompress exposes the stored zisofs stream, including bad zlib data.
        packed = image[number(alpha + 2) * 2048:number(alpha + 2) * 2048 + number(alpha + 10)]
        files = {"alpha.bin": bytes(packed)}
    elif kind == "policy-cruft":
        both(alpha + 10, number(alpha + 10) | 0xab000000)
        both(root + 10, root_size | 0xcd000000)
        child = number(nested + 2) * 2048
        both(child + 10, number(child + 10) | 0xef000000)
        empty = named[b"EMPTY.;1"]
        both(empty + 10, 0xab000000)
        both(empty + 2, len(image) // 2048 + 100)
    elif kind == "policy-cruft-multi":
        first = next(pos for pos in positions if image[pos + 25] & 128)
        both(first + 10, number(first + 10) | 0xab000000)
        # Only the declared logical prefix is mapped. The unused second
        # physical section must not reject the readable first-section prefix.
        both(first + image[first] + 2, len(image) // 2048 + 100)
    elif kind.startswith("policy-sbsector"):
        end = primary
        while base != "sierra" and image[end] != 255:
            end += 2048
        descriptors = bytes(image[primary:end + 2048])
        image[primary:end + 2048] = bytes(len(descriptors))
        start = (512 + 16) * 2048
        image.extend(bytes(start + len(descriptors) - len(image)))
        image[start:start + len(descriptors)] = descriptors
    elif kind.startswith("policy-date-"):
        stamp = bytearray(b"2041010203040537\0")
        if kind == "policy-date-east":
            stamp[-1] = 52
        elif kind == "policy-date-invalid-zone":
            stamp[-1] = 53
        elif kind == "policy-date-normalize":
            stamp[6:8] = b"32"
        elif kind == "policy-date-malformed":
            stamp[4] = ord('X')
        elif kind == "policy-date-empty":
            stamp = bytearray(17)
        else:
            raise ValueError(kind)
        image[primary + 813:primary + 830] = stamp
        image[primary + 830:primary + 847] = stamp
    elif kind == "policy-leading-supplementary":
        saved = bytes(image[primary:primary + 2048])
        image[primary] = 2
        both(primary + 158, len(image) // 2048 + 100)
        image[primary + 188] = 255
        image[primary + 72:primary + 80] = b"ignored!"
        image[primary + 2048:primary + 4096] = saved
        files = native_volume("plain")[1]
        files.pop("[BOOT]/Boot-NoEmul.img")
    elif kind == "policy-joliet-only":
        image[primary] = 3
    elif kind == "policy-late-boot":
        boot = bytes(image[17 * 2048:18 * 2048])
        terminator = bytes(image[19 * 2048:20 * 2048])
        image[17 * 2048] = 3
        image[19 * 2048:20 * 2048] = boot
        image[20 * 2048:21 * 2048] = terminator
    elif kind in ("policy-joliet-flags", "policy-first-joliet", "policy-unknown-joliet"):
        secondary = primary
        while image[secondary] != 2:
            secondary += 2048
        if kind == "policy-joliet-flags":
            image[secondary + 7] |= 1
        else:
            image[secondary + 2048:secondary + 4096] = image[secondary:secondary + 2048]
            if kind == "policy-first-joliet":
                both(secondary + 2048 + 158, len(image) // 2048 + 100)
            else:
                image[secondary + 90] = ord('Z')
                files = native_volume("plain")[1]
    elif kind == "policy-descriptor-optional":
        image[primary + 72:primary + 80] = b"ignored!"
        image[primary + 882] = 0x9a
        image[primary + 188] = 255
        image[primary + 156] = 0
    else:
        raise ValueError(kind)
    return bytes(image), files
