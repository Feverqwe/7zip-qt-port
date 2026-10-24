# Shared archive readers for the application and console

User request, 2026-10-24: integrate the Linux/GPL readers with the shipped `7z`
and remove parallel SDK reader implementations. This supersedes the separate
source-variant policy introduced on 2026-10-22. The historical split started at
repository commit `17b72715e8385fd13d29b1eb4e969e4ce6a49030`; its original
conditional implementations and validation remain available in Git history.

## Source ownership

Each complete reader has one owner under `src/gpl/CPP/`, preserving its upstream
relative path: ext, FAT, NTFS, HFS, SquashFS, ISO, UDF, MBR and composite CPIO.
The SDK copies of fifteen source/header units are removed. SquashFS already had
one owner. CramFS was retired separately and is outside this change.

The original 7-Zip inventory remains pinned in the [baseline](../upstream-baseline.json)
to <https://github.com/ip7z/7zip.git>, full revision
`b88c6bc09339de32f7d18bbab900686b244f42ba`. Linux references, transferred
dependencies, omitted kernel/write scope, licenses and feature limitations remain
in the owning [filesystem audit](filesystems-linux.md) and its component links.
This change imports no new Linux algorithms and advances no reference baseline.

Four portable dependencies formerly embedded in the duplicated handlers are
extracted into shared units under `src/CPP/7zip/Archive`:

| Original owner | Transferred scope | Shared owners and consumers |
| --- | --- | --- |
| `ExtHandler.cpp` | `NExt::CHeader`, geometry parsing, `IsArc_Ext`, `IsArc_Ext_PhySize`, supporting constants | `ExtHeader.h/.cpp`; ext module, image/container probes and LP |
| `FatHandler.cpp` | `NFat::CHeader`, geometry parsing and portable `IsArc_Fat` | `FatHeader.h/.cpp`; FAT, MBR and GPT probes |
| `HfsHandler.cpp` | `NHfs::CCompressHeader`, `MethodsMaskToProp`, `CDecoder` and supporting constants | `HfsDecmpfs.cpp`, shared `HfsHandler.h`; HFS module and APFS |
| `MbrHandler.cpp` | `NMbr::GetFileSystem` and its signature helpers | `MbrProbe.cpp`; MBR module and GPT |

The baseline's `extracted` map records the symbols and local owners against
SHA256 hashes of the complete original handler units. `verify` checks those
original bytes; `review` identifies the extracted owners when an original handler
changes. Use the existing [update procedure](../upstream-update.md), comparing
the proposed original unit with both the complete reader and these support units.
Local refactoring does not change the original hashes.

The portable FAT probe retains the boot-signature requirement used by partition
discovery. The full FAT reader's registration uses its existing signature-free
probe and Linux legacy-header fallback, sharing the same geometry parser.
No second FAT reader or new format-selection policy is introduced.
HFS's public support header explicitly includes its archive callback dependency;
the shared compression block size stays with that declaration.
The shared ext geometry parser retains the complete reader's recovery tolerance
for inconsistent free-inode/block counters. Core image probes now recognize the
same volumes; the former SDK-only free-counter refusal is not retained. Bounds
and geometry checks continue to use the existing canonical parser.

## Build and runtime

The official external-codec console `7z` and the application's native engine
discover the same deployed `Formats` modules through upstream `LoadCodecs`.
There is no console invocation from 7zFM/7zG. The core Format7zF source lists no
longer compile the replaced full readers or their duplicate registrations.
`runtime_format_sources` selects the extracted support objects for Format7zF;
the console executable and original SFX do not link these objects. Runtime
fingerprints and provenance include their sources and headers. Incremental
build trees, compiler caches and unchanged source timestamps are retained.

Complete GPL readers remain separate binary modules. GPLv2-only Linux code,
GPLv3 GRUB readers and the unRAR-bearing core are not combined into one binary.
Original notices and the existing component license files remain unchanged.
The runtime preparer rejects full GPL readers in the core overlay. Source
uniqueness now applies across both source roots, preventing renewed SDK copies.

## Validation

Source consolidation is complete for the selected owners. Eight complete moved
sections were compared with their pre-change bodies, accounting separately for
formatting; the former SDK and canonical HFS compression/MBR probe bodies also
match. Full reader algorithms and dependencies remain connected through their
existing modules. The baseline inventory/hash verification and update review
passed without advancing the reference revision.

Validation, 2026-10-24, `feature/unified-gpl-readers` based on `4f3b80b4`:

| Platform | Checked scope and result | Remaining acceptance limits |
| --- | --- | --- |
| macOS arm64 | Runtime/SFX, modules, application and private install built. Focused batch 24/26; separate shared-support 11/11, APFS/device/tooling consumers 5/5 and tooling 2/2 passed. Independent native/console FAT rows passed. Installed console legacy-FAT list/test/extract, Unicode/spaces 7z create/test/extract and native Cocoa startup passed. | `iso_stream` fails opening the Joliet emoji name; `udf_stream` has seven trailing-data failures and one Unicode-name failure, matching previously recorded scenarios. Full native ISO/UDF acceptance is not established. |
| Linux x64 | Runtime/SFX and selected targets built. Batch 25/28; followup 8/10, including successful `linux_frames_stream` retry. Final independent FAT rows passed. All selected CLI format suites and APFS/device consumer passed. | `udf_stream` and `iso_stream` retained their 45/60-second timeouts, including discovery delays; ISO also reached the known emoji-name failure. |
| Windows x64/MSVC | Runtime/SFX and selected targets built. Batch 22/28; followup 4/5 passed, including independent FAT rows, frames, NTFS streams and provenance. Selected remaining CLI/module/APFS/device consumers passed. Three existing ext CLI scenarios for recovery-state access, bigalloc and reserved journal blocks passed separately. | Full `ext` CLI suite still exceeds its 45-second limit. `udf_stream` reports seven trailing-data failures. The first batch's FAT/frames discovery and NTFS timeouts cleared on retry; the initial batch is not reported as fully green. |

Preparation fixes are covered by compiler-backed regressions: shared support
links/rebuilds only in Format7zF, and original-only SFX snapshots exclude it.
MSVC required `StdAfx.h` before the extracted units' relative includes; the new
snapshot test compares original Git bytes rather than assuming LF fixtures on
Windows. Both corrections passed their affected checks. The independent console
FAT row runs even when the native discovery row fails.

Runner receipts under the harness `build/runners/hyperv/`: Linux
`20261024T181838Z-1fb1034c`, `20261024T182945Z-088ed443`,
`20261024T184022Z-9c210c21`; Windows `20261024T182235Z-ef973f02`,
`20261024T184619Z-de3e2f35`, `20261024T185326Z-e4e0ed05`.
The first failed preparation receipt `20261024T181627Z-3cec32a9` is retained.
Local source comparisons and validation evidence are under the task worktree's
`build/review/unified-readers`.

Incremental outputs and compiler caches were retained. Disposable local/Linux
fixtures were cleaned; current Windows timeout evidence was retained. No full
gate, slow suite, physical-device test, Linux/Windows private-install check or UI
appearance acceptance was run. These runtime limits do not imply missing source
transfer, and source consolidation does not establish full runtime acceptance.
