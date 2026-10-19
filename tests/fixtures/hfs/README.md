`hfsplus.img.zlib` is a zlib-compressed 16 MiB HFS+ volume created with macOS
`hdiutil create -layout NONE -format UDRW -fs HFS+ -size 16m -srcfolder seed
-volname HfsFixture base.dmg`, trimmed to the volume's allocation size.
It contains only generated test data: two patterned files, an empty directory,
one resource fork and one custom inline extended attribute. No user files or
device contents were used. `tests/hfs_fixture.py` defines their expected bytes
and derives the fragmented, preallocated and damaged variants.

`hfsplus-modern.img.zlib` starts from that volume, attached as `CRawDiskImage`
in a disposable mount point with macOS `hdiutil`. Native `link`, `symlink` and
file APIs created three names for `alpha`, composed Latin/Hangul names, an emoji,
a colon name and a symlink to `alpha`; the HFS+ driver wrote their decomposed
disk names and backing inode. The image was detached before compression.
The volume also contains native generated filesystem metadata. No user files
were copied into it. `tests/hfs_modern_fixture.py` defines the expected payloads
and the focused Unicode, link-guard, metadata and corrupt variants.

`alpha.lzvn` contains only the generated `bytes(range(256)) * 8` payload,
compressed by macOS `compression_encode_buffer(..., COMPRESSION_LZFSE)`.
The native encoder emitted one `bvxn` block; its 12-byte header and final
`bvx$` marker were removed to retain the raw LZVN payload used by decmpfs.
The derived compressed-file cases protect the original zlib/LZVN read paths
when loading the handler from the separate format module.
