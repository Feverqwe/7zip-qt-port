"""Deterministic contents shared by EROFS creator fixtures and behavior tests."""
import hashlib
import struct

PAYLOAD_NAME = "данные 空.bin"
PAYLOAD = (b"EROFS repeated cluster\x00" * 6000 +
           b"".join(hashlib.sha256(str(i).encode()).digest() for i in range(290)) + b"tail")
FILES = {PAYLOAD_NAME: PAYLOAD, "duplicate.bin": PAYLOAD, "папка/tiny": b"inline tail",
         "empty": b"", "sparse.bin": b"A" * 4096 + bytes(2 * 65536) + b"tail"}


def mutation_locations(data):
    """Locate the root and payload in plain/full-index creator fixtures."""
    block_size = 1 << data[1036]
    metadata = struct.unpack_from("<I", data, 1064)[0] * block_size
    root_nid = struct.unpack_from("<H", data, 1038)[0]
    root = metadata + root_nid * 32
    fmt, xcount = struct.unpack_from("<HH", data, root)
    inode_size = 64 if fmt & 1 else 32
    xattrs = 12 + (xcount - 1) * 4 if xcount else 0
    directory_size = struct.unpack_from("<Q" if fmt & 1 else "<I", data, root + 8)[0]
    directory = (root + inode_size + xattrs if (fmt >> 1) & 7 == 2 else
                 struct.unpack_from("<I", data, root + 16)[0] * block_size)
    first_name = struct.unpack_from("<H", data, directory + 8)[0]
    entries = first_name // 12
    for i in range(entries):
        entry = directory + i * 12
        start = struct.unpack_from("<H", data, entry + 8)[0]
        end = (struct.unpack_from("<H", data, entry + 20)[0] if i + 1 < entries else
               directory_size)
        if data[directory + start:directory + end].rstrip(b"\0") == PAYLOAD_NAME.encode():
            nid = struct.unpack_from("<Q", data, entry)[0]
            return {"root": root, "entry": entry, "name": directory + start,
                    "file": metadata + nid * 32, "block_size": block_size, "root_nid": root_nid}
    raise ValueError("Payload inode not found in fixture directory")
