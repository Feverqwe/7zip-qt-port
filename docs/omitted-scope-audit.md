# Omitted source scope review

This review records module bodies and caller/dependency boundaries for P3-05.
It does not establish whole-module, platform or final acceptance from filenames.
The historical [P0-01 registry](history/p0-01-source-inventory.md) remains intact;
later functional ownership is recorded in the [port audit](upstream-port-audit.md).

Source: [ip7z/7zip](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17),
full revision `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.
The maintained per-module [manifest](upstream-baseline.json) owns reproducible
local provenance; this document does not replace its exact/adapted distinction.

## Missing UI translation units reviewed on 2026-10-05

The original FileManager/Agent/GUI/Common directories contain 67/7/10/25 `.cpp`
files, totaling 109. Current source and vendor paths cover 103 of those names.
The six missing paths below were reviewed in full with their headers and direct
callers. This physical inventory is not a functional coverage measure: remaining
103 bodies, branches and resources still require the P3-05 reconciliation.

| Original path under `CPP/7zip/UI/` | Body and callers | Disposition and dependency boundary |
| --- | --- | --- |
| `FileManager/ProgramLocation.cpp` | Only includes `StdAfx.h`; `ProgramLocation.h` has an empty include guard. FM.mak and GUI/makefile name the object. | No active implementation to port at this revision. Module discovery is supplied by the actual source `Windows/DLL.cpp` owner, not an invented ProgramLocation implementation. |
| `FileManager/RegistryPlugins.cpp` | All old plugin lookup/list bodies are inside comments. Header plugin types/declarations are commented too. | No active plugin registry behavior at this revision. Actual folder/codec discovery belongs to `FilePlugins`, `LoadCodecs` and their accepted consumers; the commented legacy code does not replace those dependencies. |
| `FileManager/StdAfx.cpp` | Only includes `StdAfx.h`. Header includes portable Common.h plus compiler warning policy and explanatory SDK comments. | Upstream precompiled-header build transport, with no runtime body. CMake compiles each source with its required headers/definitions; exact StdAfx/Common/Precomp dependencies remain in the manifest. No claim that compiler macros or platform definitions are unnecessary. |
| `GUI/StdAfx.cpp` | Only includes `StdAfx.h`, whose active contents are warning policy and FileManager/StdAfx.h include. | Same build-transport boundary. GUI.cpp now has production consumers on all supported desktop platforms; omitting this empty PCH translation unit does not omit the GUI dispatcher. |
| `FileManager/EnumFormatEtc.cpp` | `CEnumFormatEtc` owns Next/Skip/Reset/Clone and target-device copies. `PanelDrag.cpp::CDataObject::EnumFormatEtc` supplies one format for DATADIR_GET through `CreateEnumFormatEtc`. | Active Win32 presentation transport, not inactive code. Current source PanelDrag owns data/lifetime/drop policy; `QtPanelDragMime` supplies URLs and `QtPanelDragTransport` supplies QDrag. MIME/native format enumeration is delegated to Qt platform transport, without a second project enumerator. Native receiver proof remains P3-03/P3-06, including blocked Finder evidence; synthetic MIME tests do not close it. |
| `FileManager/ProgressDialog.cpp` | Active legacy CProgressSync pause/stop, progress range/percent, timer, cancellation prompt, external-close/event and thread-join bodies. FM.mak and GUI/makefile instead select ProgressDialog2. SFXSetup/makefile and resource.rc select this legacy dialog. | Explicit installer-bundle boundary: SFXSetup produces `7zS.sfx`, with its separate installer/configuration workflow. The accepted SFX builders select unchanged SFXWin `7z.sfx` on Windows and SFXCon on Unix; SFXWin/resource.rc uses ProgressDialog2. Legacy implementation is not declared inactive or replaced with a homemade equivalent. Custom installer `7zS.sfx` is outside these selected bundles; a future installer feature must reuse its original bodies/dependencies. |

The legacy review includes `ProgressDialog.h`, `ProgressDialogRes.h`, `ProgressDialog.rc` and the
SFXSetup caller/build map. Its class name overlaps the modern progress class;
copying both into one library would create two implementations of the same
symbols. Existing FM/GUI presentation and progress tests apply to ProgressDialog2,
and the original SFX bundle compiles its own selected engine/application sources.

## ClassDefs and native GUID dependencies reviewed on 2026-10-05

Original `FileManager/ClassDefs.cpp` has no runtime algorithm: after StdAfx and
MyWindows it includes MyInitGuid before Agent.h, MyWindowsNew.h and
Explorer/MyExplorerCommand.h. This order selects GUID definitions rather than
external declarations. The adapted upstream-relative ClassDefs remains the
single portable interface owner in `sevenzip_common`: IArchive, ICoder,
IPassword, IFolder, Agent and IFolderArchive retain their original GUID values.
These dependencies are explicit because the adapted Agent header has a narrower
dependency surface. MyInitGuid supplies IID_IUnknown on Unix; Windows uses the
platform libraries as its original comments specify.

The native headers remain exact, live third_party dependencies even though they
are absent from the portable definition unit:

| Dependency and original role | Current caller and definition boundary |
| --- | --- |
| `FileManager/MyWindowsNew.h`: taskbar COM declarations/GUIDs and old SDK fallback | Windows ProgressDialog2.h includes it. CProgressDialog uses CLSID_TaskbarList/IID_ITaskbarList3 in CoCreateInstance and retains its native taskbar operations. Modern MSVC selects system SDK declarations; sevenzip_common publicly links uuid and ole32. Unix selects the existing Qt taskbar adapter. |
| `Explorer/MyExplorerCommand.h`: shell item/command/enumerator declarations and old SDK fallback | Windows ContextMenu.h includes it before the four original COM bases. QueryInterface and EnumSubCommands retain native interfaces. Modern MSVC selects SDK declarations; system GUID definitions come from uuid. Unix excludes Windows COM bases while retaining the source command owner. |
| `Common/MyInitGuid.h`: one definition owner per GUID | The shell DLL's adapted DllExportsExplorer.cpp includes shared interface declarations before MyInitGuid, then refreshes MyWindows' definition macro to define only the project shell CLSID. It does not define a second portable interface set. Its distinct CLSID avoids registering over upstream 7-Zip. |

Current Windows MSVC all-target builds and native progress/shell consumers
provide link/runtime evidence for the selected SDK branch; the port audit's
P3-05.1 records the latest frozen checks and failed whole-gate limits. This review
does not claim execution of old MinGW/old SDK/Windows CE fallback branches or
native taskbar behavior on other operating systems. These retained declarations
do not require a new project GUID implementation. No source/header changes.

## Remaining reconciliation

P3-05 remains open. The selected ClassDefs/native SDK boundary is recorded above;
every claimed full source body and resource,
desktop-versus-CE branch, standalone console/GUI consumer and custom Qt algorithm
must be reconciled against its owning functional audit. The source entrypoint gap
found by this review is accepted in the bounded P3-05.1 caller component.
Engine/Qt packaging, notices and own-code licensing belong to P3-04; final
rendered/native UX acceptance belongs to P3-06.
