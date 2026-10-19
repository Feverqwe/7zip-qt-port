"""UDF inode permissions, ownership, special-file EA and timestamp variants."""
import binascii
import datetime
import struct

CASES = ("ia-owner", "ia-overflow", "ia-high-owner", "ia-permissions", "ia-extra-permissions",
         "ia-links", "ia-directory", "ia-undef", "ia-vat-type", "ia-realtime",
         "ia-char", "ia-block", "ia-fifo", "ia-socket", "ia-device-chain",
         "ia-device-subtypes", "ia-device-tag-crc", "ia-device-max", "ia-unaligned-ea",
         "ia-efe-owner", "ia-efe-device", "ia-hidden-zero-links",
         "ia-time-zone", "ia-time-negative-zone", "ia-time-unspecified", "ia-time-type0",
         "ia-time-type2", "ia-time-fraction", "ia-time-bad-fraction", "ia-time-leap",
         "ia-time-month", "ia-time-day")
BAD = ("ia-zero-links", "ia-zero-directory-links", "ia-bad-type", "ia-device-missing",
       "ia-device-tag-id", "ia-device-tag-location", "ia-device-zero-attr",
       "ia-device-over-attr", "ia-device-wrong-subtype", "ia-ea-outside")
# The kernel may read fields beyond their declared EA/attribute sizes. Do not
# accept that as a decoding contract or exercise those branches in a live kernel.
MODULE_BAD = ("ia-device-short-header", "ia-device-short-attr")
METADATA_ONLY = ("ia-device-payload",)


def build(kind):
    from udf_fixture import volume as base
    source = "icb-efe" if kind.startswith("ia-efe-") else "metadata-bootstrap" if kind == "ia-hidden-zero-links" else "native"
    image, files = base(source)
    image = bytearray(image)
    block = 2048
    sector = 280 if kind.startswith("ia-efe-") else 261 if kind in ("ia-directory", "ia-zero-directory-links") else 277 if kind == "ia-hidden-zero-links" else 263
    pos = sector * block
    extended = image[pos:pos + 2] == b"\x0a\x01"
    start = 216 if extended else 176
    name = "nested" if sector == 261 else "alpha.bin"
    mode = 0o100754
    uid, gid, links = 1042, 2043, 7
    if kind == "ia-overflow": uid = gid = 0xffffffff
    if kind == "ia-high-owner": uid, gid = 0xfffffffe, 0x80000000
    if kind == "ia-permissions": mode = 0o107651
    if kind == "ia-extra-permissions": mode = 0o100777
    if kind == "ia-links": links = 65535
    if kind == "ia-directory": mode, links = 0o42750, 65535
    if kind in ("ia-zero-links", "ia-zero-directory-links", "ia-hidden-zero-links"): links = 0
    if kind == "ia-hidden-zero-links": mode = 0o754
    wire_type = image[pos + 27]
    if kind == "ia-undef": wire_type = 0
    if kind == "ia-vat-type": wire_type = 248
    if kind == "ia-realtime": wire_type = 249
    if kind == "ia-bad-type": wire_type = 8
    device = kind.startswith("ia-device-") or kind in ("ia-char", "ia-block", "ia-efe-device")
    special = device or kind in ("ia-fifo", "ia-socket")
    if special:
        wire_type = 6 if kind == "ia-block" else 9 if kind == "ia-fifo" else 10 if kind == "ia-socket" else 7
        mode = (0o60000 if wire_type == 6 else 0o20000 if wire_type == 7 else 0o10000 if wire_type == 9 else 0o140000) | 0o754
        struct.pack_into("<Q", image, pos + 56, 123 if kind == "ia-device-payload" else 0)
        files["alpha.bin"] = b""
    image[pos + 27] = wire_type
    permissions = ((mode & 0o700) << 4) | ((mode & 0o70) << 2) | (mode & 7)
    if kind == "ia-extra-permissions": permissions = 0xffffffff
    struct.pack_into("<IIIH", image, pos + 36, uid, gid, permissions, links)
    flags = struct.unpack_from("<H", image, pos + 34)[0] & ~0x1c0
    flags |= 0x40 if mode & 0o4000 else 0
    flags |= 0x80 if mode & 0o2000 else 0
    flags |= 0x100 if mode & 0o1000 else 0
    struct.pack_into("<H", image, pos + 34, flags)

    def tag(at, length):
        struct.pack_into("<HH", image, at + 8,
                         binascii.crc_hqx(image[at + 16:at + length], 0), length - 16)
        image[at + 4] = 0
        image[at + 4] = sum(image[at:at + 16]) & 255

    ea = b""
    if device and kind != "ia-device-missing":
        major, minor = (0xffffffff, 0xffffffff) if kind == "ia-device-max" else (241, 74565)
        header = bytearray(24)
        struct.pack_into("<HH", header, 0, 262, 2)
        struct.pack_into("<I", header, 12, sector - 257)
        leaf = struct.pack("<IB3xIIII", 12, 1, 24, 0, major, minor)
        before = struct.pack("<IB3xI", 1, 1, 12) if kind == "ia-device-chain" else struct.pack("<IB3xIIII", 12, 2, 24, 0, 3, 4) if kind == "ia-device-subtypes" else b""
        ea = header + before + leaf
        if kind == "ia-device-tag-id": struct.pack_into("<H", ea, 0, 261)
        if kind == "ia-device-tag-location": struct.pack_into("<I", ea, 12, 999)
        if kind == "ia-device-zero-attr": struct.pack_into("<I", ea, 24 + 8, 0)
        if kind == "ia-device-over-attr": struct.pack_into("<I", ea, 24 + 8, 0xffffffff)
        if kind == "ia-device-wrong-subtype": ea[24 + 4] = 2
        if kind == "ia-device-short-attr": struct.pack_into("<I", ea, 24 + 8, 12)
        if kind == "ia-device-short-header": ea = ea[:16]
        if kind == "ia-device-tag-crc":
            ea[4] = 0xfa
            struct.pack_into("<HH", ea, 8, 0xdead, 0xffff)
    elif kind == "ia-unaligned-ea": ea = b"\xff"
    ad_length = struct.unpack_from("<I", image, pos + start - 4)[0]
    ad = bytes(image[pos + start:pos + start + ad_length])
    struct.pack_into("<I", image, pos + start - 8, len(ea))
    image[pos + start:pos + block] = ea + ad + bytes(block - start - len(ea) - len(ad))
    if kind == "ia-ea-outside": struct.pack_into("<I", image, pos + start - 8, block)
    attr = {"mode": mode, "uid": 65534 if uid == 0xffffffff else uid,
            "gid": 65534 if gid == 0xffffffff else gid,
            "links": links + int(wire_type == 4)}
    if special: attr["size"] = 123 if kind == "ia-device-payload" else 0
    if device:
        attr["major"], attr["minor"] = (4095, 1048575) if kind == "ia-device-max" else (241, 74565)
    if kind.startswith("ia-time-"):
        year, month, day, hour, minute, second = 2040, 2, 3, 4, 5, 6
        type, offset = 1, 0
        sub = (12, 34, 56)
        if kind == "ia-time-zone": offset = 360
        if kind == "ia-time-negative-zone": offset = -90
        if kind == "ia-time-unspecified": offset = -2047
        if kind == "ia-time-type0": type, offset = 0, 360
        if kind == "ia-time-type2": type, offset = 2, -90
        if kind == "ia-time-bad-fraction": sub = (12, 100, 56)
        if kind == "ia-time-leap": second = 60
        if kind == "ia-time-month": month = 13
        if kind == "ia-time-day": day = 0
        stamp = struct.pack("<HH8B", (type << 12) | (offset & 0xfff), year, month, day, hour, minute, second, *sub)
        image[pos + 84:pos + 96] = stamp
        actual = datetime.datetime(2041 if month == 13 else year, 1 if month == 13 else month, 1,
                                   tzinfo=datetime.timezone.utc) + datetime.timedelta(days=day - 1, hours=hour, minutes=minute, seconds=second)
        if type == 1 and offset != -2047: actual -= datetime.timedelta(minutes=offset)
        attr["seconds"] = int(actual.timestamp())
        attr["nanoseconds"] = 0 if 100 in sub else 123456000
    tag(pos, start + len(ea) + ad_length)
    attributes = {} if kind == "ia-hidden-zero-links" else {name: attr}
    return bytes(image), files, attributes


def volume(kind):
    image, files, _ = build(kind)
    return image, files


def attributes(kind):
    return build(kind)[2]
