# RAR regression fixture

`test_read_format_rar5_compressed.rar.uu` is retained exactly from
[libarchive v3.8.1, commit9525f90ca4bd14c7b335e2f8c84a4607b0af6bdf](https://github.com/libarchive/libarchive/blob/9525f90ca4bd14c7b335e2f8c84a4607b0af6bdf/libarchive/test/test_read_format_rar5_compressed.rar.uu).
The `.rar` is its uu-decoded content. Original distribution notices are preserved
in `libarchive-COPYING.txt`; this is test data, not an application dependency.

SHA-256 of `.uu`:7a87af5035430d2307f9a183b2da8a71c95b3c0a5b8e8d1f172ef3343d4b8620.
The source regression test is `libarchive/test/test_read_format_rar5.c`
at the same revision: compressed `test.bin`,1200 bytes, CRC32=7CCA70CD.
SHA-256 of decoded `.rar`:68f9d7e2662cd5bc5607ba9c6bf1edbf121cf3b08011a7d8643091791750a7c8.
Our test verifies source-handler extraction against the recorded size/CRC and,
where `bsdtar` is installed, independent libarchive extraction. The installed
official7zz26.03 reports Unsupported Method for this compressed fixture.
It complements the locally
generated stored RAR5/RAR7 memory fixtures; stored data alone does not validate
the copied compressed decoder body. These fixtures contain no user archive data.
