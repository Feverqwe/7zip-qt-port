# Architecture

## Reading guide

Start with the affected boundary below, then inspect its current source bodies,
callers and dependencies. [Provenance](upstream-port-audit.md) records borrowed
code and adaptations; [testing](testing.md) routes behavior to focused checks.
Detailed dated component investigations remain in the
[archive](archive/qt-port/architecture/component-notes.md).

## Repository boundary

`src/CPP/7zip/` contains adapted upstream SDK modules at their original paths;
`src/gpl/CPP/7zip/` contains the separate Linux/GPL archive-reader variants.
`third_party/7zip/` contains reusable upstream dependencies. Local Qt integration
lives under `src/`. Preserve one authoritative implementation per module variant and
portable upstream types. [AGENTS.md](../AGENTS.md#source-port-requirements)
owns the source-port rules; the [manifest](upstream-baseline.json) owns revisions.
The [variant audit](upstream-port-audit/license-variants.md) records the split and
its engine/module boundaries.

The application is a C++17 / Qt 6 Widgets host for original FileManager/Agent/GUI
logic. Qt adapters replace Win32 presentation, event delivery and native services.
The official archive engine remains responsible for formats and compression.

## Window and panel state

Original `CApp`/`CPanel` own command routing, selection, navigation and panel
operation semantics. `MainWindow` projects that state into widgets and connects
the platform services. It must not become a second owner of archive algorithms
or command-availability decisions.

`QtAppOperationState` extends original timer/notification guards across asynchronous
work. Each window retains its own operation state and panel bindings. Completion
must update the captured owning panel even if focus changes while a modal dialog
or worker is active. Navigation, refresh and publication preserve independent
peer state and reject stale results after a location/provider change.

Filesystem enumeration and archive folder bindings feed the list/model projection.
Original item indices, marks and focused row are distinct from display text and
Qt selection. Sorting, column metadata and native namespaces retain their source
owners; presentation does not infer operation paths from rendered cells.

Main-window chrome has a scoped Qt presentation adapter for toolbar spacing,
labels and the parent button. It does not style dialogs or own commands; the
editable address retains the original panel address/navigation owner.

## Dialogs and preferences

Original dialog/page modules own values, validation, prompts and operation
decisions. Qt owns widget construction, native-font geometry, focus and signal
transport. Shared geometry services adapt dialog units and work-area positioning;
the [text specifications](ui-layout-contracts.md) own observable layout contracts.

Contextual forms opt into parent-window modality on macOS through the shared
presentation adapter. Layout construction itself does not make windows modal.
Operation progress keeps its existing application-wide blocking policy;
source synchronous dialog results and worker/callback ownership remain intact.

Preferences are applied through the source Options/page owners. Cancel must
preserve stored settings and registration state. Native capabilities and privilege
failures must remain visible to the owning operation rather than silently changing
the workflow. Platform menu, association, file picker and application-startup
services stay behind narrow adapters.

Settings presentation organizes those six owners into task pages with adaptive
navigation. Controls read the owners' live draft without another persistence
layer. Format search only filters the unified list; bulk selection covers the
whole list. Checkboxes project the source association states through the original
transition; advanced controls retain its full cycle and supported system scope.

Interface text is owned by Qt Linguist catalogs. The language service discovers
catalogs; module-owned translators support the GUI and the native shell host,
including DLL unload before its host application exits. Upstream resource IDs
remain an adapter boundary to those catalogs. Standard Qt widget catalogs
are merged at build time. Presentation bindings retain source/context for live
retranslation; namespace icons are selected by resource identity. See
[localization](localization.md) for contributor commands.

## Filesystem and volume workers

Original filesystem, copy and volume modules own operation plans and results.
Qt workers provide scheduling, progress and cancellation without changing those
algorithms. Widgets are updated on the GUI thread; worker and callback lifetimes
must outlive any queued completion or modal prompt they can produce.

Use deterministic private fixtures for partial writes, conflict decisions,
cross-volume behavior and cancellation. Native Trash, devices, links, metadata
and permissions have separate platform boundaries; do not infer their support
from an ordinary local file copy.

## Archive backend and safety

`ArchiveBackend` owns archive integration: official native-module and process
transport, argument construction/parsing, validation, cancellation and results.
Widgets invoke operations through this boundary and do not construct `7zz` syntax.
Source extract/update callbacks and Agent folder bindings retain the original
functional owners.

Treat archive names and metadata as untrusted. Validate output destinations and
member paths at the operation boundary, preserve literal Unicode arguments and
avoid shell interpretation. Transactional update and temporary-resource owners
retain original archives and leases until the operation completes. Extraction
has different partial-output semantics, described in [parity](7zfm-parity.md).

Nested archives and external editing require explicit resource lifetimes across
open processes, pending writes and parent-chain updates. Module images, streams,
temporary files and folder providers cannot be released while consumers retain
them. Failure and cancellation must leave a reusable application state and
preserve retained edit data where the operation requires it.

## Diagnostics

Original Progress/Messages and source error paths own user-facing operation
diagnostics. Process/worker adapters deliver progress, pause, cancel and completion
to those owners. Avoid logging passwords or user archive contents.

Widget JSON and geometry producers support targeted UI review. Native platform,
physical input, OS panels and rendered reference checks have distinct scopes;
see [runtime comparison](ui-runtime-comparison.md). Dated validation belongs in
[source audit](upstream-port-audit.md), rather than this architecture guide.
