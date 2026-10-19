NTFS test image created on Ubuntu 24.04.5 with ntfs-3g 2022.10.3:

```
truncate -s 16M base.ntfs
mkntfs -F -Q -s 512 -c 4096 base.ntfs
ntfscp -f base.ntfs alpha /alpha
ntfscp -f base.ntfs beta '/данные 空'
ntfscp -f base.ntfs gamma /gamma
```

`alpha` contains 512 repetitions of bytes 0…255, `beta` contains
`NTFS resident payload`, and `gamma` contains 65536 `B` bytes.
The image is stored with zlib compression. Tests mutate copies to exercise
sparse runs, compression, initialized data, boot recovery, sector geometry,
nonresident reparse data and malformed inputs. The creator image is disposable
test data and contains no user files.

`wof-frames.json` contains independent compressor output from Ubuntu's official
`libwim` 1.14.4-1.1build2, compression level 50. The producer uses
`wimlib_create_compressor` with XPRESS and block sizes 4096/8192/16384, or LZX
with block size 32768, then `wimlib_compress` for each frame (stored bytes when
compression cannot reduce its size). Inputs contain repeated bytes,
deterministic SHA-256 data, x86 CALL
bytes for E8 translation, and a short Unicode tail. Each original frame has a
SHA-256 in the JSON. Tests do not need libwim at runtime.

The MFT fixtures move the final MFT extents and describe them through resident
or nonresident attribute lists. WOF fixtures include both ADS storage kinds,
stored/compressed frames, initialized tails and malformed inputs. A larger
logical WOF file tests 64-bit offsets with selective reads; tests never extract
its full logical size.
