# P3-03.8 — Current Windows codec execution boundaries (2026-10-17)

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

The [final acceptance record](../ux-port-audit.md#final-acceptance-of-product3a0b3b7)
accepts the bounded cold/GUI execution criterion using the first-after-reboot
receipt below and current delivery. The historical delay's cause remains
unestablished. Dated diagnostics below retain their original failed results and
the acceptance status at that point; they are not causal-fix claims.

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

## Direct native-call observation, v209

Run `20261017T082215Z-2c2c7520` executes only `codec_probe`. A zero-delay
instance of its existing own-process `DelayedProcessStart` fixture observes
the first deadline regression; subsequent intentional600ms rows retain their
own instances. No arguments, process flags, environment or application/state
deadlines change. The observer skips `Sleep(0)`, starts timing after the entry
stderr flush and preserves Win32 last-error across logging. It is removed after
the probe's worker has joined. This is temporary snapshot instrumentation, not
a product hook or a system-wide observer.

The whole probe passes18Qt/0fail/0skip. In the first50ms deadline regression,
native `CreateProcessW` returns successfully after532ms. The Qt started signal
and `process.start` return have the same recorded millisecond tick as the native
return; worker admission has the full50ms budget. This run therefore separates
the native boundary from Qt's post-call work, but does not reproduce the
earlier8594ms refusal or establish its cause. Passing this instrumented run is
not a cold-stability fix; no additional identical replay is needed to make that
limitation clear. `external_codecs` is not selected in v209.

All three instrumented files are restored, generated CTest commands match their
stock configuration, the stock target rebuild passes and all eight runtime/SFX
hashes match both snapshot and protected producer. Collected evidence is
`windows-final-codec-phases-v209-evidence/` under the ignored root below.
Preflight removes only obsolete objects from the completed v192 compile-only
failure, retaining source, executable artifacts and failure logs; host D: is OK.

## Actual GUI executable context, 2026-10-17

Run `20261017T170249Z-94f35cce` observes the actual product56c01ba Windows GUI
entry, rather than the console QtTest process. Rechecked its main/desktop/backend
discovery consumers, catalog/image owners and the pinned Qt6.12 process body.
Qt selects CREATE_NO_WINDOW when GetConsoleWindow is null; no product flag
override is needed. Temporary own-process observation uses the existing
zero-delay native fixture, preserving arguments, flags and Win32 last error.

The executable's PE subsystem is GUI. A real MainWindow becomes visible; its
native CreateProcessW call has parentConsole=0, flags134218752 and succeeds
in39842 microseconds. Discovery returns S_OK with a catalog, without timeout or
cancellation, and the application quits through a queued Qt call after delivery.
The original5000ms discovery deadline remains unchanged. This establishes the
actual GUI context and rejects a blind “add CREATE_NO_WINDOW” correction; it
does not reproduce or explain the historical nine-second delay. Comparison with
the earlier console timing is warm/order-confounded, not causal evidence.

All four temporarily instrumented files are restored; the stock application
target rebuild succeeds. All1058 product/build inputs and eight runtime/SFX
hashes match afterwards. The existing incremental tree/compiler cache is reused.
Only the completed package run's hash-verified disposable extraction is removed;
artifacts, references and failure evidence are retained. Host storage before/after
is OK. No security setting, VM lifecycle, process flags, deadline, global observer,
cache flush or upstream source is changed. Evidence remains in
`build/review/goal-gui-context-20261017/`; the one-off driver is archived there
and removed from scripts. Windows cold cause/stability remains open.

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

## First execution after an authorized guest reboot

Run `20261017T171918Z-82c643e3` tests product56c01ba / documentation6fc84c7
after a fresh boot of the existing runner-windows guest. The user authorized
the graceful guest reboot and subsequent console login. The guest's boot epoch
changes from1792011374.6563091 to1792257316.137739; SSH identity and the runner
console session are verified. No host reboot, forced application termination,
authentication-policy change or automatic-login configuration is used.

The three targets were built before reboot in the existing incremental tree.
The first codec suite after login is `external_codecs`:13Qt PASS in4656ms
(CTest1/1,5.69s). It is followed by `process_pause`:4Qt PASS in1412ms and
`codec_probe`:18Qt PASS in8724ms. All35 checks pass without failures or skips;
the5000ms application deadline remains unchanged. No build or runtime
preparation occurs after reboot. All1486 prepared source hashes and17 executable,
DLL/configuration hashes match after execution; the final uptime is299437ms.

This establishes the bounded first-after-boot scenario for that revision.
It does not establish the cause of the earlier8–9s native process/loader stalls
or erase their failed receipts. Final stability acceptance remains open.
Actual LastTest logs, preparation/boot guards and results are retained under
`build/review/goal-postboot-codec-20261017/acceptance/`; the one-off driver is
archived in that task directory and removed from scripts. Post-run host storage
is OK; existing guest build trees, runtime and compiler caches are preserved.
