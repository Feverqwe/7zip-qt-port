"""Native fscrypt images with one validated leaf before an invalid sibling."""
import struct
from ext_fixture import Volume
from ext_modern_fixture import crc32c


def encrypted_partial_map(image, case):
    data = bytearray(image)
    bs = 1024 << struct.unpack_from("<I", data, 1024 + 24)[0]
    per_group = struct.unpack_from("<I", data, 1024 + 40)[0]
    inode_size = struct.unpack_from("<H", data, 1024 + 88)[0]
    descriptor_size = struct.unpack_from("<H", data, 1024 + 254)[0]
    group, index = divmod(case["file_inode"] - 1, per_group)
    descriptor = bs + group * descriptor_size
    table = struct.unpack_from("<I", data, descriptor + 8)[0]
    table |= struct.unpack_from("<I", data, descriptor + 40)[0] << 32
    offset = table * bs + index * inode_size
    inode = bytearray(data[offset:offset + inode_size])
    logical, physical, length = case["extents"][0]
    tail_logical, tail_physical, _ = case["extents"][-1]
    assert logical == 0 and length == bs and tail_logical > length
    assert tail_physical != physical
    seed = crc32c(0xFFFFFFFF, data[1024 + 104:1024 + 120])
    seed = crc32c(seed, struct.pack("<I", case["file_inode"]))
    seed = crc32c(seed, inode[100:104])
    # Reuse the file's tail block as a valid first metadata leaf. The second
    # child points to unmodified ciphertext, which is invalid extent metadata.
    # No allocation or unrelated inode is changed by this synthetic mutation.
    leaf = Volume.extent_header(0, [(0, 1, physical // bs)], 4)
    leaf += struct.pack("<I", crc32c(seed, leaf))
    data[tail_physical:tail_physical + len(leaf)] = leaf
    inode[40:100] = Volume.extent_header(
        1, [(0, tail_physical // bs), (tail_logical // bs, physical // bs)], 4)
    inode[124:126] = bytes(2)
    inode[130:132] = bytes(2)
    checksum = crc32c(seed, inode)
    struct.pack_into("<H", inode, 124, checksum & 0xFFFF)
    struct.pack_into("<H", inode, 130, checksum >> 16)
    data[offset:offset + inode_size] = inode
    return bytes(data)
