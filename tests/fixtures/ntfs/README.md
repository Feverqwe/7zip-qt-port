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
