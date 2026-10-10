# Regression and security traceability

Source long-path picker and portable dialog boundaries: `source_browse_dialog`,
`browse_dialog`, `combo_dialog`, `copy_dialog`, `volume_dialog`, `language_dialog`,
`source_archive_browse`, `compression_format`. Owned long Unicode fixtures
cover routing, filters/sort, parent/reload/create/activation, error and cancellation,
raw names and output state. Geometry emits8 EN/RU normal/compact file/folder JSON.
Normal native picker and source custom browser are distinct test paths; neither
offscreen result establishes rendered Windows or final package acceptance.

The ordinary picker additionally covers source Open-new-name and Save-existing
without an extra confirmation, unchanged bytes/cwd and accepted-only publication.
CommonDialog's original inherited DTO/filter contract is exercised by Browse
and the real Compress/Edit consumers, rather than a separate Qt filter API.

Password source DTO/process caller: `password_window`, `password_dialog`,
`open_callback`, `update_callback`, `archive_progress`, `archive_opening`,
`language_dialog`, `ui_archive_states`, `archive_backend`, `ui_parity_password`.
Public raw empty/Unicode/spaces, source caching/ShowPassword, cancellation,
reopen/reset, local encrypted failure/retry and redacted geometry are covered;
see the [receipt](upstream-port-audit.md#p3-05--password-state-and-process-callers-2026-10-10).

Operation prompt source lifetime/error-policy consumers: `extract_prompt`,
`password_dialog`, `language_dialog`, `language_standard_buttons`,
`native_rar_memory`, `native_extract_callback`, `archive_opening`,
`update_callback`, `progress_dialog`, `ui_parity_overwrite`, `ui_parity_mem`.
Public modal Close/Enter/Escape and invalid-limit suppression/correction retain
source rows/results/fields; see the [receipt](upstream-port-audit.md#p3-05--operation-prompts-and-source-modal-lifetime-2026-10-10).

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
| P3-05 source context menu | CPanel OnContextMenu header/list/keyboard/empty/Cancel/numeric routing; captured owning pane, popup peer-focus/Close lifetime and actual source Create | `source_context_menu`, `panel_shell`, `shell_context`, `shell_menu_window`, `native_command_dispatch`, `source_create`, `panel_name_text` |
| P3-05 PanelOperations/Comment | Complete worker/Create/Delete/Rename/Comment disposition; source-owned guarded publication, Cancel, failed reload/recovery and real ZIP/AES/nested writes | `filesystem_comment`, `native_comment_ui`, `native_comment`, `source_create`, `source_delete`, `panel_rename_state`, `panel_refresh_names`, `panel_refresh_notifications`, `panel_refresh_drop`, `panel_refresh_completion` |
| P3-05 ViewSettings/Copy history | Source UStringVector/upper-case distinction, K/K persistence and CApp Cancel/accept/reopen; popup completion restores command availability; related binary settings/columns/toolbar consumers | `copy_dialog`, `app_copy_routing`, `view_settings_columns`, `settings`, `native_toolbar_state`, `list_settings_runtime`, `main_shell`, `persisted_location`, `fm_settings`, `toolbar_archive_workflow`, `source_context_menu` |
| P3-05 six Options pages/Apply errors | Source PSNRET_NOERROR after association error continues later pages; Settings PSNRET_INVALID and MyMessages policy remain | `options_integration`, `options_dialog`, `system_page`, `menu_page`, `language`, `language_ui`, `options_window`, `association_services` |
| P3-05 source selection lifecycle | CPanel owns mask dialog/default/Cancel/application; source index/mark loops, both panes/modes, inactive owner and independent peer | `source_selection`, `panel_selection`, `ui_state`, `flat_view`, `combo_dialog`, `panel_key`, `panel_item_changed` |
| P3-05 source Rename activation | Source focused-row RenameFile and parent/read-only OnBeginLabelEdit; table/icon editor, Escape and operated marks independent of focus | `panel_rename_state`, `panel_key`, `panel_item_changed`, `panel_name_text`, `ui_operations`, `alt_streams_command` |
| P3-05 source Delete lifecycle | CPanel confirmation/accepted-result refresh and one source error, No/Cancel early return, partial bytes/provider/peer and deferred guards; Move transaction retains explicit-index seam | `source_delete`, `ui_operations`, `native_fs_operations`, `panel_key`, `alt_streams_command`, `archive_transfer_backup` |
| P3-05 source Create File/Folder | CPanel owns complete dialog/validation/operation/refresh and first-component focus; alternative marks, collision, Cancel and deferred-scan guards | `source_create`, `large_list_window`, `ui_operations`, `panel_key`, `alt_streams_command` |
| P3-05 source Split/Combine | CApp selection/peer destination and native plans; UInt64/Trim, contiguous prefix/empty parts, NIO/preallocation, progress and joined transactional failure/cancel | `volume_dialog`, `volume_workflow`, `volume_operations`, `combine_names`, `worker_window_progress` |
| P3-05 filesystem comparator/cache | Source CFSFolder::CompareItems and IFolderCompare binding; actual FileTable/CPanel sorting, raw versus visible comments, metadata failure/cache/reload, selection and bytes | `panel_filesystem_sort`, `fs_folder`, `panel_sort`, `panel_archive_sort`, `panel_column_sort`, `filesystem_reload` |
| P3-05 compression method/control states | Source method IDs, Copy/unknown dictionary, empty/Auto controls, TAR memory visibility; Store success/failure/Cancel and exact bytes | `compression_method_state`, `compression_profile_selection`, `source_compress_dialog`, `codec_creation_ui` |
| P3-05 GUI path/history consumers | Source CompressDialog/ExtractDialog Trim, path resolution, shared UString uniqueness; EN/RU Create/list/Extract, SplitDest, Cancel, settings and exact bytes | `dialog_path_rules`, `compression_path`, `extract_dialog`, `split_destination` |
| P3-05 archive directory property projection | Source GetProperty, CalculateSizes and CompareItems; list/Properties/error/retry, native ZIP/7z sort/Refresh/selection and byte preservation | `agent_property_projection`, `native_column_workflow`, `source_properties`, `panel_column_sort` |
| P3-05 Agent provider lifecycle/metadata | Source Close/ReOpen/LoadItems, fresh binding after repair; ZIP/WIM proxy Cancel/retry; source written-count descriptor policy | `agent_lifecycle`, `agent_opening`, `archive_property_catalog` |
| P3-05 source Agent opening/factory | CArchiveFolderManager/CAgent Open and binding; backend reader/catalog and stream lease; S_FALSE diagnostics, progress/Cancel, Extract/Test/error bytes | `agent_opening`, `agent_folder`, `archive_opening`, `archive_open_options`, `archive_property_catalog` |
| P0-01 source inventory | Per-module audit and baseline | `upstream_inventory`, `upstream_baseline` |
| P0-02 independent panels | CApp/CPanel; Qt per-window operation state | `app_state`, `panel_runtime`, `binding_lifecycle` |
| P0-03 availability and dispatch | CApp/WndProc/PanelMenu; QtFileManagerCommands | `native_command_dispatch`, `command_policy_runtime`, `menu_policy` |
| P0-04 address/parent/nested rollback | PanelFolder/PanelOpen; native folder binding | `native_navigation_window`, `panel_navigation`, `ui_navigation`, `nested_archive` |
| P0-05 filesystem/flat enumeration | FSFolder/PanelItems; scanner/model projection | `fs_folder`, `filesystem_scanner`, `flat_view` |
| P0-06 columns/sort/time/refresh | PanelSort/PanelItems; Qt header/viewport adapter | `panel_columns`, `panel_sort`, `panel_auto_refresh`, `view_settings_columns` |
| P0-04/06 native publication failure and open guard handoff | CPanel::OnReload/OpenItem/OpenAsArc_Index; QtFolderProjection and Copy completion | `native_renderer_failure`, `native_renderer_copy`, `panel_item_icons` |
| P0-04/P1-09 native nested stream/memory/disk open, publication failure, confirmed Cancel and retry | Original Agent::GetStream/OpenItemInArchive; Qt native item-order/module lease and deferred guard handoff | `native_nested_stream`, `native_nested_memory`, `native_nested_disk`, `nested_archive`, `nested_edit_chain` |
| P1-08 native external editor update with publication failure and actual writer retry | PanelItemOpen::OnOpenItemChanged; Agent CopyFromFile/UpdateOneFile/CommonUpdate and Qt terminal publication/focus | `native_external_2`, `native_external_301`, `archive_external_edit` |
| P0-07 focus/marks/keyboard/mouse | PanelSelect/PanelKey; shared hover adapter | `panel_key`, `panel_key_source`, `panel_selection`, `list_settings_runtime`, `list_activation` |
| P0-08 two-panel focus/persistence | CApp/Panel; MainWindow projection | `panel_focus`, `panel_runtime`, `window_info`, `ui_copy_same` |
| P0-09 menu/toolbar/context/status | MyLoadMenu/PanelMenu/PanelListNotify; Qt presentation | `main_shell`, `panel_shell`, `native_toolbar_state`, `file_menu_policy` |
| P0-10 progress/pause/background/cancel | CProgressDialog/CProgressSync; process/worker transport | `progress_dialog`, `progress_cancel`, `progress_priority_failure`, `process_pause`, `worker_checkpoint` |
| P0-11 conflict decisions/rollback | OverwriteDialog/AutoRenamePath/FSFolderCopy; transaction | `extract_prompt`, `file_transfer`, `file_transfer_direct` |
| P0-12 selected/root/batch Ask | ExtractGUI/ExtractCallback/PanelCopy; ArchiveBackend path guard | `source_extract_backend`, `source_extract_contract`, `extraction_options`, `ui_operations` |
| P0-12/P3-05 toolbar Extract/Test in both panes, archive Copy To, one/multiple filesystem archives, Cancel, corrupt archive, caption/language/focus | CApp/CPanel ExtractArchives/TestArchives; CompressCall2/ExtractGUI/ExtractDialog; QtShellArchiveServices backend bridge | `toolbar_archive_workflow`, `shell_archive_services`, `language_dialog`, `ui_navigation`, `ui_archive_states`, Windows `native_gui_workflow` |
| P1-01 disk/archive transfer routes | CApp::OnCopy/PanelCopy/Agent; backend snapshot/rollback | `ui_operations`, `ui_copy_same`, `archive_transfer_backup`, `source_update_backend` |
| P1-01 bound archive destination publication and rollback/reopen recovery | CApp::OnCopy/RefreshListCtrl; CAgent::Close and caller-supplied backup close service; physical ReOpen/fallback | `copy_archive_refresh`, `interarchive_recovery`, `archive_transfer_backup`, `ui_operations` |
| P1-01 bound filesystem Copy/Move publication, both panes and deferred lists | CApp::OnCopy/OnReload; retained CFSFolder worker and per-pane scan guards | `ui_copy_routing`, `filesystem_scanner`, `filesystem_reload` |
| P1-02 copy/merge/cross-volume Move | FSFolderCopy/CCopyStateIO; exclusive transaction | `fs_folder_copy`, `file_transfer`, `file_transfer_direct`, `worker_window_progress` |
| P1-03 create/rename/delete/trash/comments | Panel/FSFolder/FileDir; native trash service | `filesystem_operations`, `native_fs_operations`, `ui_operations`, `filesystem_comment` |
| P1-04 archive mutation/publish | ArchiveFolder/AgentOut; ArchiveBackend native/CLI transport | `source_update_backend`, `archive_mutation`, `codec_update`, `ui_archive_states` |
| P1-04 native F2 Rename writer/publication/retry and guarded periodic status | PanelOperations::OnEndLabelEdit; Agent RenameItem/CommonUpdate; Qt rename/timer/status projection | `native_rename_2`, `native_rename_301`, `panel_rename_state`, `panel_processing`, `panel_refresh_completion` |
| P1-05 Add/update/path modes | CompressDialog/UpdateGUI/UpdatePair; Qt controls/backend | `source_compress_dialog`, `shell_update_window`, `ui_update_modes`, `source_update_backend` |
| P1-06 archive volumes | CArchiveLink/CMultiOutStream; leased volume publication | `archive_volume_creation`, `volume_workflow`, `ui_archive_volumes` |
| P1-07 options/memory/deletion lifecycle | COptionsDialog/CompressDialog/Update; Qt control/worker bridge | `compression_advanced`, `compression_memory`, `compression_limit`, `archive_creation_lifecycle` |
| P1-08 external View/Edit/Diff/VerCtrl | PanelItemOpen/EditItem/VerCtrl; OS process and GUI delivery | `archive_external_edit`, `archive_external_lifetime`, `archive_external_security`, `external_editors_ui`, `verctrl` |
| P1-09 nested parent write chain | PanelItemOpen/AgentOut; retained resource/parent publication | `nested_edit_chain`, `nested_update`, `owned_temp_publication` |
| P1-09 changed native Parent: Yes/No/Cancel, actual writer failure/Cancel, ignored publication error, retry and spool cleanup | OpenParentArchiveFolder/CloseOneLevel/OpenParentFolder; original CopyFromFile and non-owning Qt callback publication | `native_parent_publication`, `panel_navigation`, `nested_edit_chain` |
| P1-10 properties/raw/folder size | PropertyName/PropIDUtils/Agent/Panel; typed native property projection, retained native levels/catalog diagnostics | `source_properties`, `native_archive_properties`, `properties_runtime`, `property_formatting`, `folder_size_view` |
| P1-11 root/mount/network/Browse/device image | RootFolder/FSDrives/NetFolder; platform mount service and original device CopyTo | `root_navigation`, `volume_mount`, `network_folder`, `browse_dialog`, `fs_drives_device`, `fs_drives_device_ui`, `native_network_navigation` (Windows) |
| P1-12 Options Apply/Cancel | OptionsDialog/*Page; Qt page container | `options_dialog`, `options_integration`, `options_window`, `fm_settings` |
| P1-13 Edit/Menu policies | EditPage/MenuPage/RegistryContextMenu; OS registration service | `external_editors_ui`, `menu_page`, `context_menu_settings`, `shell_registry` |
| P1-14 language/fallback/focus | CLang/LangUtils/*Dialog; Qt LanguageChange/bindings | `language`, `language_ui`, `language_dialog`, `language_standard_buttons`, `language_focus`, `language_source_update` |
| P1-15 Help/About/temp cleanup | HelpUtils/AboutDialog/TempFiles; CHM viewer and owned leases | `help_source`, `help_ui`, `temp_cleanup`, `owned_temp`, `temp_detached_launch` |
| P1-16 panel/archive drag | PanelDrag/CompressCall2; QDrag/MIME/leased materialization; native bound writer/publication/refusal/retry | `drag_drop`, `drag_archive`, `drag_drop_ui`, `drag_drop_add_ui`, `source_drop_add`, `native_drop_publication` |
| P1-17 ZIP item comment | ArchiveFolder/Zip writer; retained native writer callback | `native_comment`, `native_comment_ui`, `native_zip_writer` |
| P1-18 real decoder RAM request | RAR handler/OpenCallback/ExtractCallback; backend callback bridge | `native_rar_memory`, `native_rar_ui`, `open_callback` |
| P2-01 formats/capabilities/KeepName | LoadCodecs/OpenArchive/Agent; immutable provider leases | `codec_catalog`, `codec_capabilities`, `codec_commands_ui`, `native_open_commands` |
| P2-02 SFX/external modules | WriteSfxModule/LoadCodecs; official module identity/publication | `sfx_creation`, `sfx_lifecycle`, `external_codecs`, `codec_probe` |
| P2-03 CRC/Hash/Test | HashCalc/HashGUI/ExtractGUI; native providers and results view | `native_hash_workflow`, `native_hash_ui`, `hash_results`, `shell_archive_services` |
| P2-04 Benchmark | Bench/BenchmarkDialog; Qt controls and worker lifetime | `native_benchmark`, `benchmark_command`, `benchmark_lifecycle` |
| P2-05 links/junctions/metadata | FileIO/FileDir/Reparse/LinkDialog; native platform bridge | `filesystem_link`, `link_command`, `source_link_storage`, `source_metadata_storage` |
| P2-06 ADS/proxy2 and source zone policy | RootFolder/AltStreamsFolder/AgentProxy/CCopyStateIO/PanelCopy; official engine metadata transport | `alt_streams_folder`, `alt_streams_command`, `alt_streams_archive_ui`, Windows `native_zone_copy`, `source_properties`, `native_update_path` |
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
| CopyFrom into archived ADS preserves main/other streams; directory/failure/Cancel cannot publish | `agent_stream_update` (official WIM writer, all-platform ordinary input and Windows filesystem ADS prefix); `native_update_path` (malformed ADS paths) |
| Archive proxy replacement cannot query old focused rows during peer publication | `agent_stream_copy_ui` (FS/ZIP→WIM ADS, F5/F6/Cancel, Delete Cancel/rollback, marks/focus, both panes/languages); `copy_archive_refresh`; `interarchive_recovery`; `ui_copy_routing` |
| GUI early errors retain source messages/question suppression before work | `gui_error_policy` (missing writer/recovery Cancel, both invalid Extract paths, EN/RU, source flag on/off, unchanged bytes and fitted message/OK JSON); `shell_update_gui`; `source_extract_backend/advanced/contract` |
| Partial Move/overwrite failure cannot silently discard sources | `file_transfer::crossVolumeDeleteFailureReportsPartialMove/directoryMergeCancelRestoresOriginalTree`; `ui_operations::archivePanelsCopyAndMoveThroughStaging`; `interarchive_recovery::rollbackFailureAndRetry` (including native reopen failure/Cancel); `archive_transfer_backup` (including refused close/retry) |
| Device image Copy/Cancel/failure preserves its read-only source | Windows `fs_drives_device::sourceCopyUsesReadOnlyPrivateDevice(cancel/failure/skip/move)`; `fs_drives_device_ui::sourceDeviceNamespaceAndF5` |
| Namespace Hash availability; private device SHA256 success/Cancel/refusal/recovery and ADS digest | `namespace_hash` (all-platform capability guard; Windows native read-only VHD/ADS, EN/RU result geometry and unchanged input) |
| Process startup/error/fragmented output/reset | `archive_backend::missingExecutable/createFailureAndRetry`; `archive_mutation::fragmentedOutputAndError`; `codec_probe` |
| External child/temp lifetime and pending modal close | `archive_external_lifetime`, `archive_external_security`, `temp_detached_launch`, `owned_temp` |
| Window destruction joins workers and rejects obsolete callbacks | `worker_window_progress`, `large_list_window`, `app_lifecycle_window`, `binding_lifecycle`, `archive_preparation` |
| Nested error/wrong-password rollback and read-only refusal | `nested_edit_chain::rejectedInnerPasswordRestoresParent/readOnlyLevelPreventsChainWrites`; `nested_update` |
| Reports omit editable values, including shown passwords | `compression_encryption::methodsAndShowPassword` verifies shown and masked report dumps omit both inputs; `tests/layout_inspection.cpp` report schema; current report/log scan in P3-02 evidence |
| Create source validation order, suppressible errors, unconditional memory/small-volume refusal and real writer recovery | `compression_error_policy` (EN/RU, disabled password controls, unknown-RAM fallback, unchanged settings/input and normal/compact message geometry); `source_archive_browse`, `compression_encryption`, `compression_limit`, `gui_error_policy` |
| Saved compression values restore source nearest/clamped dictionary/order/solid and unavailable-thread behavior | `compression_profile_selection` (EN/RU, Cancel/restart, real accepted Create/list/test/extract and exact bytes); `compression_preferences`, `compression_dictionary`, `compression_order`, `compression_solid`, `compression_resources` |

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
