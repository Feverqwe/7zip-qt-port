# File-transfer exception ownership — 2026-10-18

Source reference: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`,
`CPP/7zip/UI/FileManager/FSFolderCopy.cpp`, CopyFile_Ask/CopyFolder and their
state/callback types. The complete QtFileTransferTransaction and
FileTransferOperation bodies/headers, callback bridge, native link/file
services and window result/join consumers were reviewed. The existing
[transaction boundary review](../archive/qt-port/upstream-port-audit/p2-11.md)
records why upstream's immediate per-entry move does not provide this
whole-batch staging and rollback service. Source copy recursion, native I/O,
overwrite and progress implementations remain authoritative and unchanged.

The Qt transaction registered a completed item after publication, but had no
cleanup owner covering exceptions before registration. Earlier published items
also had no exception rollback, and exceptions could escape the QThread entry
point. An exception during source deletion could bypass the existing partial
move recovery after some source bytes had already been removed.

The current item now retains a scope guard until its completed record is
accepted. A second guard retains batch rollback until the completion phase
takes responsibility. Both use the existing move/copy restoration rules;
backup ownership is recorded only after a successful backup rename. Source
deletion exceptions enter the existing partial-move path, preserving the
published copy after an attempted deletion. The worker boundary converts other
escaping exceptions to a failure result after cleanup. No source algorithm,
module boundary or dependency was replaced, and no source branch was omitted.
The existing per-item loop remains together to preserve rollback ordering;
this focused fix does not further split the reviewed transaction component.

Two new public worker regressions use its existing filesystem adapter and
private Unicode/spaced paths. A rename-service exception on the second source
must restore the first source and the previous destination bytes without stage
residue. An exception after actual source removal must report failure while
preserving the removed file's published bytes and the untouched second source.
Existing coverage did not exercise throwing services. The old source allowed
these exceptions to escape the worker; no pre-fix crashing run was performed.
Tests use ordinary C++ service exceptions, without allocation-failure injection,
Qt modification, standalone reproducers or real user files.

The affected targets rebuilt incrementally and passed file_transfer,
file_transfer_direct, language_file_transfer, ui_copy_same and worker_checkpoint
on macOS arm64 (5/5, 0.95 s), Linux x64 (5/5, 2.75 s) and Windows x64
(5/5, 10.07 s), Qt 6.12. Guest run: `20261018T134603Z-8d612cf9`.
Compiler caches and persistent build trees were retained. These checks cover
the transfer worker and its direct consumers; no full/slow gate or UI appearance
change/acceptance is claimed.
