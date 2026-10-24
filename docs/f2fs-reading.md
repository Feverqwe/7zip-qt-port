# Read F2FS images

Sevenzip opens F2FS images for browsing and copying files. Open an image as an
archive, or use [Open device](direct-device-reading.md) for an unmounted flash
drive. The source remains read-only; creating, updating, repairing and replaying
F2FS are unavailable.

The initial reader accepts cleanly unmounted, single-device filesystems with
4 KiB blocks. It reads ordinary files, inline data, sparse files, directories
and symbolic links, including images with extra inode attributes and supported
metadata checksums. Android sparse containers must first be converted to a raw
filesystem image; the reader does not unwrap Android disk encryption.

Encrypted, compressed and fs-verity files are marked unsupported and cannot be
extracted. Encrypted directories are visible, but their contents are unavailable;
readable siblings remain accessible. An extraction that selects unsupported
entries reports errors. Raw fscrypt keys supported by the ext reader do not yet
unlock F2FS files.

Multi-device filesystems, the optimized F2FS read-only layout, large NAT bitmaps, packed SSA, nonstandard block or
segment geometry, unfinished recovery and damaged-image recovery are outside
this initial reader. Do not treat it as a complete replacement for the Linux
F2FS driver or as support for every Android userdata image.

[Source provenance and validation](upstream-port-audit/f2fs.md).
