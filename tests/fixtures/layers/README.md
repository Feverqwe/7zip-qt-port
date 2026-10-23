# Streamed container chain

These generated QCOW2 v3 images contain the existing disposable
[`devices/mbr.img.gz`](../devices/README.md) disk: MBR → ext4 →
`папка 空/данные с пробелом.txt`. No real device or user data is used.

The virtual disk is 1 GiB, mostly unallocated or explicit zero clusters.
`stored.qcow2.gz` contains stored clusters; `deflate.qcow2.gz` also contains
independently Deflate-compressed clusters. `zstd.qcow2.gz` is produced by
`qemu-img` with Zstandard compression. The outer gzip only keeps the fixture
small in the repository; tests decompress it before opening QCOW2.
`manifest.json` records the raw source/image sizes, SHA-256 hashes and expected
selected file bytes. The encoder follows the
[QEMU QCOW2 specification](https://www.qemu.org/docs/master/interop/qcow2.html).
It is test data generation, not a product decoder. The Zstandard fixture is
validated by `qemu-img check` and byte comparison with its raw source; no guest
OS is booted.

Regenerate with Python 3 and `qemu-img` available on PATH:

```sh
python3 scripts/generate-qcow-fixtures.py tests/fixtures/layers
```

Nothing is mounted or attached. The QCOW regression compares the readable disk
prefix with the raw source, checks backward and sparse-tail reads, and rejects
unsupported layouts and damaged Zstandard data through the official handler.
