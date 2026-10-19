# Extraction of stream items without stored names — 2026-10-19

Source: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`. Existing adapted modules retain
their recorded baselines; no new upstream algorithm is copied.

Reviewed `CPP/7zip/Archive/GzHandler.cpp` (`CHandler::GetProperty`),
`CPP/7zip/UI/Common/OpenArchive.{h,cpp}` (`CArc::GetItem_Path`,
`GetItem_DefaultPath`), `CPP/7zip/UI/Agent/AgentProxy.cpp` (`CProxyArc::Load`),
and Agent `GetRealIndices` / `Extract`, including the extraction callback
initialization with the owning `CArc`. An absent gzip filename is legal.
The source proxy and extractor resolve it using the archive's default name.

The Qt selection/preflight boundary instead queried raw `kpidPath` and silently
returned when the handler reported `VT_EMPTY`. Consequently F5 Copy and toolbar
Extract accepted their destination dialog but produced no output. Dragging used
the source extractor and succeeded. A gzip entered from the read-only ext4
RECOVER panel reproduces this difference independently of the earlier POSIX
colon validation defect.

A narrow Agent adapter now exposes its owning `CArc::GetItem_Path` for a real
archive index. Qt selection validation and the archive-entry projection both
reuse that method. Item enumeration/index expansion, default-name rules,
extraction, destination containment and platform filename correction remain
owned by their existing source implementations. Raw archive property semantics
are preserved. No journal, device access or spool ownership policy changes.

The existing ext4 timestamp fixture contains a gzip without a stored filename.
The device UI regression explicitly accepts reading without journal replay,
enters that gzip, selects the derived SQL name and submits both Copy and Extract
dialogs. It checks SQL bytes, refresh, full gzip extraction, parent navigation,
whole-filesystem extraction and unchanged source bytes. Existing Russian rows
retain copying of the outer gzip; English rows exercise the nested member,
avoiding duplicate language variants for this nonvisual correction.

Before the fix, both nested commands failed to create their output; the original
expanded diagnostic run reached its 90-second timeout after three such failures.
The same nested checks passed after the change. This is fixture evidence, not
validation of real pending journal transactions or a physical device.

Validation: seven focused suites PASS on macOS arm64 and Linux x64:
`device_ui`, `nested_archive`, `source_extract_backend`,
`native_archive_properties`, `extraction_options`, `ui_copy_routing`,
`ui_copy_same`. macOS's initial native-properties run selected a different
Homebrew engine version; repeating that suite with the matching project engine
on PATH passed. Linux receipt: `20261019T171705Z-8e99fc15`, persistent tree/cache,
single build job. The final device matrix ran within the ordinary test budget.
The refreshed install-only macOS app passed deep/strict ad hoc signature and
native startup smoke checks. The user also confirmed that copying now works.

Windows initially passed the other six suites; the nested UI fixture expected
the original POSIX name after source `PanelItemOpen.cpp` had correctly sanitized
the temporary archive filename for Windows. The test now accounts for that
platform behavior while checking the original name again after returning to the
device. The product correction is identical on all platforms. The final
`device_ui` retry passed on Linux and Windows in
`20261019T172304Z-b34afee7`; combined with the first batch, all seven suites pass
on both guests. Windows used its logged-on desktop; the device fixture itself
uses offscreen Qt on the guests and native Cocoa locally. No full gate or opt-in
slow suites were run. Disposable test data was cleaned; builds/caches retained.
