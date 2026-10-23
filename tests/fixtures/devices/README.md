# Read-only device fixtures

These gzip-compressed ordinary files were generated on the Linux runner with
e2fsprogs `mke2fs` and util-linux `sfdisk`. They contain only generated test data,
including a Unicode directory/file and the text `device ext4 payload` followed
by a newline. No user disks or archives were used.

Run `bash tests/fixtures/devices/generate.sh OUTPUT_DIRECTORY` on Linux to
regenerate the images. The generator never attaches or mounts an image.
Filesystem timestamps/UUIDs and partition identifiers may differ between runs;
tests compare observable content and partition boundaries, not binary identity.

`ext2.img.gz`, `ext3.img.gz` and `ext4.img.gz` contain the filesystem directly.
The ext3 sample has a clean journal. `mbr.img.gz` contains ext4 in a
Linux partition. `gpt.img.gz` also has a second blank Linux-type partition to
verify explicit partition selection and rejection of unsupported content.

`ext4-timestamp.img.gz` contains a small gzip SQL sample under
`.internal/twiMonBot/2024-05-07T02:00:02Z.sql.gz`. It reproduces copying a
filename with colons through native device extraction and the panel commands.

`apfs.img.zlib` is a 32 MiB raw APFS container generated on macOS with
`hdiutil create -layout NONE -format UDRW -fs APFS -size 32m -srcfolder seed
-volname DeviceFixture apfs.dmg`, then zlib-compressed. Its generated inputs
are `данные 空.txt` (`APFS generated device payload` plus newline) and
`nested/file with spaces.bin` (sixteen repetitions of bytes 0…255). No user
files or physical media were used. UUIDs and timestamps may vary on regeneration.
