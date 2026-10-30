# Обновление 7-Zip до 26.04 — 2026-10-18

Источник: <https://github.com/ip7z/7zip.git>, переход с
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17` (26.03) на
`9128b80e3a471108678e2b3ec3c985861c8d7b0f` (26.04).
Исходная ревизия Desktop — `39cdfa721e97889187dff44cbb0c1d3d2ec72404`;
обновление проверяется в рабочем дереве поверх неё.

## Полнота переноса исходников

Сравнение выполнено в отдельном Git snapshot, до runtime-проверок.
Проверены изменения всех путей из [baseline](../upstream-baseline.json),
их Qt-отклонения, прямые callers и зависимости. Default revision переведена
целиком: неизменённые файлы совпадают с новой ревизией, смешанных overrides нет.
Прежние границы copied/adapted/omitted сохраняются в
[базовом аудите](../upstream-port-audit.md).

Изменённые exact-файлы скопированы побайтно, с исходными notices и лицензией:

- `C/7zVersion.h`, `CpuArch.h`, `XzDec.c`: версия, constant endian conversions,
  защита расчёта памяти многопоточного XZ от переполнения.
- `CPP/7zip/Archive/Zip/{ZipIn.cpp,ZipIn.h,ZipItem.h,ZipUpdate.cpp}`:
  проверки ZIP64 offsets/sizes, чтение комментария и lifetime annotations.
  Сохранён весь исходный reader/update dependency closure.
- `CPP/7zip/Common/{FilterCoder.cpp,LimitedStreams.cpp,LimitedStreams.h,OutMemStream.h}`:
  размер буфера внешних фильтров, устранение повторного seek и annotations.
  `CPP/7zip/Archive/HandlerCont.cpp`, `Compress/LzfseDecoder.h`,
  `Crypto/RandGen.cpp` обновлены вместе с ними.
- `CPP/Common/{AutoPtr.h,Common0.h,IntToString.h,MyCom.h,MyString.h,MyVector.h,`
  `StringConvert.h,StringToInt.h,UTFConvert.cpp}`: исходные контейнеры,
  сортировка без указателя перед началом массива и lifetime annotations.
- `CPP/Windows/{FileMapping.h,MemoryGlobal.h,Menu.h,Synchronization.h,SystemInfo.cpp}`:
  исходные annotations и compiler diagnostics.

Изменённые адаптированные модули перенесены узкими патчами:

- `CPP/7zip/UI/Agent/AgentProxy.{cpp,h}`: `CProxyFile2::IsAux`, чтение `kpidIsAux`,
  исключение AUX при `AddRealIndices_of_ArcItem`/`AddRealIndices_of_Dir`
  с сохранением обхода дочерних записей; исходная коррекция `MY_CPU_LE`.
  Сохранены локальные исправления владения BSTR, deep-tree обхода и памяти.
- `CPP/7zip/UI/Common/Extract.cpp::DecompressArchive`: исходное исключение AUX
  до передачи индексов handler. `ArchiveExtractCallback.cpp::{GetExtractStream,GetStream}`
  и `UI/FileManager/ExtractCallback.cpp::GetStream7`: исходная обработка каталогов
  для hash callbacks, с исключением каталогов из VirtFileSystem.
  Qt path policy, callbacks, cancellation и transport остаются на прежних границах.
- `UI/Common/LoadCodecs.h`: исходный порядок полей и инициализации `CCodecs`,
  с сохранением локального каталога ошибок.
- Annotations перенесены в `Archive/Rar/Rar5Handler.cpp`,
  `UI/Agent/{Agent.cpp,AgentOut.cpp,ArchiveFolderOut.cpp}`,
  `UI/Common/{ArchiveExtractCallback.h,HashCalc.cpp,HashCalc.h,OpenArchive.h,Update.cpp,WorkDir.h}`,
  `UI/Explorer/ContextMenu.cpp`, `UI/FileManager/{App.h,BrowseDialog.cpp,BrowseDialog2.cpp,`
  `FSFolder.h,Panel.h,PanelItemOpen.cpp,ProgressDialog2.h}`,
  `UI/GUI/BenchmarkDialog.cpp`, `CPP/Common/Wildcard.h`,
  `CPP/Windows/{FileDir.h,Shell.h}`.
  Объявление `UI/GUI/CompressDialog.h::COptionsDialog` находится в Qt-заголовке
  `src/dialogs/compress/qtcompressoptionsdialog.h`; оно обновлено там, без второго владельца.

Не применяются изменения удалённых Win32-boundary bodies: casts к `LPARAM`
в MemDialog/SettingsPage заменены типизированными Qt controls; `FM.cpp`
использует QWidget state вместо WinCE `g_Maximized` и не вызывает `Set_Wow64`;
`BrowseDialog2::FindNonHexChar_F` отсутствует в Qt browse workflow.
`CAgent::_progress_ArchiveOpenCallback_for_Open` уже инициализирован в объявлении.
В `UI/GUI/GUI.cpp` нет исходного глобального `g_ExternalCodecs_Ptr`, поэтому
его diagnostic suppression не переносится. Новых функциональных omissions нет.

CLI engine, codec module и SFX собираются целиком из новой pinned ревизии
официальными build scripts. В частности, изменения WIM/NTFS/VHD/ISO/NSIS
получены внутри исходного engine, без самостоятельных локальных handlers.
Соседний оригинальный checkout обновлён fast-forward по запросу пользователя;
исходники в нём не редактировались.

## Ресурсы

Официальный [Windows x64 дистрибутив 26.04](https://github.com/ip7z/7zip/releases/download/26.04/7z2604-x64.exe)
сопоставлен с сохранёнными ресурсами. EN/RU и License совпадают побайтно.
В CHM изменены только `start.htm` и `cmdline/syntax.htm`: версия и якорь
ShortNames. Точные bytes и SHA256 записаны в `resources/7zip/*provenance.json`.
Индексные потоки CHM по-прежнему не включаются. Qt workflow/layout не меняется.

## Runtime-проверки

`upstream_baseline.py verify` до и после переноса: PASS, 415 exact,
198 adapted и 6 local paths. Проверка bytes и inventory не заменяет runtime.
Официальные engine/module/SFX собраны из 26.04; `7zz i` подтверждает новую
версию. Все сборки использовали существующие build trees и compiler caches.

macOS arm64 / Qt 6.12: `./scripts/check.sh` вне sandbox собрал все targets,
прошёл formatting и выполнил 334 suites за 265,15 с: 331 PASS, 3 FAIL.
Полный gate **не зелёный**. Падения — прежние assertions доступности команд:
`ui_operations::stage7ArchiveCopyAndDropCancellation` (Copy),
`ui_archive_states::contextMenuFollowsSelectionAndLocation` (Rename),
`source_context_menu::emptyAndParent` (Create File). Эти же отказы были
зафиксированы до данного обновления; тела тестов и соответствующие command
adapters этой задачей не менялись. Локальный полный лог сохранён в
`build/upstream-update-26.04-check.log`. Первоначальный focused batch
`archive_proxy|native_hash_workflow|native_zip_writer` также прошёл 3/3.

Linux x64 / Qt 6.12: 20/20 focused suites PASS в batch
`20261018T062919Z-82f24449`. Windows x64 / Qt 6.12 в том же batch:
17/20 PASS; `source_extract_advanced` и `source_link_extract` остановились
на создании symlink fixtures без elevated token, `agent_stream_copy_ui`
однократно потерял выделение и ADS destination в строке `0-ru-1`.
Повтор всех трёх suites в elevated interactive desktop session,
batch `20261018T063711Z-18193ef2`: 3/3 PASS. Причина первого symlink-отказа —
неверный режим запуска; первичная потеря ADS-выделения не воспроизвелась,
исправление её причины этим обновлением не заявляется.

Focused scope: native proxy/AUX, hash bundle/workflow/UI, extraction callbacks,
source extraction/links/contracts, ZIP writer, update callbacks, external
codecs/catalog/creation, stream update/copy/ADS, SFX creation/lifecycle и help UI.
На Linux пропущены Windows-only NTFS DACL, filesystem ADS и WIM reparse cases;
они выполнены на Windows, включая elevated повтор link fixtures.
Новые regressions защищают AUX-directory selection с сохранением потомков
и учёт каталогов в hash при filesystem/stream extraction. Остальные изменения
проверены существующим покрытием, без проверок исходного текста или значений версии.

Slow suites не запрошены и исключены. Полный Linux/Windows gate, exhaustive
format corpus и rendered comparison с оригинальным Windows 7zFM не запускались.
Полнота source-переноса и перечисленные runtime-свидетельства имеют разные границы.
До запусков на гостях были только `current`, поэтому старые snapshots не удалялись;
кэши и build trees сохранены, временная локальная распаковка release удалена.
