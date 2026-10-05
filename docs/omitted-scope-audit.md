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

## Remaining reconciliation

P3-05 remains open. ClassDefs portable interface GUIDs and native SDK `uuid`
dependencies need their recorded caller closure, rather than a blanket omission
of MyWindowsNew/MyExplorerCommand. Every claimed full source body and resource,
desktop-versus-CE branch, standalone console/GUI consumer and custom Qt algorithm
must be reconciled against its owning functional audit. The source entrypoint gap
found by this review is tracked as P3-05.1 and is being tested separately.
Engine/Qt packaging, notices and own-code licensing belong to P3-04; final
rendered/native UX acceptance belongs to P3-06.
