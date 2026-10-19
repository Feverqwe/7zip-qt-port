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

`rock.iso.zlib` and `rock-joliet.iso.zlib` use the same genisoimage version with
`-R -iso-level 3 -V RockFixture -no-pad`, with `-J -input-charset utf-8` for the
second image. They contain generated Unicode and punctuation names, a 239-byte
name, two symlinks and eleven nested directories, including native CL/RE/PL
relocation and NM/SL continuation records. File modes are 0640 and directory
modes 0750, UID/GID 1000; genisoimage also records native generated metadata.
Modification times were set to 1760000000; native change/access times may differ.
`tests/iso_rock_fixture.py` defines payloads and the focused CE/SL/CL, TF/PN and
primary-root variants. No symlink was followed to copy outside fixture data.
