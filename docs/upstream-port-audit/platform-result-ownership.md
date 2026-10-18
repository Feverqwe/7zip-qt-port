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
