# Platform lookup result ownership — 2026-10-18

Source: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Reviewed complete `CPP/Windows/Shell.cpp` and its header, COM/medium/PIDL
dependencies and native drag/context-menu consumers. After applying one common
formatting style, the pre-change Shell bodies match upstream; only includes
differ. The related complete AltStreamsFolder source/header and its panel/copy
callers retain original folder, stream-name, file-handle and progress owners.
No new ownership gap was established in those ADS paths.

The native IDLIST reader freed STRRET_WSTR only after copying and appending its
name. An exception in either allocation skipped CoTaskMemFree. A local
unique_ptr with the same native deleter now owns each returned string before
those operations. Parsing, fallback order, path normalization, errors and
output ordering are retained. No source branch or dependency is omitted.
The caller-owned result contract is documented by
[IShellFolder::GetDisplayNameOf](https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ishellfolder-getdisplaynameof).

The Linux association adapter had the same manual-cleanup gap while appending
revision data: GKeyFile and each returned value could survive an exception.
Both now use the existing local unique_ptr pattern with g_key_file_free/g_free.
[GLib get_value](https://docs.gtk.org/glib/method.KeyFile.get_value.html) transfers
its result to the caller; [KeyFile.free](https://docs.gtk.org/glib/method.KeyFile.free.html)
releases this newly created file object and its contents. The complete GIO
adapter was reviewed, including owned desktop-app/file/key/value/error results,
borrowed icons and comparison of preference revisions. The original
RegistryAssociations bodies operate on Windows registry keys; GIO remains the
necessary Linux platform boundary, with association decisions in the existing
source/services. No association policy or mutation algorithm changed.

Linux launcher/taskbar owners were also reviewed: the adapter requires the GUI
thread; the launcher unregisters its object path before QObject destruction,
and its service watcher has the launcher as parent/context. No new confirmed
lifetime defect was found in those paths.

The exception cleanup gaps are established by source review. Allocation failure
was not injected. The native shell test now additionally resolves an actual
Shell IDList Array from a private Unicode fixture with CF_HDROP unavailable;
the previous real-shell path could complete through HDROP alone. This covers
the modified STRRET-producing branch and its observable result, without
assertions about private symbols or implementation text. Existing association
and private-bus tests cover the Linux consumers. No standalone Qt reproducer,
UI appearance change or visual acceptance is part of this work.

Focused acceptance on Qt 6.12: Windows x64 passed native_shell_data,
native_drag_protocol, native_drag_qt, native_system_menu, alt_streams_folder,
alt_streams_command and alt_streams_archive_ui (7/7, 11.81 s), run
`20261018T130733Z-db3e0eb3`, using the elevated desktop for native link fixtures.
Linux x64 passed association_services, both linux_association_clear variants,
launcher_progress and taskbar_adapter (5/5, 1.07 s), run
`20261018T130737Z-7472c516`. GIO fixtures isolate their desktop/preferences;
launcher fixtures use a private bus. Affected targets were rebuilt with the
existing compiler caches/build trees. These platform-specific changes do not
establish macOS runtime acceptance or a whole-project/full/slow gate.

## Windows error-message buffers

The subsequent complete ErrorMsg.cpp review found the same source gap in both
native FormatMessage branches: LocalFree followed an allocating UString copy.
Each successful API result now has a local unique_ptr using LocalFree before
conversion begins. Source formatting, unknown-code fallback, newline trimming,
translation adapters and platform branches are retained. The original module at
the source revision above has the same manual-release ordering;
[FormatMessageW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessagew)
specifies LocalFree for its allocated result. No allocation failure was injected
and no synthetic assertion-only test was added; existing public error consumers
already exercise native and unknown errors.

Windows x64 / Qt 6.12 rebuilt targets and passed native_services,
native_language_host, language, progress_sync, progress_worker and
native_shell_com (6/6, 6.21 s), run `20261018T132010Z-1d877580`, with its existing
build/cache. The modified branches are Windows-only; this receipt does not add
macOS/Linux runtime or global application acceptance.

## macOS association completion owner

Reviewed the complete native association bridge, platform adapter, native header,
SystemPage Apply consumer and Options Apply/reentry guards. The source
RegistryAssociations module at the revision above uses synchronous Windows
registry services; it has no reusable AppKit completion implementation. The
macOS bridge is the required platform boundary; association decisions remain
in the existing services and source page.

The native completion block retains a shared QEventLoop. Its final captured
owner can be released after the GUI caller returns; the previous default
deleter therefore depended on the native block's release thread. The local
deleter now calls deleteLater(), preserving shared lifetime while scheduling
QObject destruction in its owning thread. Qt documents both the prohibition
on direct deletion from another thread and the thread-safe deferred alternative
in [QObject](https://doc.qt.io/qt-6/qobject.html#deleteLater).
Result publication, queued quit, early completion, native errors and association
selection are unchanged. No source branch or dependency was omitted.

macOS / Qt 6.12 rebuilt the affected library and passed association_services,
options_dialog, options_integration and options_keyboard (4/4, 1.70 s) using
the existing incremental build/cache. These tests exercise isolated association
services and their Options consumers; they do not establish runtime acceptance
of AppKit's mutation callback or reproduce a native crash. No real user defaults
were changed, no allocation failure was injected and no standalone Qt reproducer
was created. The change is macOS-only; Linux/Windows and full/slow gates were
not run for this iteration.
