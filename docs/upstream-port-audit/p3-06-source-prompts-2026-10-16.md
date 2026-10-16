# P3-06 — Full text, password, memory and diagnostic presenters (2026-10-16)

[Index](../upstream-port-audit.md). Current native presenters pass on all three
OS. Eight original EN/RU initial views and a successful current Windows RAR
consumer are verified; the earlier startup failure remains retained. This receipt does not close
P3-06 or its parents.

## Source boundary

Baseline https://github.com/ip7z/7zip.git,
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`:
`CPP/7zip/UI/FileManager/{EditDialog,PasswordDialog,MemDialog,MessagesDialog}`
`.cpp/.h/.rc`, resource headers and `CPP/7zip/GuiCommon.rc`.
Reviewed complete bodies and callers: `CListViewDialog::ShowItemInfo`,
`PanelDrag.cpp` terminal messages, `COpenArchiveCallback::Open_CryptoGetTextPassword`,
`CExtractCallbackImp::CryptoGetTextPassword/RequestMemoryUse`, and
`CUpdateCallbackGUI2::ShowAskPasswordDialog`. Portable UString parsing, RAM query,
error formatting and callback result policies retain their existing upstream
implementations. The upstream checkout is read-only; original notices remain.

Adaptation stays in the corresponding `src/CPP/7zip/UI/FileManager` files.
No new module is copied/omitted and no archive algorithm or backend transport
changes. Edit/Password initialization replaces hardcoded pixel dimensions with
336×256/216×88DLU source clients, native font/minimum protection, eight-DLU
margins and64×16DLU footer buttons. Password label/edit/show positions retain
source gaps; native standard-button ordering and compact accessibility resizing
remain Qt adaptations of its fixed source template. Edit remains read-only,
literal/nonwrapping, with Enter retained in its edit and full stored text.
Initial Close focus/default restores source initial Enter-to-close; Tab reaches
the read-only edit. Messages also initially focuses Close; Memory initially
focuses Save limit and Password its edit. Original first-tab-stop behavior is
verified through GetGUIThreadInfo, not inferred from the default-button style.

Memory restores336×216DLU,72DLU message/62DLU Action group,50DLU edit/spinner
pair and source insets/gaps. Native control minimums can enlarge the client.
`BuildLimitRow`/`BuildActions` keep the presentation constructor focused; other
handlers and state remain in the original module. `UDS_ALIGNRIGHT` shrinks the
buddy edit inside the combined width, and `UDS_ARROWKEYS` routes ↑/↓ to the same
step operation as the arrow buttons. These Win32 control services have no
portable upstream Qt implementation; the narrow existing Qt control adapter
supplies them. Buttons are not independent tab stops; disabled edit is unchanged.
See [Microsoft's native style definitions](https://learn.microsoft.com/en-us/windows/win32/controls/up-down-control-styles).
Raw typed-value validation and spinner range remain independent, including zero
and invalid-value retry; source NeedSave/Remember/SkipArc/Cancel are unchanged.

Messages already has its456×176DLU source frame and original OnSize arithmetic;
it now requests source minimize/maximize capabilities and preserves them when
assigning its modal owner. Edit also requests them. Password/Memory retain their
fixed-template capabilities. All four omit context-help title-bar buttons;
native modal window managers may suppress requested chrome.

## Regressions and verified scope

Actual pre-product RED has eight failures: hardcoded Edit/Password/Memory frames
in EN/RU, and absent Messages flags after assigning a parent. Inputs are proven
equal to5f9e258 before changes. The bounded `source_prompt_presentation` suite
checks all four source-initial EN/RU frames, flags, label/footer fitting and
exposed controls; existing normal/compact/language and behavioral suites remain.
It also checks centered paired Memory edit width and Remember inset.

Native review finds disappearing footer gaps at native minimum sizes. An
additional actual RED4 covers Password/Memory EN/RU; minimum gaps12/10DLU are
now explicit. Cocoa's native button-box backing rectangle extends one pixel
into its layout spacer. The source gap is asserted through public layout-item
geometry, while actual widget visibility/fitting assertions remain. Qt6.12
performs that native margin conversion in
[QWidgetItem::geometry](https://github.com/qt/qtbase/blob/6.12/src/widgets/kernel/qlayoutitem.cpp#L456).
The first Cocoa raw-rectangle attempt remains retained as a failed observation.

Separate actual RED2 shows buddy-edit Up retaining2 instead of3 and Down
retaining2 instead of1. The event filter now calls the existing StepSpin handler;
the parser/range implementation is unchanged. Public tests compare keyboard
behavior with the existing buttons and check disabled-value preservation and
NoFocus buttons. They do not claim physical OS keyboard delivery.

The first unsandboxed macOS Qt6.12 full gate passes324/324 in265.96s,
5170Qt/0fail/66skip before the localized gap/keyboard completion. Subsequent
affected checks pass13/13 in6.80s for gaps and6/6 in5.64s for the keyboard route.
Latest native Cocoa Edit/Password/Memory/Messages consumer passes4/4 in2.19s,
37Qt/0fail/0skip, with tracked source inputs unchanged and generated CTest
configuration restored. No macOS OS input or screenshot is used. The earlier
native4/4 result35Qt predates the keyboard regression and remains historical.
The earlier focused backend/contract check includes actual native RAR memory
35Qt/0skip, password opening and callbacks;87Qt/0fail/0skip overall.

Linux initial fresh snapshot `20261016T070238Z-09f00594` passes11/11 focused
(136Qt/0fail/0skip,2.49s) and4/4 native (35Qt/0fail/0skip,0.52s), before localized
gap/keyboard completion. It retains30JSON/14PNG. Linux follow-up
`20261016T071531Z-a69084d3` passes four native suites but fails native_rar_memory
before its cases: the one-off runtime copy used copyfile and lost7zz executable
mode. Bytes/pin are intact; this is retained as an environment/harness failure.
The next fresh snapshot uses copy2, verifies modes and executable access, and
rehashes all eight official runtime/SFX inputs before configuring/building.
It reuses the pinned engine, without claiming a new engine build. Final current
Linux snapshot `20261016T072535Z-85fda308` passes13/13 in2.77s and4/4 native in0.52s;
138Qt/0fail/0skip focused and37Qt/0fail/0skip native; product-input proof is
retained with its evidence. Private
Xvfb/Openbox uses private XDG/config paths, never changes HOME, and does not
prove physical-console input.

The current Linux whole `20261016T074214Z-2dbbb5ac` passes323/323 in529.30s,
5164Qt/0fail/66skip. All1626 uploaded inputs match terminal guest files and
stock Git diff is empty; synthetic snapshot
`0d2c96b94f15a458acec87a66d20c13a802552c7`. Logs/config and independent source
proof are in `linux-source-prompts-full-gate-evidence`.

Windows ordinary Session1/Limited/Priority=Normal snapshot
`20261016T075352Z-68007f92` passes all four native presenter suites in2.59s,
37Qt/0fail/0skip,30JSON/14PNG. Its focused set passes12/13 in53.35s,
104Qt/1fail/0skip: native_rar_memory init's catalog-ready check arrives12800ms
after its5s wait, before the UI cases. This original cold/startup failure is
retained in `windows-source-prompts-native-v3-evidence`; it is not accepted as
a passing real RAR workflow. Bounded replay preserves deadlines and source.
The previous frozen5f9e258 whole Windows gate independently finishes339/341
with process_pause timeout and codec_probe entered-marker delay; it does not
cover this newer component or establish a cold-delay fix.

### Final source-focus and rendered-reference completion

Original reference producer `20261016T083051Z-20d41d13` builds the pinned Windows
sources using a private test entry adapter; final capture controller
`20261016T092550Z-afbf9754` captures eight initial EN/RU windows. The original
process is Limited/Elevated=False before startup and private registry access.
All18 original presenter/resource/shared/FM input bodies are independently
verified against Git blobs; the Windows archive changes only LF to CRLF.
The entry calls the original presenters directly and uses official pinned26.03
language assets verified against `resources/7zip/provenance.json`. Each process
exits normally and removes its private reference key; binary/adapter hashes and
source inputs remain unchanged. Edit uses the real ShowItemInfo caller's CRLF
fixture convention via WM_SETTEXT; this does not claim the complete FM workflow.
All eight final reference PNGs are unobstructed and reviewed. Earlier console-
occluded captures and the raw-LF Edit fixture remain excluded from acceptance.

Original GetGUIThreadInfo shows Edit IDCLOSE8, Password edit120, Memory Save7801,
and Messages Close1 in both languages. Actual RED5 exposes wrong Edit/Messages
focus and initial Edit Return behavior. The Qt presenters now preserve these
states; explicit Memory Save focus prevents Cocoa from choosing Continue.
Focused public tests check initial focus, default Close/Return and Tab-to-edit.
Edit scrollbar checks wait for deferred document layout with the existing
QTRY deadline; the actual scrollability/fitting requirements are unchanged.

Final unsandboxed macOS full gate passes324/324 in266.43s after the behavior
changes,5173Qt/0fail/66skip; native Cocoa passes4/4 in2.73s,38Qt/0fail/0skip. No macOS OS input or
screenshots are used. Final Windows/Linux snapshot
`20261016T094647Z-56a2315a` passes13/13 focused and4/4 native on each guest,
139Qt/0fail/0skip focused and38Qt/0fail/0skip native. Summed per-test times are
Windows35.25/2.48s and Linux3.05/0.87s. The Windows focused set includes actual
native RAR memory without retry. All12 affected owner/header/build/test files
match the final local inputs byte-for-byte; generated configurations are restored.
Both guests retain30JSON/14PNG. Current initial grouping/order/labels/footer fit
are compared with the eight original views; native font/minimum/button ordering
adaptations remain explicit. This is bounded presentation acceptance, not pixel
equality or a complete physical-input/full workflow audit. Linux uses private
Xvfb/Openbox; Windows uses ordinary Session1/Limited/Normal priority.

Two original-producer native RAR replays also pass35Qt each without skips.
The initial catalog startup failure and earlier Linux mode-copy/native deferred-
layout failures remain retained. Eleven slow suites remain excluded. The prior
Linux whole323/323 predates final focus changes and is historical; current
Windows whole stability, final packages and whole native/workflow acceptance
remain required.

Evidence root: ignored `build/review/final-availability-8e9343b-20261014`:
`source-prompt-frame-red-evidence`, `source-prompt-footer-gap-red-evidence`,
`source-memory-spinner-key-red-evidence`, `source-prompt-*`/`source-memory-*`
logs, `mac-source-prompts-full-*`, `mac-source-prompts-native-v1` through`v4`,
`source-prompt-focus-red-evidence`, `mac-source-prompts-final-full-*`,
`mac-source-prompts-native-v6`, `windows-source-prompts-original-clean-evidence`,
and `{linux,windows}-source-prompts-native-final-v2-evidence`. Historical failed
attempts are retained; generated configurations are restored.
