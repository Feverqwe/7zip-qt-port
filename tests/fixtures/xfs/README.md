# XFS read-only samples

`v4.zlib` and `v5.zlib` are zlib-compressed XFS filesystem images created
with xfsprogs 6.6.0 `mkfs.xfs` on Linux. They contain only generated test data:
UTF-8 file names/content, an empty file, a nested directory, an inline symlink
and a directory large enough to use external directory blocks. A 200-byte
link uses remote storage in v4 and inline storage in v5. xfsprogs 6.6's
prototype writer emits raw bytes without the required XSLM header for a v5
remote link; those malformed samples are not used as a successful reference.
The v5 image
enables metadata CRCs and bigtime. No filesystem mount or real user data is used.

Reproduce with the checked-in generator on a Linux machine with xfsprogs:

```sh
python3 tests/fixtures/xfs/generate.py /tmp/xfs-fixtures
```

Image UUIDs/timestamps may differ; tests check observable file data and paths.
`unpack.py` expands these samples to disposable sparse regular files on all
test platforms. Tests additionally modify copies to exercise malformed inputs,
unwritten/distant extents and a bmap leaf; they do not modify the checked-in
samples. The fixtures are test data, not an application runtime dependency.
