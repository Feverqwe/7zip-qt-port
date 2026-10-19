`hfsplus.img.zlib` is a zlib-compressed 16 MiB HFS+ volume created with macOS
`hdiutil create -layout NONE -format UDRW -fs HFS+ -size 16m -srcfolder seed
-volname HfsFixture base.dmg`, trimmed to the volume's allocation size.
It contains only generated test data: two patterned files, an empty directory,
one resource fork and one custom inline extended attribute. No user files or
device contents were used. `tests/hfs_fixture.py` defines their expected bytes
and derives the fragmented, preallocated and damaged variants.
