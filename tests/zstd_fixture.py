"""Zstandard framing plus fixed libzstd 1.5.7 compressed fixtures."""
from pathlib import Path
import json
import struct
import sys
import zlib

CASES = ("raw", "empty", "empty-rle", "zero-blocks", "rle", "unknown-size", "fcs-1", "fcs-2", "fcs-8",
         "dict-zero-1", "dict-zero-2", "dict-zero-4", "unused", "multi-block",
         "compressed", "checksum", "huffman", "concatenated", "empty-first",
         "skip-first", "skip-only", "skip-between", "mixed-history")
BAD = ("magic", "reserved", "block-type", "block-size", "body-short", "header-short",
       "size-small", "size-large", "checksum-bad", "dictionary", "skip-short", "tail",
       "compressed-bad", "late-crc")

# libzstd 1.5.7, compressionLevel=5, checksumFlag=1 only for checksum.
COMPRESSED = {'compressed': '28b52ffda028880400cc0800e410000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff7a737464207061796c6f616400ff01000eeffe11f99a024c0000087a0100fcff3910024d000008f4010024081d0801', 'checksum': '28b52ffda428880400cc0800e410000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff7a737464207061796c6f616400ff01000eeffe11f99a024c0000087a0100fcff3910024d000008f4010024081d0801beb3ff30', 'huffman': '28b52ffd60000f1d42002ae8501d0ab00f208104123a495240d101d101d101fc526f96563a63ec627401de1c9717ccbb6383fc2c88c5b7d9b68521a4cc83312e038d05b3fa0170f14fa22a588b0975b7c2c6ee46fe94dbc3afafe631cd98f5e8e50dd963058b2e982b3ab49f72ce847f5eba37609f21dbabc8e899613e56b8d47a77d5126fbd0a3d8f417e17d385ef86bc96a8f57dad52e7fecf6066a2d2cc37ec9b05f7189291b40701cf48f419641ce85f681ccf52d4b5c20a51f97329d05190aa5a08625d97e0dfeb1f57a073a77a8f6dd01fe035171e85d30c297454a7b68c04811a1b491263c4baa04b53eafb7c22a20bef14a8bf9021bdb7dd5580c4b710ae16b73cd859fda56c00f2fa9397d859a3bbd00e5f8ef44c6fdf32f3062b45436046eb643b302598bc8b5946de7d054a194acd6e7a76901e82c0460d36bdf53cf48ba250939a7da5517c5db5cc60870dac03c5c34b3a178ceee30510323e3d02022fe98150494d02245c0a05ae33d2b73661d3baf2ddefdd4e846ccc5ba081da628eada3943027d6ce685ec5c5ea7b7f50aed4e4cf017d19591d67c0c0c755cdce01fb488f238117e80944945135e75dd136e6be5576c8e9048ac3742ee0eff4bf0c8212ff0d6aabda0193098b878363e458d1c9ebea74533eda94ac825a8cc1f78ecec2e79eee88fccacc48fc041ea1267b8d631f410462fa53b7af1c5bdad459c10ff0603444ac8d2651900cc95399ce6679845f9ab938be41fef855a5975482b4f7bfd655a3e5de1dca03df239ae41d3904e4870031c7d5bbb5df51b3efb9d56ea0b48006365c7e1bb56c0abe3df7ebf5b069aa4743d23bb1961502ae709be8d05c43820aa01af5fb941d4dd1deab618a6a0aefafc44afb703b9b012b2ce9f6ef98aa70ef5b194087952ee86dc2ff8f66433c8da1626b2bcbaebb9799ed5062f3f55eb55220be99ab567f7bc9d19c680c65cf7f396435cfc4e29232bfad4e9fefb83f531dfa09866ef5cd5cdd6710005d937528d16f9f52743c57844f9b2ddce9ee2b3d2e9a5c32c74d664b013ceaf5b5d27d720792a6df5cc0443c7aaa646452d05005bd96e2f6b4546fc07fa5c1d38e16b2d1a717b9044fa825a4484c595059f5c436beb62589a0b15aca51f6a81d0915eed6e18d6478cde716455a2cb0a00761d758a2a6fadaa3a7959028cb75b431055fe667908d0ac1b1767713f650457766b6c11bf017c8c83a7dcb87e1ee961e19538bbbecd8c9725a92a125460f1da109dffb9b71523509ad2091ffe2c893dc64281c5f2d3c86dce5c74a10d78218a08e6f4db3b4813e1c9cf27ae9bc522b1a016431e89735bc5755be168d08146db68bfc729b12c5556737847e8d96fbcb2e46d2bca92d57e35c35960dda822f871755e7c92c668db984c9624a6aaff2701d6f894065afbc39960ad8402ce8aef1262df9f4bdb021ce9e75d332cddea4315e1818a2d913db888e7fff316a3e831e44a630dba34e454e75644dda87e5c780aa5db29e8fd00f92c483d9e581a963526559d78b454f979af70e77223b81fc16297253b22d1a7f6ef203f0d367f2bf6c20929223b474aecd3640591476d452f336761550cc13bcb7c59f07b79823397a6daec8844d4014143d7cec1f38647f75007445136ec6ae3e9d7da1b3774ed99f23f602ab91c9c79c6ce852dd711943d6e3d666958222e852c2ea400e62d4d63dbb4b7f767c250817a55203c671a0676b2fcf6a52aab2670f3ed12252416213571d9d0418fec6efb3f44bcfd3806dd1716beda1cb22a817120c0fd9dd17896be3794f253f806a11f59d26430606b55f88b6545c4b5ff6102e5e28733291ca74392f1902a0714322dbf3c64e7dc00b9fdd83a120c5328993ff35cdb1446ba4a1b75aa8ecaf9356027066885e913ae4a30220ca72c64d24c89aefab4cf5216bf1252b227f847bd74f3fb6d81c49135f5e3d8a43259d0caf24ad29cfbc4db30a3b8c5363cb8a4ffd149a838663c13f7b8615cf8f4157024f8a09230662420521c23c7d951b15fb8ce94c81d1fc1afe7956858ec62633941be032eb04339e64a647724407b150a8d29b371a1db0dd42c67bda2f9673aa8cca787bc58abcfe3493069a563862f9a3c3d66e2fa95b3227ae09354ddebae11da7951c437a796ed7f43f34bf2fe616a9ab091c64d945fbfbc5c2e03c482a66aa8bd79fad21369c14d9ac5c7167c2f23a4c79e8b75c8d520dca28f3af85edc53ffc071a364317540d9698c112ad196c428b1e250565121247e145a750ef898567e718df80515ff8733c404810a78400b54b349ff1d346c7f1e9ef729ca582f11f90ff0f11f7a3f66dbfb4e47cfd867d9d4b5e4e9b498a68f580447d0bea3b92a2c0ae634b863a944c2aa1cb291ac9fe2b57693e65980a32614ea6a3ce457517f3acde1b9ae587d4b01e7aebf25e2f46d0796649be8a5a7a692c59aba4fb961778eaa45373577449fc4559cd72bed01620c09b3eada9560863d874c84aa95dda00463d09f35b2a3800cb375a3c5092deb95e3e36b0861f6f101bd697466bb0c037982cea69eaf1a01fa41c3786351acb475713e70a6aae877420b71ff96a3fcaf8401752fe512370319be9a2c6863f6c39857b6e948a16660777a584ee179e3d5d5ae6607f127d97b9cb451d5f281014012146497a10f063d4e1301a0e20a1c97db46c0a6ad88e93a74236b212268559568fa5582b5e964279314ac8471f8bbad52936b98d5ccc9eccda736304ad1d6924ac29683326dac4106b54195602d6808ca9655d123d22b7ccd7ad0a45f94d9d465e00c5b58843a32ceb47bc5bee506637f6a381f27e328cb1623857f22640f8d181c40b9c8e1513142f104560bef4ba1f0880aa81209ca390459fd05c4de77d0c37159d9a63d111b6fec7d8b6e3f10a8d6c1861f83ffd4c65c1a19bd1148519b0dc14c37139dfc3f220079c4305e3138c6706cfb165c0e092a4ed6e51371988a25310639f8233'}


def payload(kind):
    if kind == "huffman":
        state, result = 19, bytearray()
        for _ in range(4096):
            state = (state * 1664525 + 1013904223) & 0xffffffff
            result.append(65 + ((state >> 24) & 15))
        return bytes(result)
    return (bytes(range(256)) + b"zstd payload\0\xff") * 1100


def raw(data, *, fcs=4, dictionary=0, unused=False, rle=False):
    flag = {0: 0, 1: 0, 2: 1, 4: 2, 8: 3}[fcs]
    descriptor = (flag << 6) | (0x20 if fcs else 0) | (0x10 if unused else 0)
    descriptor |= {0: 0, 1: 1, 2: 2, 4: 3}[dictionary]
    header = b"\x28\xb5\x2f\xfd" + bytes((descriptor,))
    if not fcs:
        header += b"\x38"  # 128 KiB window, unknown content size.
    header += bytes(dictionary)
    if fcs:
        header += (len(data) - (256 if fcs == 2 else 0)).to_bytes(fcs, "little")
    blocks = []
    for position in range(0, max(1, len(data)), 131072):
        chunk = data[position:position + 131072]
        last = position + 131072 >= len(data)
        value = len(chunk) << 3 | (2 if rle else 0) | last
        blocks.append(value.to_bytes(3, "little") + ((chunk[:1] or b"\0") if rle else chunk))
    return header + b"".join(blocks)


def skip(data, number=0):
    return struct.pack("<II", 0x184d2a50 + number, len(data)) + data


def volume(kind):
    expected = b"Raw Zstandard payload\0\xff" * 23
    image = raw(expected)
    if kind in ("compressed", "checksum", "huffman"):
        expected = payload(kind)
        image = bytes.fromhex(COMPRESSED[kind])
    elif kind == "empty":
        expected, image = b"", raw(b"")
    elif kind == "empty-rle":
        expected, image = b"", raw(b"", rle=True)
    elif kind == "zero-blocks":
        image = image[:9] + bytes(3) + (len(expected) << 3).to_bytes(3, "little") + expected + b"\1\0\0"
    elif kind == "rle":
        expected = b"A" * 300000
        image = raw(expected, rle=True)
    elif kind == "multi-block":
        expected = bytes(range(256)) * 1700
        image = raw(expected, fcs=0)
    elif kind == "mixed-history":
        # Mix raw/RLE and compressed history across cyclic-window wraps.
        compressed = bytes.fromhex(COMPRESSED["compressed"])
        blocks = compressed[9:]
        position = 0
        while position < len(blocks):
            header = int.from_bytes(blocks[position:position + 3], "little")
            if header & 1:
                blocks = blocks[:position] + (header & ~1).to_bytes(3, "little") + blocks[position + 3:]
            position += 3 + (1 if (header >> 1) & 3 == 1 else header >> 3)
        prefix = bytes(range(256)) * 17
        raw_block = (len(prefix) << 3).to_bytes(3, "little") + prefix
        rle_block = ((7000 << 3) | 2).to_bytes(3, "little") + b"A"
        image = b"\x28\xb5\x2f\xfd\x00\x38" + (raw_block + rle_block + blocks) * 9 + b"\x01\x00\x00"
        expected = (prefix + b"A" * 7000 + payload("compressed")) * 9
    elif kind == "unknown-size":
        image = raw(expected, fcs=0)
    elif kind.startswith("fcs-"):
        width = int(kind[4:])
        if width == 1:
            expected = expected[:53]
        image = raw(expected, fcs=width)
    elif kind.startswith("dict-zero-"):
        image = raw(expected, dictionary=int(kind[10:]))
    elif kind == "unused":
        image = raw(expected, unused=True)
    elif kind == "concatenated":
        image += raw(b"second frame")
        expected += b"second frame"
    elif kind == "empty-first":
        image = raw(b"") + image
    elif kind == "skip-first":
        image = b"".join(skip(bytes((i,)) * (i + 1), i) for i in range(16)) + image
    elif kind == "skip-only":
        expected, image = b"", skip(b"ignored bytes", 15) + skip(b"")
    elif kind == "skip-between":
        image += skip(b"\x28\xb5\x2f\xfdnot a real frame", 7) + raw(b"second") + skip(b"")
        expected += b"second"
    elif kind in ("magic", "reserved", "block-type"):
        offset = {"magic": 0, "reserved": 4, "block-type": 9}[kind]
        value = {"magic": 1, "reserved": 8, "block-type": 6}[kind]
        image = image[:offset] + bytes((image[offset] | value,)) + image[offset + 1:]
    elif kind == "block-size":
        image = image[:9] + ((131073 << 3) | 1).to_bytes(3, "little") + image[12:]
    elif kind == "body-short":
        image = image[:-1]
    elif kind == "header-short":
        image = image[:7]
    elif kind in ("size-small", "size-large"):
        size = len(expected) + (1 if kind == "size-large" else -1)
        image = image[:5] + struct.pack("<I", size) + image[9:]
    elif kind in ("checksum-bad", "compressed-bad", "late-crc"):
        bad = bytearray.fromhex(COMPRESSED["checksum"])
        bad[-1 if kind != "compressed-bad" else 15] ^= 1
        image = (image if kind == "late-crc" else b"") + bad
    elif kind == "dictionary":
        image = raw(expected, dictionary=1)
        image = image[:5] + b"\x01" + image[6:]
    elif kind == "skip-short":
        image = skip(b"abc")[:-1]
    elif kind == "tail":
        image += b"unexpected tail"
    elif kind == "size-overflow":
        frame = raw(b"", fcs=8)
        frame = frame[:5] + (1 << 63).to_bytes(8, "little") + frame[13:]
        image = frame + frame
    elif kind == "zip-zstd":
        image = bytes.fromhex(COMPRESSED["checksum"])
        expected = payload("checksum")
        name = "данные 空.txt".encode("utf-8")
        checksum = zlib.crc32(expected)
        local = struct.pack("<IHHHHHIIIHH", 0x04034b50, 63, 0x800, 93, 0, 0,
                            checksum, len(image), len(expected), len(name), 0) + name + image
        central = struct.pack("<IHHHHHHIIIHHHHHII", 0x02014b50, 63, 63, 0x800, 93, 0, 0,
                              checksum, len(image), len(expected), len(name), 0, 0, 0, 0, 0, 0) + name
        image = local + central + struct.pack("<IHHHHIIH", 0x06054b50, 0, 0, 1, 1,
                                             len(central), len(local), 0)
    return image, expected


if __name__ == "__main__":
    image, expected = volume(sys.argv[1])
    Path(sys.argv[2]).write_bytes(image)
    print(json.dumps({"payload": expected.hex()}))
