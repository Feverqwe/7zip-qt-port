# P3-03 — current default gates and caption-fit regression, 2026-10-17

Product `2a7303c1379f051154878947bb9062c1b9cac4bb`, Qt6.12.0, is checked on
macOS arm64 and the existing Linux/Windows x64 Hyper-V guests. The official
engine remains https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. No upstream or production source
changes are made by this test correction.

## Full runs, before the test correction

| Platform | Default CTest | Qt passed/failed/skipped | Time |
| --- | --- | --- | ---: |
| macOS | 329/330 PASS; language_dialog fails | 5073/1/66 | 403.97s |
| Linux | 331/332 PASS; language_dialog fails | 5067/1/66 | 585.10s |
| Windows Limited | 331/348 executed PASS; 17 fail | 5259/71/20 | 2231.68s |

All three runs pass formatting and all-target compilation, then restore every
captured source input. Windows additionally restores stock CMake and successfully
reconfigures/rebuilds external_codec_tests and process_pause_tests. Its failures
are language_dialog and16 privilege/setup suites; these remain failed in the
original full receipt. external_codecs13Qt, codec_probe18Qt and process_pause4Qt
pass in this run. This does not explain or erase historical cold deadline failures.

The first Linux invocation fails before CTest when cc1plus is killed. A serial
build retry uses the same persistent current/build/dev and completes the above
run. Memory pressure is a hypothesis, not a proven kernel OOM event. The initial
driver's copied old LastTest is stale and is excluded from these CTest results.

Twelve slow suites are excluded. Earlier explicit acceptance of eleven long
suites stays separate; the declined twelfth matrix is not run. Windows omits the
VHD-creating rows under the user's no-new-virtual-disks instruction: fs_drives_device
is disabled, fs_drives_device_ui runs its logical-drive row, namespace_hash runs
logical-drive and source ADS rows. Omitted rows are not counted as passes.

## Observable caption fit, rather than a preferred footer width

The source Copy Browse button deliberately retains its20DLU width, accepted in
the [Copy runtime comparison](p3-06-copy-runtime-2026-10-17.md). The language
fixture still required every button except a private-name Split exception to be
at least its Qt preferred size. This rejects valid narrow source buttons on all
three platforms, even when their displayed caption fits.

The existing fixture now checks each visible button's displayed mnemonic-aware
caption against its style content rectangle. Visibility, label fit, EN/RU, raw
values and normal/compact scenarios remain. No private-name whitelist or product
layout change is introduced. This removes a refactoring-sensitive assertion;
it does not relax the source workflow or require wider buttons to satisfy a test.

After focused target builds, default language_dialog passes1/1 on each OS:
macOS1.03s, Linux0.70s, Windows2.24s. Native Cocoa/XCB/Windows each pass8Qt/0fail/
0skip with38 current widget JSON reports. Source guards pass. Linux uses private
Xvfb/Openbox; macOS uses the allowed native Qt producer, with no screenshots or
agent OS input. C++ is clang-formatted and diff check passes.

The original full failures and successful focused follow-ups form a composite
receipt. None of the original full gates is relabelled PASS. Their unchanged
passing portions are not rerun merely for this test-only correction.

## Separate Windows privilege fixtures

Run `20261017T135110Z-088f4b33` builds the15 affected native test targets and
runs exactly16 suites with the previously authorized scoped administrator token:
owned_temp, temp_cleanup, source_extract_advanced, source_link_extract,
link_dialog, archive_creation_lifecycle, filesystem_link, panel_rename_state,
project_memory_setup, source_link_storage, file_manager_startup,
archive_metadata_ui, network_folder, native_network_navigation,
native_persisted_locations and native_address_navigation. The project-memory
fixture is a script, without a compiled target.

All16 pass49.57s,229Qt/0fail/0skip. The actual administrator-token guard and
source-unchanged guard pass; no VHD fixture or slow suite is selected. Owned
symlink/share fixtures retain their normal cleanup. This is a focused supplement,
not a repeated Highest full gate or a relabelling of the Limited result.

Evidence: `build/review/goal-final-default-2a7303c/`. Guest full run
`20261017T130157Z-0a616180`, Linux serial retry `20261017T130334Z-3c363939`,
language follow-ups Linux `20261017T133546Z-7dff3adf` and Windows
`20261017T135003Z-29371d0e`; original LastTest/source manifests and follow-up
default/native logs/reports are retained. The eight official runtime/SFX inputs
are reused from hash-verified protected producers. Persistent builds/caches and
active evidence are preserved. No push, publication or upstream modification.

P3-03/P3-06 remain active: current passing codec checks are not a causal cold
stability fix, and the remaining native workflow matrix has separate acceptance.
The parent count remains44/47.
