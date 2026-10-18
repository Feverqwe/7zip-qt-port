# Linux native-file close ownership — 2026-10-18

Source: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Reviewed the original `CPP/Windows/FileIO.cpp` POSIX open, close, read/write,
metadata and destruction paths, their `FileIO.h` owners, the COM adapters in
`CPP/7zip/Common/FileStreams.cpp`, and close/cleanup/publication consumers in
`MultiOutStream.cpp`, native `UI/Common/Update.cpp` and QtArchiveVolumeOutput.
The complete FileIO implementation was identical to the recorded upstream
before adaptation; it moves to `src/CPP/Windows/FileIO.cpp` as its sole copy.
Existing private Windows include roots resolve its unchanged source dependencies,
including `StdAfx.h`, `FileIO.h`, `FileName.h` and device `C/Alloc.h`.

On Linux a failed close can already have released its descriptor. Keeping that
integer allowed an explicit retry or either file destructor to close a different
file that reused the descriptor. The Linux branch now relinquishes ownership
before invoking close, while returning the original result and errno. Other
platform branches retain their source behavior. This platform distinction is
required by the documented [Linux close semantics](https://man7.org/linux/man-pages/man2/close.2.html);
in particular, Linux EINTR differs from the POSIX.1-2024 contract.

The multi-volume owner also assumed a failed close retained an open stream.
Its destructor detached the COM owner and skipped deletion, leaking the owner
and leaving temporary output. On Linux it now releases the already-closed stream
and removes its linked-list entry even on error. Abort cleanup attempts file
deletion after the failed close and preserves the first close error. Finalization
still returns the close error before publication; subsequent abort cleanup can
remove the temporary output. The source stream algorithms, metadata handling,
normal success paths and non-Linux close behavior remain in place; no module,
functional branch or dependency is omitted.

The Linux-only `linux_file_close` regression interposes close in its own test
executable. A real kernel close is followed by immediate descriptor reuse for a
disposable file and an injected EIO, ENOSPC, EDQUOT or EINTR. Public input/output
file methods must report the original error and preserve the replacement file
through retry and destruction. Volume abort and final-flush cases also verify
the error result, retained replacement descriptor and removal of temporary
output. All cases failed before the fix and passed afterwards on Linux x64;
the fixture completes in hundredths of a second and needs no timing or threads.

This adaptation applies to file owners linked into Sevenzip Desktop. The
separately built official engine and codec module retain their own upstream
FileIO implementation; this change does not patch those external binaries or
the read-only upstream checkout. Their error paths are outside this fix's
runtime acceptance. macOS close-error semantics are not inferred from Linux.

The application and twelve direct-consumer suites (native filesystem operations,
hashing, browse, extraction, archive update, ZIP, SFX and volume creation) passed
on macOS arm64, Linux x64 and Windows x64 / Qt 6.12. Linux additionally passed
the new close-error regression. Guest run `20261018T095559Z-d9d5aef7` used the
same source digest
`df718b705fb90f487ee430524016f9ca47f512cd4a0f6044f97722ad5421ef7e` on both
platforms. Existing incremental builds and compiler caches were retained.
The baseline verifier passed using a disposable index that represented both
the FileIO move and the concurrent, unstaged CommandLineParser move; the real
index excluded the latter from this commit. No full gate, opt-in slow matrix
or rendered UI comparison was run for this nonvisual change.

## POSIX output reuse and deferred metadata — 2026-10-18

The subsequent complete FileIO.cpp/header review also covered Windows handle
creation/close, device size/read buffers, read/write/seek, metadata and their
FileStreams, MultiOutStream, extraction and filesystem consumers. The Windows
association adapter retains local QSettings/string owners through registry
revision collection and original registry operations; no new lifetime gap was
established there. Windows close behavior was not changed to imitate Linux.

The source POSIX COutFile open wrapper assigned the next Path before calling
the base OpenBinary, whose nonvirtual Close bypassed COutFile's deferred
metadata finalization. Reusing an output object therefore lost the first
file's requested timestamp and could apply it to the second file. The same
loss occurred when opening the next path failed. Both implicit-close cases
failed on the macOS baseline; the explicit-close control passed.

OpenBinary_forWrite_oflag now calls the existing COutFile::Close before
changing Path, respects its failure, and clears the previous file's deferred
metadata flags before accepting another output. The existing metadata, time
conversion, OS I/O and error algorithms are retained; mode_for_Create is
unchanged. This is a local ownership correction in the copied source, not a
new filesystem implementation. No module, dependency or source branch was
omitted; the borrowed formatting outside the small change is retained.

The new public file_output_lifecycle suite reuses one output for two private
files, checks their bytes and timestamps, and covers rejected reopening plus
explicit Close. The fixture uses Unicode/spaces and ordinary local operations;
there is no memory-failure injection or standalone Qt example. Windows runs
the same cases as a control for its immediate metadata implementation.

The application built on macOS arm64, Linux x64 and Windows x64 / Qt 6.12;
guest application run `20261018T131613Z-d8054420`. file_output_lifecycle,
multiout_lifecycle, filesystem_link and source_update_backend passed on all
three platforms, with linux_file_close also passing on Linux
(`20261018T131315Z-216007e4`). archive_volume_creation in that batch failed
only its two normal/compact visibility assertions in the concurrently changing
Add dialog; that suite is not accepted as a whole by this iteration.
Additional archive_preparation, source_extract_backend and native_zip_writer
suites passed on macOS (3/3, 0.78 s), Linux (3/3, 1.05 s) and Windows
(3/3, 4.20 s), guest run `20261018T131502Z-e36fd2af`.
Existing builds and compiler caches were reused. No full/slow gate or new
appearance acceptance was run; the independent Add geometry failures remain
explicit.
