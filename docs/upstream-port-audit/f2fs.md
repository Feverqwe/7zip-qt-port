# Read-only F2FS — 2026-10-24

Worktree `.worktrees/f2fs-readonly`, branch `feature/f2fs-readonly`, starts at
`12047bab`. User scope: a separate feature branch for reading Android/flash
F2FS images. [User-facing limits](../f2fs-reading.md) describe the initial reader.

## Source comparison and choice

Linux implements F2FS in `fs/f2fs`, including encryption, compression, checkpoint
recovery and VFS services. The existing Linux reference at
`58161ead29e50d70a317ea26b472e0c7508e69da` was inspected for the inode and inline
read paths (`inode.c`, `inline.c`, `f2fs.h`). It is a reference, not a claim that
the full kernel read side has been transferred. No Linux code is imported here.

The selected portable base is GRUB's existing read-only implementation, written
by Jaegeuk Kim. This avoids introducing a second filesystem parser or importing
the kernel's mount, writeback, page cache and VFS infrastructure. Before editing,
the complete F2FS module, its fshelp caller/reader, disk/error/types/safemath
dependencies and all exported GRUB entry points were compared. The complete
f2fs-tools `fsck/dump.c` read traversal and its mount/header/lib dependencies were
also inspected; its host extraction and mutable global configuration are not
used as the application's archive backend.

| Source | Baseline and local ownership |
| --- | --- |
| [GRUB](https://git.savannah.gnu.org/git/grub.git), `b07cc37ca64fe99260884e7325abc333374e098b` | [Manifest](../f2fs-grub-baseline.json): full original `grub-core/fs/f2fs.c`, `grub-core/fs/fshelp.c`, headers and COPYING hashes. Selected disk structures, superblock/checkpoint CRC and selection, NAT journal/table, node-path mapping, file/inline and directory readers live in `src/fs/f2fs/f2fs.c`; the fshelp transfer is `grub_fshelp_read_file`. |
| [f2fs-tools](https://git.kernel.org/pub/scm/linux/kernel/git/jaegeuk/f2fs-tools.git), `83fd39a184df96db5def895ddc55bf68e720ef76` | [Manifest](../f2fs-tools-baseline.json): complete original header/library/COPYING hashes. Selected modern inode/inline geometry helpers, disk fields/flags and `f2fs_inode_chksum` adapt into the same C owner. |

The C implementation preserves GRUB's module organization, names, checkpoint/NAT
algorithms, direct/indirect/double-indirect node traversal, directory traversal
and file reader. It remains compiled as C11. The compatibility header replaces
GRUB allocation, errors, endian primitives and disk reads with a bounded native
stream callback. Error state is thread-local; the C++ transport serializes access
to shared input and preserves HRESULT failures and cancellation. Every file
stream retains an independent node and the shared volume/input lifetime.

Adaptations also cover modern extra inode attributes and flexible inline xattr
geometry, inode/superblock checksums, the actual selected checkpoint address
(rather than inferring its location from version parity), and the correct
24-byte node footer. The original GRUB node declaration reserves 40 footer bytes;
the port makes the full disk node exactly 4096 bytes. Source hardening checks
volume/table/node spans, node IDs, bitmap/name spans, embedded NULs, unsafe path
components, directory cycles, size arithmetic and cancellation while traversing
holes. NEW_ADDR is zero-filled; unreadable data is never reported as verified
successful extraction.

Omitted GRUB scope: bootloader module registration, path lookup/symlink following,
read hooks, label/UUID conversion and GRUB open/close wrappers. The archive
adapter already owns traversal, names, streams and presentation. The disk source
has no write callback. No fsck, sload, mkfs, console client, mount or Linux driver
is invoked by the application. Native extraction uses the existing engine's
callbacks and path safeguards.

This is a complete transfer of the selected GRUB reading scenario, with the
listed narrow adaptations; it is **not full Linux F2FS read-feature parity**.
Compression, fscrypt, verity verification, xattr/ACL extraction, alternate block
sizes, optimized read-only layout/active-log counts, large NAT checkpoint layouts, multiple devices and recovery/replay are
not implemented. Flagged files cannot return ciphertext, compressed bytes or
unverified verity data as decoded content. Flagged directories stay visible
without descending into unavailable names. Their extraction reports unsupported
method. Unknown layouts and non-clean checkpoints are rejected rather than
misinterpreted. No new recovery/partial-access mode is introduced for F2FS.
Volume label/UUID, timestamp nanoseconds, ACLs and project/quota metadata are not
exposed by this initial archive presentation. Ordinary Unix ownership/mode and
available timestamps are exposed; inode change time is not substituted for an
absent creation time.

## Integration and licensing

`sevenzip_f2fs` is a separate archive-only module registered as `F2FS`, with
signature at byte 1024 and extensions `f2fs img`. Deployment and installation
include it on macOS, Linux and Windows. The native catalogue discovers it through
the existing plugin ABI. `ArchiveBackend`/`ArchiveNativeOpen` remain the owners
of operations; direct-device discovery adds F2FS to its filesystem candidates.
There is no update interface. Presentation glue is in `f2fshandler.cpp` and the
transport/lifetime boundary in `f2fsarchivereader.*`; no new UI is needed.

GRUB is GPL-3.0-or-later. f2fs-tools' header and library are used under their
explicit LGPL-2.1 option, as recorded in its COPYING; their Samsung/Google
notices are retained alongside the transferred helpers. The combined format
module is GPL-3.0-or-later. Shared 7-Zip ABI/stream support retains LGPL notices;
the existing GPL-2.0-or-later module entry point is compatible with this module.
No GPL-2.0-only filesystem/crypto code or unRAR decoder is linked into it.
The installed notices include GRUB COPYING, f2fs-tools COPYING and 7-Zip copying.

## Reproduce provenance and review an update

Keep source checkouts outside the project, containing the recorded commits.
Reuse the existing verifier; it pins original bytes, not the adaptation:

```sh
git clone https://git.savannah.gnu.org/git/grub.git /tmp/f2fs-grub-review
git clone https://git.kernel.org/pub/scm/linux/kernel/git/jaegeuk/f2fs-tools.git /tmp/f2fs-tools-review
python3 scripts/upstream_baseline.py reference-verify --manifest docs/f2fs-grub-baseline.json --upstream /tmp/f2fs-grub-review
python3 scripts/upstream_baseline.py reference-verify --manifest docs/f2fs-tools-baseline.json --upstream /tmp/f2fs-tools-review
python3 scripts/upstream_baseline.py reference-review --manifest docs/f2fs-grub-baseline.json --upstream /tmp/f2fs-grub-review --new-revision <full-commit> --output /tmp/f2fs-grub-update.md
python3 scripts/upstream_baseline.py reference-review --manifest docs/f2fs-tools-baseline.json --upstream /tmp/f2fs-tools-review --new-revision <full-commit> --output /tmp/f2fs-tools-update.md
```

Both review reports show complete original-unit revision changes and each local
adaptation against the baseline. Do not overwrite the partial reader with full
bootloader/tools modules. Review omitted scope and dependencies, retain local
bounds/lifetime fixes, validate affected behavior, then update provenance.

## Fixtures and runtime evidence

`scripts/generate-f2fs-fixtures.py` uses official mkfs/sload without mounting.
Synthetic fixtures include Unicode/spaces, nested inline data, an empty file,
a symlink and a 12 MiB file reaching indirect nodes. The manifest records the
producer, image hashes and expected payload hash. The checked fixtures were
created by Ubuntu f2fs-tools 1.16.0; both completed its built-in fsck checks.
They test baseline and extra_attr/inode_checksum/sb_checksum layouts.

The 1.16.0 producer's flexible_inline_xattr variant produced inconsistent inode
geometry and failed its own fsck. That attempt is excluded; passing runtime
coverage for flexible inline geometry is not claimed. Selected modern helpers
come from the newer source revision above, independently of fixture producer.

Focused suites are `f2fs_module` (public archive ABI, byte-exact extraction,
metadata/streams, holes, corruption, paths, unsupported data, failure/cancel) and
`f2fs_native` (production catalogue discovery, image autodetection, bounded 4Kn
device adapter and copying). Source completeness and runtime evidence are
separate. Physical devices, Android userdata encryption, recovery, full/release
and slow suites are not covered by these synthetic checks.

Final source re-comparison and baseline verification passed for both manifests;
the existing reference-review tool produced both adaptation reports without
changing source hashes. `git diff --check` passed.

On 2026-10-24, both `f2fs_module` and `f2fs_native` passed on macOS ARM64,
Linux x64 and Windows x64. The final batch includes the optimized-layout guard,
summary bounds and creation/change-time distinction. The macOS application was
built successfully; both guests built the affected module and native consumers.
Existing incremental build trees, prepared runtimes and compiler caches were
reused. These results do not establish real-media authorization, damaged-volume
recovery, flexible-inline fixture acceptance or full kernel feature coverage.

### Integration after rebase — 2026-10-24

Rebased onto master `ce9e9a45` with the LZ4 Frame and Lzip additions. Only
overlapping documentation needed conflict resolution; the F2FS reader, native
adapter and tests are unchanged. On macOS ARM64, `f2fs_module`, `f2fs_native`,
`lz4_frame` and `lzip` passed together (4/4) using the existing incremental build
and compiler cache. Linux/Windows were not repeated for this integration;
their preceding F2FS evidence above remains dated to that implementation.
