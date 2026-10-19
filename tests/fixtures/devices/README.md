# Read-only device fixtures

These gzip-compressed ordinary files were generated on the Linux runner with
e2fsprogs `mke2fs` and util-linux `sfdisk`. They contain only generated test data,
including a Unicode directory/file and the text `device ext4 payload` followed
by a newline. No user disks or archives were used.

Run `bash tests/fixtures/devices/generate.sh OUTPUT_DIRECTORY` on Linux to
regenerate the images. The generator never attaches or mounts an image.
Filesystem timestamps/UUIDs and partition identifiers may differ between runs;
tests compare observable content and partition boundaries, not binary identity.

`ext4.img.gz` contains the filesystem directly. `mbr.img.gz` contains it in a
Linux partition. `gpt.img.gz` also has a second blank Linux-type partition to
verify explicit partition selection and rejection of unsupported content.
