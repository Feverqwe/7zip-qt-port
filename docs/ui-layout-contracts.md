# UI specifications

Before changing layout, text, visibility or resize behavior, read the affected
specification and exact original resources/handlers/callers. Inspect current-build
normal/compact widget JSON; include language, focus and conditional states relevant
to the change. Commands and evidence boundaries are in
[geometry](testing/geometry.md), [resource tooling](ui-reference.md) and
[native runtime comparison](ui-runtime-comparison.md).

Specifications describe required observable behavior. Historical package IDs and
dated checks in the detailed sections identify their origin; they do not reopen
the completed port or establish current acceptance. New validation receipts belong
in [the audit](upstream-port-audit.md). Update the affected contract when its
observable requirements change; keep implementation history in the
[archive](archive/README.md).

| View or state | Contract |
| --- | --- |
| Rendered Add/Extract reference | [Спецификация](ui-layout-contracts/shared.md#rendered-addextract-reference-p3-06-2026-10-15) |
| Native Copy terminal errors | [Спецификация](ui-layout-contracts/shared.md#native-copy-terminal-errors-p3-05) |
| Native System handler images | [Спецификация](ui-layout-contracts/shared.md#native-system-handler-images-p3-06-2026-10-17) |
| Refresh focus and alternative marks | [Спецификация](ui-layout-contracts/shared.md#refresh-focus-and-alternative-marks-p3-05) |
| Panel address keys and flat view | [Спецификация](ui-layout-contracts/shared.md#panel-address-keys-and-flat-view-p3-05) |
| Archive directory CRC projection | [Спецификация](ui-layout-contracts/shared.md#archive-directory-crc-projection-p3-05) |
| Changed nested archive Parent | [Спецификация](ui-layout-contracts/shared.md#changed-nested-archive-parent-p3-0515) |
| Native external update publication | [Спецификация](ui-layout-contracts/shared.md#native-external-update-publication-p3-0515) |
| Native nested opening | [Спецификация](ui-layout-contracts/shared.md#native-nested-opening-p3-0515) |
| Source panel creation | [Спецификация](ui-layout-contracts/shared.md#source-panel-creation-p3-0515) |
| Source startup window placement | [Спецификация](ui-layout-contracts/shared.md#source-startup-window-placement-p3-0516) |
| Source Copy/Move destination | [Спецификация](ui-layout-contracts/shared.md#source-copymove-destination-p3-0515-required-scope) |
| Persisted source locations | [Спецификация](ui-layout-contracts/shared.md#persisted-source-locations-p3-0514-required-scope) |
| Source root command target | [Спецификация](ui-layout-contracts/shared.md#source-root-command-target-p3-0513-implementation-validation-active) |
| Source address dropdown | [Спецификация](ui-layout-contracts/shared.md#source-address-dropdown-p3-0512-required-remaining-scope) |
| Source caption helpers | [Спецификация](ui-layout-contracts/shared.md#source-caption-helpers-p3-0510-validation-active) |
| Namespace Hash result consumers | [Спецификация](ui-layout-contracts/shared.md#namespace-hash-result-consumers-p3-058-validation-active) |
| Standard source dialog buttons | [Спецификация](ui-layout-contracts/shared.md#standard-source-dialog-buttons-p3-059-accepted-button-scope) |
| Source shell popup | [Спецификация](ui-layout-contracts/shared.md#source-shell-popup-p2-09-in-progress) |
| Shell Menu page | [Спецификация](ui-layout-contracts/shared.md#shell-menu-page-p2-09-accepted) |
| System associations | [Спецификация](ui-layout-contracts/shared.md#system-associations-p2-08-accepted) |
| Add metadata Options | [Спецификация](ui-layout-contracts/shared.md#add-metadata-options-p2-07-in-progress) |
| Link | [Спецификация](ui-layout-contracts/shared.md#link-p2-05) |
| Benchmark | [Спецификация](ui-layout-contracts/shared.md#benchmark-p2-04) |
| SFX Options group | [Спецификация](ui-layout-contracts/shared.md#sfx-options-group-p2-02) |
| File-list drag target | [Спецификация](ui-layout-contracts/shared.md#file-list-drag-target-p1-16-wip) |
| Commands and reports | [Спецификация](ui-layout-contracts/shared.md#commands-and-reports) |
| Alternate streams namespace | [Спецификация](ui-layout-contracts/shared.md#alternate-streams-namespace-p2-06-active) |
| Add archive naming metadata | [Спецификация](ui-layout-contracts/shared.md#add-archive-naming-metadata-p2-01) |
| Open archive picker | [Спецификация](ui-layout-contracts/shared.md#open-archive-picker-p2-01) |
| Delete Temporary Files | [Спецификация](ui-layout-contracts/shared.md#delete-temporary-files-p1-152) |
| Offline Help and About | [Спецификация](ui-layout-contracts/shared.md#offline-help-and-about-p1-151) |
| Live dialog language consumers | [Спецификация](ui-layout-contracts/shared.md#live-dialog-language-consumers-p1-142) |
| Menu page | [Спецификация](ui-layout-contracts/shared.md#menu-page-p1-131) |
| Advanced compression options | [Спецификация](ui-layout-contracts/shared.md#advanced-compression-options-p1-071) |
| Compression lifecycle and memory rejection | [Спецификация](ui-layout-contracts/shared.md#compression-lifecycle-and-memory-rejection-p1-072) |
| Archive volume creation | [Спецификация](ui-layout-contracts/shared.md#archive-volume-creation-p1-061) |
| Existing archive Add options | [Спецификация](ui-layout-contracts/shared.md#existing-archive-add-options-p1-051) |
| Main menu and toolbar | [Спецификация](ui-layout-contracts/shared.md#main-menu-and-toolbar-p0-091) |
| Per-panel context and status | [Спецификация](ui-layout-contracts/shared.md#per-panel-context-and-status-p0-092) |
| Two disk panels | [Спецификация](ui-layout-contracts/shared.md#two-disk-panels-p0-081) |
| Mixed and restored panels | [Спецификация](ui-layout-contracts/shared.md#mixed-and-restored-panels-p0-082) |
| Virtual root main window | [Спецификация](ui-layout-contracts/shared.md#virtual-root-main-window-p0-042) |
| Mounted volumes | [Спецификация](ui-layout-contracts/shared.md#mounted-volumes-p1-111) |
| Windows device namespace and F5 Copy | [Спецификация](ui-layout-contracts/shared.md#windows-device-namespace-and-f5-copy-p3-057) |
| Network namespace | [Спецификация](ui-layout-contracts/shared.md#network-namespace-p1-113) |
| Source extraction prompts | [Спецификация](ui-layout-contracts/shared.md#source-extraction-prompts-p0-041-prerequisite) |
| Archive metadata cells | [Спецификация](ui-layout-contracts/shared.md#archive-metadata-cells) |
| Source sorting in the file list | [Спецификация](ui-layout-contracts/shared.md#source-sorting-in-the-file-list-p0-061) |
| Source property columns | [Спецификация](ui-layout-contracts/shared.md#source-property-columns-p0-062) |
| Time precision and UTC | [Спецификация](ui-layout-contracts/shared.md#time-precision-and-utc-p0-063) |
| File list settings | [Спецификация](ui-layout-contracts/shared.md#file-list-settings) |
| External editor settings | [Спецификация](ui-layout-contracts/shared.md#external-editor-settings) |
| VerCtrl commands and Revert | [Спецификация](ui-layout-contracts/shared.md#verctrl-commands-and-revert) |
| Folder history list | [Спецификация](ui-layout-contracts/shared.md#folder-history-list) |
| Hash results | [Спецификация](ui-layout-contracts/shared.md#hash-results) |
| Full text detail | [Спецификация](ui-layout-contracts/shared.md#full-text-detail) |
| Selected properties | [Спецификация](ui-layout-contracts/shared.md#selected-properties) |
| GUI early error messages | [Спецификация](ui-layout-contracts/shared.md#gui-early-error-messages) |
| Shared name/mask dialog | [Спецификация](ui-layout-contracts/shared.md#shared-namemask-dialog) |
| Split and Combine dialogs | [Спецификация](ui-layout-contracts/shared.md#split-and-combine-dialogs) |
| Copy/Move dialog | [Спецификация](ui-layout-contracts/shared.md#copymove-dialog) |
| Extract dialog | [Спецификация](ui-layout-contracts/shared.md#extract-dialog) |
| Create archive dialog | [Спецификация](ui-layout-contracts/shared.md#create-archive-dialog) |
| Source long-path browser | [Спецификация](ui-layout-contracts/shared.md#source-long-path-browser) |
| Archive password prompt | [Спецификация](ui-layout-contracts/shared.md#archive-password-prompt) |
| Progress dialog and modal lifecycle | [Спецификация](ui-layout-contracts/shared.md#progress-dialog-and-modal-lifecycle) |
