# Relative dot paths during extraction — 2026-10-17

Source reference: <https://github.com/ip7z/7zip.git>, commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

## Source comparison and adaptation

Reviewed `CPP/7zip/UI/Common/ExtractingFilePath.cpp` (`Correct_PathPart`,
`Correct_FsPath`, `MakePathFromParts`) and the callers
`CArchiveExtractCallback::CorrectPathParts` / `GetExtractStream` in
`CPP/7zip/UI/Common/ArchiveExtractCallback.cpp`, together with the path-part
types, header and `Extract.cpp` initialization. The source removes dot directory
components before forming the filesystem output path. The port already retains
these bodies; no upstream module or algorithm is replaced by this correction.

The Qt preflight in `archivebackend_validation.cpp` previously rejected `.`
components, preventing extraction of ordinary `./name` CPIO payloads. It now
accepts current-directory components and reuses the existing `SplitPathToParts`,
`Correct_FsPath` and `MakePathFromParts` for output conflict checks. Raw traversal
components remain rejected before source correction. A directory entry naming
the extraction root is accepted. Selection continues to use literal archive
names; stream-specific handling remains with its existing source adaptation.

Dependencies are the already linked Common string/path types and adapted
`ExtractingFilePath` module. The separate Qt security boundary deliberately
retains stricter rejection of traversal, absolute paths outside the destination,
reserved names, conflicting outputs and unsupported links. Native source output
and link checks remain in `CQtExtractPathPolicy`. No presentation changes or
additional source omissions are introduced.

## Runtime validation

On 2026-10-17, macOS arm64, the working-tree change passed:

- `./scripts/build.sh dev archive_backend_tests source_extract_advanced_tests
  source_extract_backend_tests sevenzip_desktop`; a focused incremental follow-up
  built the changed fixture consumers and `extraction_options_tests`.
- `./scripts/ctest.sh --preset dev -R
  '^(archive_backend|source_extract_advanced|source_extract_backend|extraction_options)$'
  --output-on-failure`: four suites passed. This covers CPIO extraction through
  the process backend and source batch workflow (file service and native handler),
  normalized output collisions, raw traversal rejection and existing extraction
  success/error/cancellation scenarios. NTFS-only checks are skipped on macOS.

Fixtures are synthetic; the reported user installer is not used as a test
fixture. Source comparison is limited to the path correction and consumers above;
passing suites do not establish source completeness for unrelated modules.
After an explicit user request, a separate manual macOS check opened the reported
installer in the built GUI through XAR → gzip → Portable ASCII CPIO → `.` and
extracted that folder's contents. All 35 regular files matched the official
`7zz` extraction in relative names, SHA-256, size and permission bits. The inner
CPIO reports `Headers Error` in the official engine, but both extractions produced
the same complete listed contents. This is manual user-file evidence, not a
committed regression fixture; the installer was neither installed nor executed.

A subsequent manual check selected only the application folder from that same
bound `.` folder: its 34 regular files matched the full extraction, including
permission bits. The automated `nested_archive` regression now covers opening a
synthetic ZIP → gzip → CPIO chain, entering `.`, and extracting either its entire
contents or only the selected application directory. A sibling sidecar verifies
that selection is respected. ZIP is used for the synthetic outer container
because the official engine cannot create XAR; the real XAR chain is covered by
the manual check above.

The follow-up built `nested_archive_tests`, `archive_backend_tests` and
`source_extract_advanced_tests` with the existing dev build/cache. The shared
fixture consumers `archive_backend` and `source_extract_advanced` passed; after
correcting the generated gzip filename, the final focused `nested_archive` run
also passed (1.31 seconds). All checks used the project build/CTest wrappers.

### CPIO header warning

Reviewed the same upstream revision's `CPP/7zip/Archive/CpioHandler.cpp`,
`CHandler::Open` hard-link grouping and `GetArchiveProperty` warning flags.
The actual Portable ASCII payload has 47 entries (12 directories, 35 regular
files), a complete `TRAILER!!!` and zero tail padding. Eleven AppleDouble sidecar
entries have distinct device/inode identities and nonempty data, but declare
more than one link. The original handler reports this discrepancy through
`_numLinks_Error` as `Headers Error`; it does not mean the payload is truncated.
Changing only those eleven link-count fields to one in a disposable diagnostic
copy removes the warning in the official engine. The user installer remains
unchanged; the diagnostic copy was removed. No CPIO handler change or suppression
of upstream warnings is needed for this extraction fix.

Windows and Linux have not been checked for this change.

## Bound current-folder extraction — 2026-10-17

Follow-up source review at the same full upstream revision covered
`CPP/7zip/UI/Agent/Agent.cpp` (`CAgentFolder::Extract`, both `BindToFolder`
overloads), `AgentProxy.cpp` (`GetDirPathParts_isChanged`), FileManager
`PanelCopy.cpp` (`CThreadCopy::ProcessVirt`) and Common
`ArchiveExtractCallback.cpp` (`Init`, `GetStream`, `CorrectPathParts`). The source
folder selects archive indices and supplies its exact path parts to the callback;
`GetStream` checks and removes that prefix before filesystem name correction.
These existing ported bodies are preserved, including original link handling
and the final Qt destination/link policy. No additional upstream bodies are
copied or omitted.

The Qt preflight now accepts that actual bound folder as explicit context. In
current-path mode, when it matches the requested current folder, it checks the
unmodified suffix after verifying the full component boundary. Thus a source
folder literally named `..` can yield an ordinary safe destination filename.
Parent components remaining in the suffix still fail before upstream correction
can remove them. Full-path, no-path and console/batch validation retain their
previous raw-path checks; supplying only a requested current folder does not
grant this exception. Stream-specific checks remain unchanged.

MainWindow supplies the prefix of the bound Agent folder; it does not infer it
from the selected name or clean it as a filesystem path. The native decoder still
selects exact archive indices, and the file-service fallback still selects the
full original member name into a controlled stream. Neither transports output
paths containing the removed prefix to the filesystem writer.

Focused regressions cover matching/mismatched/absent source context, a nested
prefix, remaining traversal, prefix-boundary confusion, reserved output names
and occupied destinations. The nested payload scenario also covers entering a
literal `..` folder and extracting either its contents or only the application,
using both file-service and native decoder paths when the module is available.
On macOS arm64, the existing incremental dev build compiled the application and
the affected test targets. The project CTest wrapper passed all six selected
suites: `archive_backend`, `nested_archive`, `extraction_options`,
`source_extract_advanced`, `source_extract_backend` and `ui_copy_routing`
(16.06 seconds total). The nested suite passed both literal-parent-folder rows
for each transport; native module rows were enabled and did not skip. Existing
root traversal refusals, source output/link checks, overwrite, error and
cancellation scenarios remained covered. `git diff --check` passed. No layout
or presentation was changed; Windows and Linux are unverified for this follow-up.
