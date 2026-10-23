# Progress controls without background priority

User request, 2026-10-23. Transferred
`9808a74de17f851fc02d793a5237b0f2ef3959e2` and the guarded progress-transport
backend reference from `33828e5a4d3849d59d2547c1cae919d80ad5e9e4`.

Source: <https://github.com/ip7z/7zip.git>,
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
`CPP/7zip/UI/FileManager/ProgressDialog2.cpp/.h`, `ProgressDialog2.rc` and
`ProgressDialog2a.rc`; also compared the sibling at
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`. Reviewed initialization, title,
priority, pause, completion, cancel, resource buttons and the Qt transport callers.
The user-requested omission removes Background/Foreground and process-priority
services, including phase inheritance and dedicated priority tests. Cancel
receives initial/default focus; Pause/Continue, confirmation and Close retain
the existing source adaptation. Qt owns layouts and native button roles.

Focused validation on macOS, Linux and Windows passed progress synchronization,
worker checkpoints/waits, progress worker, archive progress, cancel, formatting,
dialog geometry, resource contract comparison, checksum and toolbar archive
workflows. Current normal/compact widget JSON shows reachable Pause/Cancel and
separate errors/footer, including completion with Close. macOS/Linux used
offscreen Qt; Windows used the logged-on desktop and native QPA. No rendered
Windows reference was captured; this is structural usability evidence.

The expanded consumer check also reported failures outside the changed cases:
`language_dialog::archiveDiagnosticValuesKeepPathsAndSelection` and both
`ui_operations::encryptedAskUsesSourceCallback` rows, identically on all three
platforms. Their bodies and functional owners are unchanged by this transfer;
the progress-language and other UI operation cases passed. These failures are
recorded separately from acceptance of the removed priority capability.
Slow suites and a full gate were not run. The runner receipt is
`20261023T180121Z-40f9b011`; incremental builds and compiler caches were retained.
