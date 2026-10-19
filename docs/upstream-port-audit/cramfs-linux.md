# cramfs sparse reads checked against Linux

## Source comparison

2026-10-19, part of the [Linux format review](filesystems-linux.md).
Linux reference repository:
<https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>, commit
`58161ead29e50d70a317ea26b472e0c7508e69da`.
Inventory unit: complete `fs/cramfs` (`inode.c`, `uncompress.c`, `internal.h`,
`README`, Makefile and Kconfig), plus `include/uapi/linux/cramfs_fs.h`.
The comparison covered superblock/feature acceptance, directory entry layout
and traversal, inode publication, folio reads, end/direct block pointers,
hole handling and the zlib wrapper. Kernel page cache, mmap/XIP, VFS mounting
and block/MTD acquisition are platform services outside the archive adapter.

7-Zip repository: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`. Complete
`CPP/7zip/Archive/CramfsHandler.cpp` bodies were read together with
`Common/StreamObjects.cpp`'s cached stream consumer. Relevant dependencies are
the original zlib/LZMA decoders, copy coder, progress callbacks and stream helpers.

## Corrected scenario and source transfer

Linux `cramfs_read_folio` treats a zero-length physical block as a hole and
zero-fills the remaining page. A logical file can contain consecutive holes,
including a partial final page. The 7-Zip handler instead passed an empty
extent to its decompressor, reporting a data error for valid sparse files.

The adapted C++ handler is retained under its original relative path in `src/`.
Its change reuses the existing C++ sparse branch from
`SquashfsHandler.cpp::CHandler::ReadBlock`: fill the requested logical block
with zeros and return success when its physical extent is empty. Existing
end-pointer validation runs before this branch. The upstream algorithm,
organization, byte-order handling and decompressor consumers are retained.
Linux C code was compared as the behavior reference; no C body was translated
into C++. No new filesystem algorithm or independent decoder was introduced.
Formatting is confined to the added block; the copied upstream layout is retained.

The baseline records the adapted module as an engine runtime input. Both
codec-runtime builders overlay that authoritative source into an owned, pinned
snapshot, compile the dynamic and standalone engine, and record source hashes
in runtime provenance. The snapshot preserves unchanged files and incremental
outputs. Prepared-runtime caching includes the source and overlay helper hashes.
No sibling checkout, Qt widget or archive transport is changed.

## Remaining comparison findings

This iteration accepts the sparse end-pointer scenario, not complete cramfs
feature parity. Per the user's clarification, these source-visible gaps are
required modernization work, not permanent accepted omissions:

- Linux extended block pointers use superblock bit 11. The 7-Zip handler
  interprets that range as a historical block-size extension and does not
  implement the direct/uncompressed pointer branches. These images need a
  separate integrated read scenario; successful sparse tests do not cover them.
- Linux accepts a superblock at byte 512 with absolute volume offsets. Engine
  signature scanning and the handler's relative buffered offsets need a joined
  review before claiming that prefixed-image scenario.
- Linux rejects a root that is not a directory and empty directory names;
  corresponding acceptance/path handling needs a metadata scenario in the
  archive adapter. Legacy LZMA and enlarged-block extensions are 7-Zip scope,
  with no counterpart in the selected Linux driver.

## Validation

Generated deterministic images exercise holes before, between and after data,
consecutive holes, a partial last block, both byte orders and byte-exact output
through production `7z` and `7zz`. Each extraction uses a Unicode/spaced image
path and verifies source-image preservation. Before the fix, all sparse cases
failed with a data error on macOS. After the fix, all cases passed.

Focused local macOS checks also pass for runtime-source snapshot updates,
preservation of incremental outputs, refusal of unowned directories,
prepared-runtime caching and source baseline inventory. Final focused suites
`cramfs`, `runtime_cache`, `upstream_baseline`, `upstream_inventory`, `exfat`,
`exfat_backend` and `brotli` passed on macOS arm64/Qt 6.12 (1.25 s), Linux x64
and Windows x64/MSVC/Qt 6.12. Runtime build receipts:
`20261019T054856Z-d84ce9f9` (Linux), `20261019T055027Z-df7dc2c5` (Windows).
Final test receipt: `20261019T055131Z-4936a6ae`. Source-baseline verification
against the read-only sibling passed locally. No full, slow, physical-device
or UI acceptance is claimed. Incremental runtime outputs and compiler caches
are retained. Temporary images/extractions were deleted by the test fixtures.
