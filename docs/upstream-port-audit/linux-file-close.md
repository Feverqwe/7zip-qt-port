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
