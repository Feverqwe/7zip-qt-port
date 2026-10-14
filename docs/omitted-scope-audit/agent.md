# Omitted source scope review — agent

[Индекс](../omitted-scope-audit.md). Scope и свидетельства ниже сохраняют даты и ревизии; они не подтверждают текущий checkout.

### Agent opening/factory body and caller review, 2026-10-07

The original Agent::Open and ArchiveFolderOpen::OpenFolderFile active bodies are
now on the production QtArchiveFolderManager route. FileFolderPluginOpen's
worker still owns modal progress and join; the source factory supplies callback
QI, source Agent lifetime, diagnostic S_FALSE publication and root-binding
progress scope. IInFolderArchive Extract/property interface bodies are restored,
with native/process services confined to ArchiveBackend. Original commented
FindFormat, FolderReOpen and property enumeration remain inactive. The exact
source, adaptations and current validation are recorded in the
[opening audit](../upstream-port-audit.md#p3-05--source-agent-opening-and-archive-folder-factory-2026-10-07).
The subsequent lifecycle component restores LoadItems' original link-open
short-circuit and ReadItems' allocation/RAM/descriptor/load ordering. Written
counts are consumed as upstream in ReadItems/GetPropertyInfo; unwritten counts
initialize to zero. Partial-proxy cleanup remains a documented joined-worker
retry adaptation across descriptor/load errors, Cancel and exceptions.
Real ZIP/WIM Close/failed-ReOpen/repair/rebind and factory cancellation consumers
are covered in the [lifecycle audit](../upstream-port-audit.md#p3-05--source-agent-provider-lifecycle-and-metadata-binding-2026-10-07).
Remaining Agent update/folder bodies still require full-module disposition;
these bounded components do not close the 109-module review or native/final acceptance.

The complete GetProperty/GetFolderProperty and path/tree comparator review also
found the missing path-proxy directory CRC branch. It is restored from the same
baseline, reusing source proxy aggregates and exact explicit-record/error/local
variant policy. Direct property/formatter/Properties and native ZIP/7z list,
sort/Refresh/selection consumers are recorded in the
[directory-property audit](../upstream-port-audit.md#p3-05--source-archive-directory-property-projection-2026-10-07).
This disposition covers these property bodies, not remaining Agent update,
extraction/index/raw-property modules or full-module acceptance.

Source PanelListNotify::OnNotifyActivateItems is now retained with explicit Qt
event modifiers. The owning table/icon Return/double-click adapters replace the
native ComCtl32 notification-version switch and share the existing OpenNative
state/history projection. Parent/multiple-item behavior stays in OpenSelectedItems;
The complete notification/status/paint caller mapping is now recorded in the
[PanelListNotify disposition](../upstream-port-audit.md#p3-05--panellistnotify-complete-body-and-caller-disposition-2026-10-10).
Physical native input/rendered/platform acceptance remains open.
See [dated consumer evidence](../upstream-port-audit.md#source-item-activation-notification-consumers-2026-10-07).

### Agent update enumeration review, 2026-10-07

Complete original/current AgentOut and its declarations/callers exposed an active
omission: `DoOperation` replaced the original alternate-stream item loop with
E_NOTIMPL, and removed the Windows filesystem-colon-prefix guard. Existing
DirItem/FileFind/EnumDirItems and UpdateCallback already support that Windows
branch. It is now restored, with a narrow Unix callback metadata adapter because
the source FileInfoBase flag is Windows-only. Public source CopyFrom transactions
use the official WIM writer and preserve source directory refusal, failure and
cancellation behavior; see the
[bounded source audit](../upstream-port-audit.md#p3-05--source-agent-alternate-stream-updates-2026-10-07).
The seven Agent bodies and associated headers were compared; the following
disposition accounts for wrappers, empty-agent/FAR versus desktop entrypoints
and dependency ownership. Native desktop and final whole-project acceptance
remain open. An absent platform member is not sufficient evidence to omit
supported archived ADS behavior.

### Agent desktop body/header/dependency disposition, 2026-10-07

Compared the seven original/adapted translation units and Agent.h, AgentProxy.h,
IFolderArchive.h and UpdateCallbackAgent.h at the pinned revision. Original
FileManager/FM.mak lists all seven in AGENT_OBJS; 7zip.mak resolves these objects
to UI/Agent source paths. Far/makefile selects the shared six bodies without
ArchiveFolderOpen, establishing a separate consumer boundary rather than making
the archive factory unused. The following is source disposition, not native
desktop/rendered or complete109-module acceptance.

| Module under `CPP/7zip/UI/Agent/` | Active source bodies and current boundary |
| --- | --- |
| `Agent.cpp` | Folder enumeration/flat mapping, property catalogs, path/tree comparators, binding/parent/ADS, raw properties, real-index expansion, stream/extract, Agent open/reopen/close/read/bind and archive-level properties retain source algorithms. Current GetRealIndex is reused by raw projection instead of duplicating its original mapping. Typed engine metadata/open/extract services, public range guards, initialized failure outputs and partial-proxy retry cleanup are explicit adapters. Opening/lifecycle/CRC audits own the identified corrections and their consumers. Original commented EnumProperties stays inactive. |
| `AgentOut.cpp` | SetFolder/SetFiles/SetProperties, archive enumeration, DoOperation planning, CommonUpdate, Delete/CreateFolder/Rename/Comment/UpdateOneFile use source bodies. GUI pathmode, filesystem links/metadata choices, input lifecycle/mtime and the Unix archived-ADS flag are typed integrations with original common update services. Property-value storage has scoped ownership. Original Windows ADS normalization/item marking and directory refusal are restored. FAR output API/empty-agent creation are distinguished below. |
| `AgentProxy.cpp` | Both path and parent-index proxies retain source load, names, directory/ADS mapping, aggregates and ordering. The existing BSTR-lifetime correction moves the local PropVariant outside its branch so its pointer survives until AllocStringAndCopy; it does not replace the algorithm. Headers retain source proxy types. |
| `ArchiveFolder.cpp` | Zone mode/buffer and CopyTo bodies are retained, including original move refusal, callback QI, normal/flat pathmode choice, overwrite mode and Extract forwarding. The application composes Move through the existing source Copy/Delete transaction; it does not add a second Agent Move implementation. Commented SetReplaceAltStreamCharsMode remains inactive. |
| `ArchiveFolderOpen.cpp` | Codec/icon loading, extension/icon lookup and OpenFolderFile retain source decisions, S_FALSE diagnostics and binding progress/lifetime. A retained catalog/open service replaces process-global codec ownership. QtArchiveFolderManager exposes IFolderManager and delegates to this source factory. Windows uses resource modules; Unix uses the compiled Format7zF string table. Commented FindFormat/FolderReOpen/GetTypes/CreateFolderFile stay inactive. |
| `ArchiveFolderOut.cpp` | Original transaction, operation switch, prefix/tail stream, temporary publication, Move callbacks/reopen, processed-source deletion, saved path/ADS folder restoration and error paths are retained. Unix nofollow directory service, owned-temp adoption, output mtime, deleteInputs and GUI action/pathmode bridges are documented adapters. Original unsupported CreateFile has its sole identical E_NOTIMPL body in qtlocationfolder.cpp; no working CreateFile behavior was omitted or invented. |
| `UpdateCallbackAgent.cpp` | Original progress/scan/update/error/stream callbacks and delegation are retained; only include ownership and portable UInt64 spelling differ. Native/process progress transport supplies services through the retained callback interfaces. |

The Agent.h review also accounts for original constructor/default state moved to
member initializers, inline bodies moved out of line, retained folder interfaces
and capability-gated Qt operations, and the new private engine/open-service
lifetime. IFolderArchive.h retains original interface GUIDs/signatures, including
the unused-by-desktop IOutFolderArchive declaration. App-only
IArchiveFolderInternal adds GUI action/pathmode methods after GetAgentFolder;
official codec ABI is kept on IInArchive/IOutArchive, not this Qt UI interface.
No binary compatibility with a separately built original FAR/UI plugin is claimed.

Original IOutFolderArchive is queried by Far/PluginWrite.cpp and PluginDelete.cpp.
PluginWrite's create path constructs an empty CAgent, sets files, and supplies a
codec/index to DoOperation. Agent::CanUpdate explicitly documents this FAR use.
Original DoOperation2 only delegates to DoOperation with g_CodecsObj/-1; the
desktop CommonUpdateOperation adapter calls the same body directly, capturing
requested/processed paths for the accepted input-lifecycle service.
GetErrorMessage's only external original caller is Far.cpp::OpenPlugin.
These omitted FAR-facing wrappers and empty-agent writer creation are not claimed
as implemented APIs. The desktop Add/create owner remains the separate original
CompressCall→GUI/UpdateGUI→Common/UpdateArchive route; restoring a parallel create
algorithm in Agent is not evidence of that desktop workflow. Final whole-project
scope/entrypoint acceptance must retain this distinction explicitly.

Dependencies are the original portable strings/variants/COM and archive
interfaces, OpenArchive/LoadCodecs, ArchiveExtractCallback/Extract, DirItem/
EnumDirItems, UpdatePair/UpdateProduce/UpdateCallback, file streams/CopyCoder,
WorkDir/file services, source folder/progress interfaces and the existing Qt
backend/native-codec resource adapters. CMake has one source owner for each
Agent module; the manifest pins each adapted/exact dependency. Full dependency
and source-language/license dispositions for these common owners remain in
their own audits, not inferred from Agent's successful build.

The new real F5/F6 consumer additionally found a Qt publication omission after
Agent updates: old rows queried a rebuilt proxy during destination activation.
Original OnCopy's notification guard is restored at this switch for FS→archive,
archive→archive and rollback. Its
[source audit](../upstream-port-audit.md#p3-05--source-copymove-publication-after-agent-updates-2026-10-07)
owns actual RED, saved-state/rollback/byte/geometry and platform checks. Other
module groups, native desktop input, rendered references and packaging remain
separate required P3 scope.
