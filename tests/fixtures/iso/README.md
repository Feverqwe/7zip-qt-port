`plain.iso.zlib` and `joliet.iso.zlib` contain only generated patterned files,
an empty file, a nested directory and a generated 2048-byte El Torito boot image.
They were created on Ubuntu with genisoimage 1.1.11:

```
genisoimage -quiet -iso-level 3 -V IsoFixture -no-pad \
  -b boot.bin -no-emul-boot -boot-load-size 4 -c boot.cat \
  -o plain.iso seed
```

The Joliet image adds `-J -input-charset utf-8` and a generated Unicode filename.
Input files used modification time 1760000000. No user archives, physical
devices or executable boot code were used. `tests/iso_fixture.py` specifies
expected bytes and derives logical-block, XAR, multi-section and damaged cases.
