# P3-03.8 — Current Windows codec execution boundaries (2026-10-17)

This is diagnostic evidence, not a cold-stability fix. Product inputs retain
`e3b3c652d4d2489d9143f3002c498057d6bd3c38`; upstream remains read-only at
https://github.com/ip7z/7zip.git commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. Reviewed original/adapted
`CCodecs::LoadDll`, `IsSupportedDll`, `LoadExternalModules`, `CLibrary::Load`
and the Qt probe/catalog/image owners and cancellation/publication callers.
Temporary instrumentation preserves their control flow and Win32 last error.
No functional source, engine, deadline, priority or security policy changes.

## Two current diagnostic snapshots

Limited/Normal snapshot `20261017T072238Z-d8d259bf` (`v190`) executes the two
complete `external_codecs`/`codec_probe` suites with verbose output and the
original caps. Both fail. External initialization starts its worker in3ms with
4997ms remaining, returns from process.start in45ms, reaches cancellation at
the5000ms deadline and waits another3011ms for the child. The still-running
QProcess warning is retained. Its Qt total is1pass/1fail in8707ms. Probe's
50ms deadline regression spends8490ms inside process.start, then fails its
original callback wait. Later native loading reaches `LoadLibraryW.begin`
without a corresponding return before the unchanged20s CTest cap. This run
does not provide a complete probe Qt total or establish why loading stalled.

Snapshot `20261017T073829Z-4ab65ca1` (`v194`) replaces Qt logging with stdio,
preserves GetLastError and adds a no-op public
`QProcess::setCreateProcessArgumentsModifier` plus a direct started-signal
observer. It changes no creation arguments. External codecs pass13Qt in5320ms;
probe completes17pass/1fail in16857ms. The failing50ms deadline regression
records8594ms between the modifier immediately before CreateProcessW and the
started signal. Worker admission and Qt preparation before that modifier are
fast. The inspected, SHA256-pinned Qt6.12.0 `qprocess_win.cpp` shows that this
interval also includes post-call handle closing/notifier setup; it must not be
reported as an exact isolated CreateProcessW duration. A direct native-call
control is the next discriminator. The four intentional600ms creation-delay
rows and native600/2100ms query-delay rows retain their fixture semantics.
This passing external-codec execution does not erase earlier cold failures.

The official wrapper/helper bytes match the current root; the launcher records
Session1, Limited, Elevated=False, Priority=Normal. No executable from the
completed v190/v192 snapshots remains running in the subsequent owned-path
process query. All five instrumented source owners and generated CTest commands
are restored, stock targets rebuild successfully, and all eight runtime/SFX
hashes match snapshot and protected producer after validation in both runs.
All11 slow suites remain excluded from these focused diagnostics; their
separately requested three-platform acceptance retains its recorded scope.

## Harness failures and retained evidence

`v189` (`20261017T071827Z-026e7e4e`) fails before tests on a private Python
source write using Windows' default code page. Explicit UTF-8 corrects that
driver. `v192` (`20261017T073302Z-18bea7f4`) fails to compile the private
GetTickCount64 trace under the borrowed unit's legacy SDK admission macros;
v194 uses the available GetTickCount for its short bounded interval. Neither
failed harness is a product regression or a test PASS. Both restore every
source/runtime input and rebuild stock targets; separately verified stock test
commands were never mutated. Their result's uncaptured-config flag remains
false instead of being rewritten into a restore claim.

Ignored root: `build/review/final-availability-8e9343b-20261014/`, containing
`windows-final-codec-phases-v{189,190,192,194}-evidence/`, exact instrumentation
patches, actual LastTest/result/source/config guards, preflight/storage receipts
and the pinned Qt process sources. Host D: storage is OK; only obsolete object
files from completed Copy run `20261017T041351Z-682bec5c` are removed, preserving
its source/binaries/logs, newest/active runs and all protected producers. No
disk creation/growth, cache flushing, Defender change, WPR/ETL export, deadline
relaxation or upstream modification is part of this investigation. Final
Windows cold stability remains open.
