# Archive folder lifetime across rollback — 2026-10-18

Source: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Reviewed `CPP/7zip/UI/Agent/ArchiveFolderOut.cpp::CommonUpdateOperation`,
`Agent.cpp::ReOpen/Close`, `AgentProxy.cpp::CProxyArc2::Load/GetDirPathParts`,
and FileManager `App.cpp::OnCopy`, with the Qt transfer/rollback, reopen,
property transport and panel publication callers. The unchanged borrowed
module baseline remains recorded in `upstream-baseline.json`.

The source saves FolderItem before closing the handler and restores it after
reopening. The Qt interarchive Move rollback must additionally close the physical
reader before replacing the destination file. Its old bridge saved the folder
path after closing. On Windows the proxy can borrow UTF-16 names directly from
the native handler; reading those names after Close intermittently lost the ADS
folder and selection. On platforms with wider wchar_t the proxy copies these
names, which explains why the Windows failure did not reproduce on macOS.

The narrow close/reopen bridge now owns the path parts, ADS flag and folder
prefix before closing. Closed folders reject item access, while their saved path
remains available for reopening and selection restoration. The existing source
path walk and binding branches are retained; no format algorithm, update branch
or dependency is replaced or omitted. Upstream has no separate file-only
rollback bridge to copy. No upstream checkout or resource is changed.

The native folder opening regression now covers Close followed by ReOpen of a
Unicode ADS folder, including its path and contents. The existing peer-stream
Copy/Move scenario covers destination bytes, cancelled source deletion,
selection and subsequent operations. Its setup waits until the panel accepts
interaction; failures identify the operation stage and stop cascading actions.

Focused validation built the application and passed `native_folder_opening`,
`agent_stream_copy_ui`, `interarchive_recovery`, `panel_key_source`, `help_source`,
`help_ui` and `ui_layout` on macOS arm64, Linux x64 and Windows x64 / Qt 6.12.
Windows additionally passed three repeats each of peer-stream Copy/Move and
source keyboard tests. These cover a native Windows keyboard/recovery consumer;
the Help, icon geometry and peer-stream suites use their configured offscreen
platform. No full gate, opt-in slow suite or original Windows visual comparison
was run.

The other recorded failures were test synchronization/geometry assumptions:
Help waits for Qt's scroll-range publication, with a 60-second suite budget after
a measured successful Windows run of about 22 seconds; bookmark input waits for
an enabled list after navigation; icon cells may wrap when two do not fit the
viewport. Label/icon containment and non-overlap remain checked. These changes
do not alter product layout or key dispatch. Icon widget reports now survive
temporary fixture cleanup in the regular UI report directory.
The final `ui_layout` rerun passed on all three platforms after this report-path
change; current compact large-font widget JSON was reviewed on macOS and Windows.
