# Unknown archive member external opening — 2026-10-24

Compared complete `CPanel::OpenItem`, `OpenItemInArchive`, `DoItemAlwaysStart`
and their launch/edit callers, `BrowseDialog2::StartApplication`, `CEditPage`
and RegistryUtils with the read-only source checkout of
<https://github.com/ip7z/7zip.git> at
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`. Existing imported inventory units
and original hashes remain in [the baseline](../upstream-baseline.json).
No external source code is imported by this change.

Upstream automatically launches an extracted member after archive probing
returns S_FALSE. It has no unknown-type confirmation preference. The port keeps
the source probing, extraction, password/error branches, process tracking and
temporary-copy lifetime. A narrow Qt service boundary asks before the automatic
handoff when the system has no open handler. Cancellation occurs before external
protection or worker ownership, allowing normal temporary-directory cleanup.
Explicit Open Outside and View/Edit retain their source launch paths.

The new preference, Applications checkbox and confirmation are requested local
integration. The checkbox uses the existing EditPage draft/apply owner without
changing the source viewer/editor/diff settings. The system queries are local
platform adapters because the Win32 launcher provides no reusable cross-platform
query implementation: Windows uses
[AssocQueryStringW](https://learn.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-assocquerystringw),
macOS uses NSWorkspace's URLForApplicationToOpenURL, and Linux resolves the MIME
type and queries [GIO's default application](https://docs.gtk.org/gio/type_func.AppInfo.get_default_for_type.html).
These are read-only queries and do not test-launch handlers. A registered handler
does not guarantee that it can decode the individual file; existing launch-error
reporting remains authoritative. The native Qt confirmation follows the
[Apple alert guidance](https://developer.apple.com/design/human-interface-guidelines/alerts)
for explicit action/cancel choices; Cocoa pixel parity and physical keyboard
acceptance are separate from widget geometry and behavior checks.

Source scope is complete for this automatic archive-member handoff; no decoding
or recovery behavior changes.

Validation on 2026-10-24, working tree based on
`b2a6f26af4b45735dea9296fa858f2093b6d0a31`: macOS application build and focused
archive_external_lifetime, external_editors_ui, options_dialog, options_keyboard,
archive_item_open, nested_archive and language checks passed. The regressions
cover the default question, cancellation/cleanup, explicit acceptance,
known-handler bypass, disabled preference, explicit outside command and settings
Apply/Cancel. Cocoa also passed the six fallback cases and EN/RU normal/compact
Applications checks; current widget JSON shows the checkbox fully exposed with
room for both captions. A sandboxed archive_item_open attempt could not observe
the TXT association; its unsandboxed rerun passed. That transport suite now
explicitly disables the question so it does not depend on installed handlers.

Linux compiled the new query and main changed targets, but additional Options
targets failed to link existing ArchiveCommandLine references to
NUpdateArchive::k_ActionSet_Add/Delete/Update before CTest could run
(`20261024T065138Z-6e006336`). Windows was occupied by a separate runner owner.
The final retry of both platforms stopped at host SSH authentication
(`20261024T065522Z-03acd3a6`); neither platform has runtime acceptance for this
change. Existing incremental trees, compiler caches and other tasks were retained.
