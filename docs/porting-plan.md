# Полный план портирования Sevenzip Desktop

План задаёт scope, dependencies и размер итерации для порученного портирования.
Общие требования — в [AGENTS.md](../AGENTS.md), текущие статусы — в
[чеклисте](porting-checklist.md), остальные владельцы фактов — в
[карте документации](README.md). Исходный snapshot от 2026-09-28 сохранён
в [исторической инвентаризации](history/porting-plan-baseline.md).

## 1. Сколько сделано и сколько осталось

Единица инвентаризации — upstream `.cpp` вместе с headers/resources/dependencies;
это не процент готовности. Единица приёмки — пакет с постоянным ID.
Текущие counts и статусы ведутся только в
[чеклисте](porting-checklist.md#правила-статусов-и-счётчика).
Физические inputs и disposition всех 109 UI units — в
[source ledger](omitted-scope-audit.md#109-unit-source-receipt-ledger-2026-10-10).

### Единицы подсчёта

[Исходная инвентаризация и её ограничения](history/porting-plan-baseline.md#единицы-подсчёта).

### Уже работает и имеет перенесённую исходную основу

[Исторический снимок](history/porting-plan-baseline.md).

### Уже работает как MVP или интеграционная адаптация

[Исторические границы обвязки](history/porting-plan-baseline.md#уже-работает-как-mvp-или-интеграционная-адаптация).

### Ограничения исходного снимка

[Контекст исходного checkout](history/porting-plan-baseline.md#ограничения-исходного-снимка).

## 2. Приоритеты, зависимости и правила одного чата

| Группа | Важность | Пользовательский результат |
| --- | --- | --- |
| **P0** | Критическая основа и ежедневный UX | Надёжная навигация, исходные команды, две панели, выделение, понятные операции и конфликты |
| **P1** | Основные рабочие сценарии и полноценный UX | Copy/Move между панелями, полные Add/Extract/Edit, настройки, язык, помощь, drag/drop |
| **P2** | Расширенная функциональность и платформенная интеграция | Форматы/кодеки, SFX, benchmark, links/streams, права, shell integration и большие данные |
| **P3** | Приёмка, обновления и распространение | Воспроизводимость, проверки всех ОС, поставка и закрытие реестра |

**UX — критерий каждого шага.** Не переносить функциональный код сегодня,
откладывая его команды, форму, ошибки, отмену и клавиатуру на «полировку».
Пользовательская настройка готова вместе с её применением, сохранением и Cancel.
Проверки платформ, безопасность и provenance тоже выполняются по шагам;
финальные пакеты P3 объединяют доказательства, а не начинают эти проверки.

Один новый чат берёт первый незавершённый **доступный по зависимостям** шаг
минимального приоритета. Внутри группы — порядок ID. Уже закрытые шаги не
переписываются без обнаруженной ошибки. Если требуются части другого пакета,
сначала выделить минимальный prerequisite и зафиксировать его подшагом;
не устраивать скрытую переработку половины приложения.

### Блокировка отдельной функции и продолжение плана

Если часть комплексного результата упирается в подтверждённое ограничение,
записать конкретную функцию, доказательство, prerequisite и критерий повторной
проверки. Проверить по исходным bodies и текущим consumers, кому именно нужна
эта функция. Если остальной результат имеет самостоятельный проверяемый контракт,
вынести блокированную функцию в отдельный ID со статусом `blocked`, сохранить
прежний ID и evidence как историю и принять основной результат только по его
точно указанному, проверенному scope. Счётчик обновить при добавлении пакета.
Обязательность вынесенной функции не исчезает; `blocked` и `N/A` не заменяют
`done`.

Заменить широкие зависимости на конкретную готовую capability там, где
consumer обходится без заблокированной функции. Для остальных сохранить
точный prerequisite. Продолжить первый доступный шаг по приоритету и ID;
не выбирать блокер повторно автоматически. Вернуться к нему при новой capability
или когда отдельная итерация явно берёт работу по снятию prerequisite.
Если оставшаяся часть пакета без функции не имеет самостоятельной приёмки,
оставить parent `partial`/`blocked`, но продолжить другие независимые пакеты.
P3 может собрать матрицу готового scope; полную приёмку нельзя закрыть, пока
обязательная вынесенная функция остаётся заблокированной.

### Размер итерации и коммита (уточнение 2026-10-07)

Пакет — граница результата, не обещание размера задачи. Рабочая итерация
завершает целый интегрированный компонент или пользовательский сценарий:
исходные тела, необходимые зависимости и адаптеры, подключение к текущим
consumers, UX и проверки. Для инфраструктуры результатом может быть полностью
подключённый контракт владельца с проверенными consumers. Перенос отдельного
helper, добавление ресурса, перенос пары методов или запись промежуточной
приёмки сами по себе не являются границей итерации.

Внутри итерации соблюдать [порядок переноса](../AGENTS.md#porting-order-source-comparison-before-runtime-acceptance):
сначала сверить исходники и устранить видимые пропуски выбранного сценария,
затем проверять полноту переноса и работоспособность адаптации.

Перед продолжением сверить последние коммиты, handoff и открытый scope;
объединить связанные остатки в крупные результаты с явным критерием приёмки.
Например, связанные ветки Refresh/Copy/Move, публикация состояния панелей,
ошибки, отмена и их регрессии входят в одну итерацию соответствующего сценария.
Не выделять отдельную итерацию или коммит на каждый guard, callback, метод,
ошибку или тест. Реализацию и проверяющие её регрессии завершать вместе;
возникшие при проверках связанные исправления доводить в той же итерации.
Сохранять прежние commits и evidence, не переписывать историю ради укрупнения.

Если пакет велик, до правок выделить несколько крупных результатов с простыми
ID `P0-10.1`, `.2` и т. п., owners, зависимостями и критериями готовности.
Новые подшаги имеют только один уровень под пакетом; внутренние задачи — обычный
список работ внутри результата, без рекурсивных ID и отдельных коммитов на
каждый пункт. Техническая сложность и новая зависимость требуют уточнения
списка работ, а не автоматического создания очередного уровня подшагов.
Отдельная prerequisite-итерация допустима, когда она даёт самостоятельный
проверяемый результат и разблокирует конкретного consumer; обоснование записать.

Один чат может завершить несколько таких итераций и продолжать весь порученный
план. Ни граница ответа, ни compaction не требуют завершать итерацию или
коммитить промежуточное состояние. Коммит создаётся после законченного результата
и проверок. Focused проверки выполняются по ходу реализации и перед завершением
итерации. Полный gate нужен только по критериям AGENTS.md; граница итерации
или коммита сама по себе его не требует.
Не откладывать обязательные проверки и не объединять несвязанные изменения
ради уменьшения числа коммитов. Родитель закрывается только после всего scope
и проверки сквозного сценария; квоты коммитов и строк кода не устанавливаются.

Работу соседнего пакета учитывать под его собственным ID. Например, общий
ProgressDialog2/modal/worker/cancel относится к P0-10; filesystem enumeration —
к P0-05. Если часть нужна раньше, записать точную prerequisite-связь с текущим
пакетом и объём досрочно выполненной работы. Не вкладывать весь соседний пакет
в цепочку подшагов P0-02 и не закрывать соседний пакет без его полной приёмки.

При перезапуске после прежнего дробления сохранить все historical IDs, commits
и evidence. Сверить текущий код и незакоммиченные изменения; для оставшегося
scope составить несколько крупных результатов с простыми ID и ссылками на
старые открытые строки. Старые строки отметить как входящие в новые результаты,
не объявляя их done. Сначала сделать эту карту, затем продолжать реализацию;
повторно переносить подтверждённый код или переписывать историю Git не требуется.
Для текущего P0-02 отдельно сопоставить оставшиеся owners/lifecycle с P0-04,
P0-05 и P0-10, чтобы зависимости и фактический прогресс имели правильных owners.

Повторяемый порядок выбора, реализации и закрытия шага находится
в [чеклисте](porting-checklist.md#как-выбрать-и-закрыть-шаг);
требования к source port и проверкам — в [AGENTS.md](../AGENTS.md).

Шаг закрывается при доказанном source port его scope, выполненном UX,
актуальных проверках и записанных отклонениях. Файл в зеркальном пути,
проходящий smoke test или disabled action не являются завершённой функцией.
Платформенно неприменимая ветка получает обоснованный статус N/A с доказательством.
Для технически заблокированного сценария статус остаётся partial/blocked,
указываются причина и минимальный следующий шаг; это не считается готовностью.

## 3. P0 — критическая основа и ежедневный UX (12 шагов)

[Scope, зависимости и приёмка](porting-plan/p0.md#3-p0--критическая-основа-и-ежедневный-ux-12-шагов).

### P0-01. Реестр символов, границ и зависимостей

[Scope, зависимости и приёмка](porting-plan/p0.md#p0-01-реестр-символов-границ-и-зависимостей).

### P0-02. Исходные владельцы CApp/CPanel и состояние панелей

[Scope, зависимости и приёмка](porting-plan/p0.md#p0-02-исходные-владельцы-cappcpanel-и-состояние-панелей).

### P0-03. Единая доступность и маршрутизация команд

[Scope, зависимости и приёмка](porting-plan/p0.md#p0-03-единая-доступность-и-маршрутизация-команд).

### P0-04. Переходы, адрес и стек открытых папок

[Scope, зависимости и приёмка](porting-plan/p0.md#p0-04-переходы-адрес-и-стек-открытых-папок).

### P0-05. Файловая модель, перечисление и flat view

[Scope, зависимости и приёмка](porting-plan/p0.md#p0-05-файловая-модель-перечисление-и-flat-view).

### P0-06. Столбцы, сортировка, формат времени и автообновление

[Scope, зависимости и приёмка](porting-plan/p0.md#p0-06-столбцы-сортировка-формат-времени-и-автообновление).

### P0-07. Выделение, клавиши и все общие настройки списка

[Scope, зависимости и приёмка](porting-plan/p0.md#p0-07-выделение-клавиши-и-все-общие-настройки-списка).

### P0-08. Две панели как основной файловый UX

[Scope, зависимости и приёмка](porting-plan/p0.md#p0-08-две-панели-как-основной-файловый-ux).

### P0-09. Меню, toolbar, status bar и контекстное меню 7zFM

[Scope, зависимости и приёмка](porting-plan/p0.md#p0-09-меню-toolbar-status-bar-и-контекстное-меню-7zfm).

### P0-10. Прогресс, сообщения, подтверждение отмены, pause/background

[Scope, зависимости и приёмка](porting-plan/p0.md#p0-10-прогресс-сообщения-подтверждение-отмены-pausebackground).

### P0-11. Общий OverwriteDialog и конфликты Copy/Move

[Scope, зависимости и приёмка](porting-plan/p0.md#p0-11-общий-overwritedialog-и-конфликты-copymove).

### P0-12. Интерактивный Ask при извлечении

[Scope, зависимости и приёмка](porting-plan/p0.md#p0-12-интерактивный-ask-при-извлечении).

## 4. P1 — основные сценарии и полноценный UX (17 шагов)

[Scope, зависимости и приёмка](porting-plan/p1.md#4-p1--основные-сценарии-и-полноценный-ux-17-шагов).

### P1-01. Copy/Move routing между диском, архивом и панелями

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-01-copymove-routing-между-диском-архивом-и-панелями).

### P1-02. Cross-volume move и завершение FSFolderCopy

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-02-cross-volume-move-и-завершение-fsfoldercopy).

### P1-03. Полные Create/Rename/Delete и корзина

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-03-полные-createrenamedelete-и-корзина).

### P1-04. Запись, Rename/Delete/CreateFolder в архиве

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-04-запись-renamedeletecreatefolder-в-архиве).

### P1-05. Полные update/path modes Add to Archive

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-05-полные-updatepath-modes-add-to-archive).

### P1-06. Многотомные архивы и связанные диалоги

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-06-многотомные-архивы-и-связанные-диалоги).

### P1-07. Advanced compression options, время и RAM dialogs

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-07-advanced-compression-options-время-и-ram-dialogs).

### P1-08. Внешние viewer/editor/diff и возврат изменённого файла

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-08-внешние-viewereditordiff-и-возврат-изменённого-файла).

### P1-09. Обратное обновление цепочки вложенных архивов

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-09-обратное-обновление-цепочки-вложенных-архивов).

### P1-10. Полные свойства и исходное форматирование значений

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-10-полные-свойства-и-исходное-форматирование-значений).

### P1-11. Корни, диски, сеть и Browse dialogs

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-11-корни-диски-сеть-и-browse-dialogs).

### P1-12. Полный OptionsDialog, Settings и Folders

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-12-полный-optionsdialog-settings-и-folders).

### P1-13. Страницы Edit и Menu

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-13-страницы-edit-и-menu).

### P1-14. Language и локализация всего интерфейса

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-14-language-и-локализация-всего-интерфейса).

### P1-15. Help/About и управление временными файлами

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-15-helpabout-и-управление-временными-файлами).

### P1-16. Полный drag/drop между панелями и внешними приложениями

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-16-полный-dragdrop-между-панелями-и-внешними-приложениями).

### P1-17. ZIP per-item comment и запись свойства

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-17-zip-per-item-comment-и-запись-свойства).

### P1-18. Реальные decoder RAM prompts

[Scope, зависимости и приёмка](porting-plan/p1.md#p1-18-реальные-decoder-ram-prompts).

## 5. P2 — расширенная функциональность и платформы (11 шагов)

[Scope, зависимости и приёмка](porting-plan/p2.md#5-p2--расширенная-функциональность-и-платформы-11-шагов).

### P2-01. Форматы, codec capabilities и Open Inside */#

[Scope, зависимости и приёмка](porting-plan/p2.md#p2-01-форматы-codec-capabilities-и-open-inside-).

### P2-02. SFX и внешние handlers/plugins

[Scope, зависимости и приёмка](porting-plan/p2.md#p2-02-sfx-и-внешние-handlersplugins).

### P2-03. CRC/Hash workflow и тест архивов

[Scope, зависимости и приёмка](porting-plan/p2.md#p2-03-crchash-workflow-и-тест-архивов).

### P2-04. Benchmark

[Scope, зависимости и приёмка](porting-plan/p2.md#p2-04-benchmark).

### P2-05. Links, junctions и metadata файловой системы

[Scope, зависимости и приёмка](porting-plan/p2.md#p2-05-links-junctions-и-metadata-файловой-системы).

### P2-06. Alternate streams и расширенные archive proxies

[Scope, зависимости и приёмка](porting-plan/p2.md#p2-06-alternate-streams-и-расширенные-archive-proxies).

### P2-07. Оставшиеся Extract modes, права и security metadata

[Scope, зависимости и приёмка](porting-plan/p2.md#p2-07-оставшиеся-extract-modes-права-и-security-metadata).

### P2-08. System page и file associations

[Scope, зависимости и приёмка](porting-plan/p2.md#p2-08-system-page-и-file-associations).

### P2-09. Shell/context integration и завершение Menu page

[Scope, зависимости и приёмка](porting-plan/p2.md#p2-09-shellcontext-integration-и-завершение-menu-page).

### P2-10. Большие списки и операции без блокировки интерфейса

[Scope, зависимости и приёмка](porting-plan/p2.md#p2-10-большие-списки-и-операции-без-блокировки-интерфейса).

### P2-11. Рефакторинг уже написанной обвязки

[Scope, зависимости и приёмка](porting-plan/p2.md#p2-11-рефакторинг-уже-написанной-обвязки).

## 6. P3 — обновляемость, все платформы и приёмка (6 шагов)

[Scope, зависимости и приёмка](porting-plan/p3.md#6-p3--обновляемость-все-платформы-и-приёмка-6-шагов).

### P3-01. Воспроизводимый upstream baseline и процедура обновления

[Scope, зависимости и приёмка](porting-plan/p3.md#p3-01-воспроизводимый-upstream-baseline-и-процедура-обновления).

### P3-02. Сквозная regression и security матрица

[Scope, зависимости и приёмка](porting-plan/p3.md#p3-02-сквозная-regression-и-security-матрица).

### P3-03. CI Windows/macOS/Linux и native проверки

[Scope, зависимости и приёмка](porting-plan/p3.md#p3-03-ci-windowsmacoslinux-и-native-проверки).

### P3-03.1. Нативная проверка drag-out в Finder (macOS)

[Scope, зависимости и приёмка](porting-plan/p3.md#p3-031-нативная-проверка-drag-out-в-finder-macos).

### P3-04. Поставка приложения, движка, ресурсов и лицензий

[Scope, зависимости и приёмка](porting-plan/p3.md#p3-04-поставка-приложения-движка-ресурсов-и-лицензий).

### P3-05. Закрытие omitted modules, branches и интеграционного долга

[Scope, зависимости и приёмка](porting-plan/p3.md#p3-05-закрытие-omitted-modules-branches-и-интеграционного-долга).

### P3-06. Итоговая функциональная и UX приёмка

[Scope, зависимости и приёмка](porting-plan/p3.md#p3-06-итоговая-функциональная-и-ux-приёмка).

## 7. Карта покрытия всех 109 upstream `.cpp`

Это ownership map, не утверждение, что весь файл в группе надо переносить.
Файл может участвовать в нескольких шагах; указан primary owner его оставшегося
scope. P0-01 уточняет symbols/dependencies, P3-05 подтверждает полноту решений.
`.h/.rc` и resources принадлежат тем же owners. Уже перенесённая часть сохраняется.

### FileManager — 67 модулей

| Primary owner | Модули |
| --- | --- |
| P0-02 | `App.cpp`, `Panel.cpp`, `FM.cpp` |
| P0-04 | `PanelFolderChange.cpp`, `OpenCallback.cpp` |
| P0-05 | `FSFolder.cpp` |
| P0-06 | `PanelItems.cpp`, `PanelSort.cpp`, `PanelListNotify.cpp`, `ViewSettings.cpp` |
| P0-07 | `PanelSelect.cpp`, `PanelKey.cpp`, `RegistryUtils.cpp`, `SettingsPage.cpp` |
| P0-09 | `MyLoadMenu.cpp`, `PanelMenu.cpp`, `SysIconUtils.cpp` |
| P0-10 | `ProgressDialog2.cpp`, `MessagesDialog.cpp`, `UpdateCallback100.cpp` |
| P0-11 | `OverwriteDialog.cpp` |
| P0-12 | `ExtractCallback.cpp`, `PasswordDialog.cpp` |
| P1-01 | `PanelCopy.cpp`, `CopyDialog.cpp` |
| P1-02 | `FSFolderCopy.cpp` |
| P1-03 | `PanelOperations.cpp`, `ComboDialog.cpp` |
| P1-06 | `SplitDialog.cpp`, `SplitUtils.cpp`, `PanelSplitFile.cpp` |
| P1-07 | `MemDialog.cpp` |
| P1-08 | `PanelItemOpen.cpp`, `VerCtrl.cpp` |
| P1-10 | `PropertyName.cpp`, `FormatUtils.cpp`, `ListViewDialog.cpp`, `EditDialog.cpp` |
| P1-11 | `RootFolder.cpp`, `FSDrives.cpp`, `NetFolder.cpp`, `BrowseDialog.cpp`, `BrowseDialog2.cpp` |
| P1-12 | `OptionsDialog.cpp`, `FoldersPage.cpp` |
| P1-13 | `EditPage.cpp`, `MenuPage.cpp` |
| P1-14 | `LangPage.cpp`, `LangUtils.cpp`, `TextPairs.cpp`, `StringUtils.cpp` |
| P1-15 | `HelpUtils.cpp`, `AboutDialog.cpp`, `ProgramLocation.cpp` |
| P1-16 | `PanelDrag.cpp`, `EnumFormatEtc.cpp` |
| P1-17 | `PanelOperations.cpp::ChangeComment`, `AgentOut.cpp::CommentItem`, `ArchiveFolderOut.cpp::SetProperty`, `ZipHandlerOut.cpp`, `ZipUpdate.cpp`, `ZipOut.cpp` |
| P1-18 | `Rar5Handler.cpp::CHandler::Extract`, `ExtractCallback.cpp::RequestMemoryUse`, `MemDialog.cpp` |
| P2-01 | `FilePlugins.cpp`, `FileFolderPluginOpen.cpp`, `RegistryPlugins.cpp` |
| P2-03 | `PanelCrc.cpp` |
| P2-05 | `LinkDialog.cpp` |
| P2-06 | `AltStreamsFolder.cpp` |
| P2-08 | `SystemPage.cpp`, `RegistryAssociations.cpp` |
| P3-05 | `ClassDefs.cpp`, `StdAfx.cpp`, `ProgressDialog.cpp` — выяснить build/legacy scope; не автоматический N/A |

### Agent — 7 модулей

| Primary owner | Модули |
| --- | --- |
| P1-10 | `Agent.cpp`, `AgentProxy.cpp`, `ArchiveFolder.cpp` |
| P0-04 | `ArchiveFolderOpen.cpp` |
| P1-04 | `AgentOut.cpp`, `ArchiveFolderOut.cpp`, `UpdateCallbackAgent.cpp` |

### GUI — 10 модулей

| Primary owner | Модули |
| --- | --- |
| P1-05/P1-07 | `CompressDialog.cpp`, `UpdateGUI.cpp` |
| P0-12/P2-07 | `ExtractDialog.cpp`, `ExtractGUI.cpp` |
| P0-10/P1-04 | `UpdateCallbackGUI.cpp`, `UpdateCallbackGUI2.cpp` |
| P2-03 | `HashGUI.cpp` |
| P2-04 | `BenchmarkDialog.cpp` |
| P3-05 | `GUI.cpp`, `StdAfx.cpp` — standalone 7zG/build entry, relevant flow исследовать до решения |

### Common — 25 модулей

| Primary owner | Модули |
| --- | --- |
| P0-04/P2-01 | `ArchiveOpenCallback.cpp`, `OpenArchive.cpp`, `LoadCodecs.cpp`, `DefaultName.cpp`, `ArchiveName.cpp` |
| P0-05/P2-10 | `EnumDirItems.cpp` |
| P0-12/P2-07 | `ArchiveExtractCallback.cpp`, `Extract.cpp`, `ExtractingFilePath.cpp` |
| P1-04/P1-05 | `Update.cpp`, `UpdateAction.cpp`, `UpdateCallback.cpp`, `UpdatePair.cpp`, `UpdateProduce.cpp`, `SetProperties.cpp` |
| P1-07/P1-12 | `ZipRegistry.cpp`, `WorkDir.cpp` |
| P1-08/P1-15 | `TempFiles.cpp` |
| P1-10/P0-06 | `PropIDUtils.cpp`, `SortUtils.cpp` |
| P2-03 | `HashCalc.cpp` |
| P2-04 | `Bench.cpp` |
| P3-05 | `ArchiveCommandLine.cpp`, `CompressCall.cpp`, `CompressCall2.cpp` — исследовать caller/entrypoint scope; relevant bodies имеют owner операции |

### За пределами этих четырёх каталогов

Это открытая dependency closure, не обещание перенести весь upstream:

- `CPP/Common`: уже используемые strings/numbers/wildcards; Lang и необходимые
  portable types. Сохранять upstream containers где применимы; прежние замены
  Qt/std проверить по P0-01, не продолжать их автоматически.
- `CPP/Windows`: FileDir/FileFind/FileIO/FileName/FileLink/TimeUtils/System/
  ProcessUtils/PropVariant и другие реально используемые services; переносимые
  тела копируются, Win32 calls адаптируются узко, GUI control wrappers не
  переносятся механически в Qt.
- `CPP/7zip/Common`: MethodProps/CMemUse, FilePathAutoRename, ProgressUtils,
  Property/stream helpers по фактическим dependencies владельца.
- `CPP/7zip/UI/Explorer`: P2-09; application command logic изучается отдельно
  от COM registration/presentation.
- `CPP/7zip/UI/Console`: reference процессного протокола для ArchiveBackend;
  самостоятельный второй console front end не нужен без отдельной причины.
- `CPP/7zip/Archive`, `Compress`, `Crypto`, `C`, SFX: engine остаётся официальным;
  vendoring only необходимых supporting types/modules или официальной runtime
  зависимости. Не копировать engine в widgets и не писать новый формат.
- `DOC`, `Lang`, Help, icons/bitmaps и licenses: включать с provenance, attribution
  и владельцами P0-09/P1-14/P1-15/P3-04.

## 8. Учёт результата и следующая работа

После каждого чата менять только свою строку/подшаг в чеклисте, добавлять evidence
в audit и обновлять реальные workflow/platform differences в parity. В handoff
указать exact checks, remaining branches, ограничения проверенной ОС и следующий
доступный ID. Не создавать новую конкурирующую документацию о состоянии.
По поручению пользователя от 2026-09-28 каждую завершённую итерацию коммитить
отдельно после проверок, включая только свои изменения. Сохранять чужие hunks,
записывать local commit как evidence; push и публикация не разрешены.

Следующий доступный результат — **P2-06**, ADS/streams и расширенные proxies,
после принятого P2-05. P1-12/P1-13 сохраняют
зависимые platform/shell функции, Finder — самостоятельный mandatory P3-03.1.
Далее по приоритету и
зависимостям. UX идёт вместе с каждым пакетом.
