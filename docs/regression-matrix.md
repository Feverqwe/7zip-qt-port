# Regression and security traceability

This is the P3-02 scenario-to-owner-to-CTest register for the accepted P0–P2
scope. [Testing](testing.md) owns fixture details and reproduction commands;
[the checklist](porting-checklist.md) owns acceptance status. Test registration
alone is not a passing result. Current three-platform results, actual Qt rows,
skips and retained failure/retry evidence belong to P3-03. Native Finder and
rendered Windows UX remain separate mandatory checks.

All fixtures use disposable local data, official source/native providers or
explicit process/callback fixtures. They do not use personal archives or live
services. UI state tests exercise public commands and signals; no internal
production access is added for this matrix.

| Package / scenario | Functional owner and integration boundary | CTest consumers |
| --- | --- | --- |
| P0-01 source inventory | Per-module audit and baseline | `upstream_inventory`, `upstream_baseline` |
| P0-02 independent panels | CApp/CPanel; Qt per-window operation state | `app_state`, `panel_runtime`, `binding_lifecycle` |
| P0-03 availability and dispatch | CApp/WndProc/PanelMenu; QtFileManagerCommands | `native_command_dispatch`, `command_policy_runtime`, `menu_policy` |
| P0-04 address/parent/nested rollback | PanelFolder/PanelOpen; native folder binding | `native_navigation_window`, `panel_navigation`, `ui_navigation`, `nested_archive` |
| P0-05 filesystem/flat enumeration | FSFolder/PanelItems; scanner/model projection | `fs_folder`, `filesystem_scanner`, `flat_view` |
| P0-06 columns/sort/time/refresh | PanelSort/PanelItems; Qt header/viewport adapter | `panel_columns`, `panel_sort`, `panel_auto_refresh`, `view_settings_columns` |
| P0-07 focus/marks/keyboard/mouse | PanelSelect/PanelKey; shared hover adapter | `panel_key`, `panel_selection`, `list_settings_runtime`, `list_activation` |
| P0-08 two-panel focus/persistence | CApp/Panel; MainWindow projection | `panel_focus`, `panel_runtime`, `window_info`, `ui_copy_same` |
| P0-09 menu/toolbar/context/status | MyLoadMenu/PanelMenu/PanelListNotify; Qt presentation | `main_shell`, `panel_shell`, `native_toolbar_state`, `file_menu_policy` |
| P0-10 progress/pause/background/cancel | CProgressDialog/CProgressSync; process/worker transport | `progress_dialog`, `progress_cancel`, `progress_priority_failure`, `process_pause`, `worker_checkpoint` |
| P0-11 conflict decisions/rollback | OverwriteDialog/AutoRenamePath/FSFolderCopy; transaction | `extract_prompt`, `file_transfer`, `file_transfer_direct` |
| P0-12 selected/root/batch Ask | ExtractGUI/ExtractCallback/PanelCopy; ArchiveBackend path guard | `source_extract_backend`, `source_extract_contract`, `extraction_options`, `ui_operations` |
| P1-01 disk/archive transfer routes | CApp::OnCopy/PanelCopy/Agent; backend snapshot/rollback | `ui_operations`, `ui_copy_same`, `archive_transfer_backup`, `source_update_backend` |
| P1-02 copy/merge/cross-volume Move | FSFolderCopy/CCopyStateIO; exclusive transaction | `fs_folder_copy`, `file_transfer`, `file_transfer_direct`, `worker_window_progress` |
| P1-03 create/rename/delete/trash/comments | Panel/FSFolder/FileDir; native trash service | `filesystem_operations`, `native_fs_operations`, `ui_operations`, `filesystem_comment` |
| P1-04 archive mutation/publish | ArchiveFolder/AgentOut; ArchiveBackend native/CLI transport | `source_update_backend`, `archive_mutation`, `codec_update`, `ui_archive_states` |
| P1-05 Add/update/path modes | CompressDialog/UpdateGUI/UpdatePair; Qt controls/backend | `source_compress_dialog`, `shell_update_window`, `ui_update_modes`, `source_update_backend` |
| P1-06 archive volumes | CArchiveLink/CMultiOutStream; leased volume publication | `archive_volume_creation`, `volume_workflow`, `ui_archive_volumes` |
| P1-07 options/memory/deletion lifecycle | COptionsDialog/CompressDialog/Update; Qt control/worker bridge | `compression_advanced`, `compression_memory`, `compression_limit`, `archive_creation_lifecycle` |
| P1-08 external View/Edit/Diff/VerCtrl | PanelItemOpen/EditItem/VerCtrl; OS process and GUI delivery | `archive_external_edit`, `archive_external_lifetime`, `archive_external_security`, `external_editors_ui`, `verctrl` |
| P1-09 nested parent write chain | PanelItemOpen/AgentOut; retained resource/parent publication | `nested_edit_chain`, `nested_update`, `owned_temp_publication` |
| P1-10 properties/raw/folder size | PropertyName/PropIDUtils/Agent/Panel; typed native property projection, retained native levels/catalog diagnostics | `source_properties`, `native_archive_properties`, `properties_runtime`, `property_formatting`, `folder_size_view` |
| P1-11 root/mount/network/Browse/device image | RootFolder/FSDrives/NetFolder; platform mount service and original device CopyTo | `root_navigation`, `volume_mount`, `network_folder`, `browse_dialog`, `fs_drives_device`, `fs_drives_device_ui`, `native_network_navigation` (Windows) |
| P1-12 Options Apply/Cancel | OptionsDialog/*Page; Qt page container | `options_dialog`, `options_integration`, `options_window`, `fm_settings` |
| P1-13 Edit/Menu policies | EditPage/MenuPage/RegistryContextMenu; OS registration service | `external_editors_ui`, `menu_page`, `context_menu_settings`, `shell_registry` |
| P1-14 language/fallback/focus | CLang/LangUtils/*Dialog; Qt LanguageChange/bindings | `language`, `language_ui`, `language_dialog`, `language_standard_buttons`, `language_focus`, `language_source_update` |
| P1-15 Help/About/temp cleanup | HelpUtils/AboutDialog/TempFiles; CHM viewer and owned leases | `help_source`, `help_ui`, `temp_cleanup`, `owned_temp`, `temp_detached_launch` |
| P1-16 panel/archive drag | PanelDrag/CompressCall2; QDrag/MIME/leased materialization | `drag_drop`, `drag_archive`, `drag_drop_ui`, `drag_drop_add_ui`, `source_drop_add` |
| P1-17 ZIP item comment | ArchiveFolder/Zip writer; retained native writer callback | `native_comment`, `native_comment_ui`, `native_zip_writer` |
| P1-18 real decoder RAM request | RAR handler/OpenCallback/ExtractCallback; backend callback bridge | `native_rar_memory`, `native_rar_ui`, `open_callback` |
| P2-01 formats/capabilities/KeepName | LoadCodecs/OpenArchive/Agent; immutable provider leases | `codec_catalog`, `codec_capabilities`, `codec_commands_ui`, `native_open_commands` |
| P2-02 SFX/external modules | WriteSfxModule/LoadCodecs; official module identity/publication | `sfx_creation`, `sfx_lifecycle`, `external_codecs`, `codec_probe` |
| P2-03 CRC/Hash/Test | HashCalc/HashGUI/ExtractGUI; native providers and results view | `native_hash_workflow`, `native_hash_ui`, `hash_results`, `shell_archive_services` |
| P2-04 Benchmark | Bench/BenchmarkDialog; Qt controls and worker lifetime | `native_benchmark`, `benchmark_command`, `benchmark_lifecycle` |
| P2-05 links/junctions/metadata | FileIO/FileDir/Reparse/LinkDialog; native platform bridge | `filesystem_link`, `link_command`, `source_link_storage`, `source_metadata_storage` |
| P2-06 ADS/proxy2 | AltStreamsFolder/AgentProxy/CCopyStateIO; platform stream transport | `alt_streams_command`, `alt_streams_archive_ui`, `source_properties`, `native_update_path` |
| P2-07 absolute/ElimDup/security extraction | ArchiveExtractCallback/FileDir; backend bounded-path/link guard | `source_extract_advanced`, `source_link_extract`, `extraction_options`, `archive_metadata_ui` |
| P2-08 associations/open events | RegistryAssociations/SystemPage; per-user native APIs | `association_services`, `association_file_open`, `system_page`, `file_manager_startup` |
| P2-09 native shell/menu/GUI integration | ContextMenu/MyLoadMenu/CompressCall; COM/HMENU/process services | `shell_context`, `shell_menu_window`, `shell_update_gui`, `shell_registry`; Windows `native_menu_integration`, `native_shell_data`, `native_system_menu`, `native_gui_workflow` |
| P2-10 bounded progress/list/lifetime | Source enumeration/copy/hash/split; Qt worker scheduling | `large_list_window`, `worker_progress`, `worker_window_progress`, `archive_preparation`, `archive_volume_creation` |
| P2-11 one authoritative implementation | Source DTO/properties/volumes/Sync; reviewed integration boundaries | Direct consumers in [the glue register](upstream-port-audit.md#explicit-root-file-scope-258-paths-reviewed-component-register); all-target compilation |

The new `archive_workflow` fixture connects create/list/selected extraction,
local editing, rejected unsafe replacement, successful update, relist/Test,
full extraction and canceled/successful filesystem Move in one backend lifetime,
for ZIP and 7z. It verifies member sizes and exact bytes, retained members,
original disk sources, unchanged archive during transfer, and private-stage
cleanup. It adds transition coverage to the separate operation regressions.

| Critical failure / security property | Concrete public regression |
| --- | --- |
| Wrong password/refusal followed by retry; no partial output or password in output | `archive_backend::encryptedRoundTrip/encryptedCreationLaunchFailure`; `source_update_backend::protectedPlanWrongPasswordAndCancelKeepOriginal`; `archive_external_security` |
| Invalid/traversal/absolute names cannot publish | `archive_backend::unsafePath/unsafeListing/unsafeDestination`; `codec_update::unsafeNativeMutationDoesNotPublish`; `native_update_path`; `source_update_backend::absolutePlanRejectsTraversalAlias` |
| Linked destination and native links/ADS cannot escape bounded output | `extraction_options::skipRejectsLinkedDestination`; `source_link_extract`; `source_extract_advanced`; `temp_cleanup`; `native_update_path` |
| Case/Unicode equivalence and duplicate names do not prove native indices | `codec_update::canonicallyEquivalentMetadataDoesNotProveNativeIndexes/duplicateNamesDoNotProveNativeIndexes`; `extraction_options::flatNameCollision`; `archive_mutation::renameImplicitZipFolderAndRejectCollision` |
| Interrupted update/cancel preserves original and permits reuse | `archive_mutation::cancelledReplacementPreservesArchive/addFailureAndCancellation`; `source_update_backend::cancellationInNativeTransactionKeepsOriginalArchive`; `owned_temp_publication`; `archive_workflow` |
| Partial Move/overwrite failure cannot silently discard sources | `file_transfer::crossVolumeDeleteFailureReportsPartialMove/directoryMergeCancelRestoresOriginalTree`; `ui_operations::archivePanelsCopyAndMoveThroughStaging`; `archive_transfer_backup` |
| Device image Copy/Cancel/failure preserves its read-only source | Windows `fs_drives_device::sourceCopyUsesReadOnlyPrivateDevice(cancel/failure/skip/move)`; `fs_drives_device_ui::sourceDeviceNamespaceAndF5` |
| Namespace Hash availability; private device SHA256 success/Cancel/refusal/recovery and ADS digest | `namespace_hash` (all-platform capability guard; Windows native read-only VHD/ADS, EN/RU result geometry and unchanged input) |
| Process startup/error/fragmented output/reset | `archive_backend::missingExecutable/createFailureAndRetry`; `archive_mutation::fragmentedOutputAndError`; `codec_probe` |
| External child/temp lifetime and pending modal close | `archive_external_lifetime`, `archive_external_security`, `temp_detached_launch`, `owned_temp` |
| Window destruction joins workers and rejects obsolete callbacks | `worker_window_progress`, `large_list_window`, `app_lifecycle_window`, `binding_lifecycle`, `archive_preparation` |
| Nested error/wrong-password rollback and read-only refusal | `nested_edit_chain::rejectedInnerPasswordRestoresParent/readOnlyLevelPreventsChainWrites`; `nested_update` |
| Reports omit editable values, including shown passwords | `compression_encryption::methodsAndShowPassword` verifies shown and masked report dumps omit both inputs; `tests/layout_inspection.cpp` report schema; current report/log scan in P3-02 evidence |

Windows privileged link/security fixtures and ordinary-token COM registration
have different requirements; native desktop tests require session1. Linux/macOS
do not support every Windows ADS/NT-security/COM service. Actual compiled slots
and data rows, with explicit platform applicability, must be checked before
acceptance; a passing executable that skips its required fixture is insufficient.
Windows native network coverage now uses its own current-user isolated share
fixture in `network_folder`; acceptance and retained history belong to P3-03.5.
The Unix mounted-share fixture remains separate. POSIX-only skip rows do not
replace Windows required coverage; device fixtures fail when their prerequisites
cannot be exercised.
Finder drag-out cannot be accepted using fake receivers or these Qt tests.
