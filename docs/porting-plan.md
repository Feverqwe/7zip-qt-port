# Полный план портирования Sevenzip Desktop

Дата исходной инвентаризации: **2026-09-28**. Рабочий baseline проекта:
`8899ffbfc0063e6b053306478df9060907977c5c` **плюс незакоммиченные изменения**.
Baseline upstream: [ip7z/7zip, 0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
План относится к коду этого checkout; новые изменения требуют уточнения статусов.
К завершению подготовки документов HEAD стал
`21b775c035eb1464fae725608307f2ddc5ad8685` (shared list activation/hover);
начальные изменения этой области уже закоммичены другой задачей.

Цель — перенести исходную реализацию 7zFM в C++17 / Qt 6 Widgets с сохранением
пользовательских сценариев Windows 7-Zip File Manager на Windows, macOS и Linux.
Это включает функциональный код внутри UI-файлов, а не только диалоги.
Официальный `7zz` остаётся движком за `ArchiveBackend`; повторная реализация
архивных форматов, компрессоров и криптографии в этот план не входит.

Соседние документы:

- [Чеклист выполнения и статусы шагов](porting-checklist.md).
- [Сообщение для каждого нового чата](porting-chat-prompt.md).
- [Происхождение и границы уже перенесённого кода](upstream-port-audit.md).
- [Сценарии, ограничения и отличия от 7zFM](7zfm-parity.md).
- [Владельцы компонентов](architecture.md), [тесты](testing.md),
  [геометрия UI](ui-layout-contracts.md), [сверка ресурсов](ui-reference.md).

План задаёт порядок работы; audit хранит доказательства source port, parity —
наблюдаемое поведение. Чеклист хранит текущий прогресс. При противоречии сначала
проверяются код и upstream, затем исправляются документы. Этот файл не заменяет
`AGENTS.md` и не даёт разрешения публиковать, развёртывать или менять upstream.

## 1. Сколько сделано и сколько осталось

### Единицы подсчёта

**Модуль инвентаризации** — один upstream `.cpp` в четырёх каталогах ниже.
Его `.h`, `.rc`, ресурсы и зависимости рассматриваются вместе с ним, но не
увеличивают этот счётчик. Это воспроизводимый подсчёт файлов, а не процент
завершённости продукта или оценка трудозатрат.

| Область upstream | Всего `.cpp` | Есть соответствующий `.cpp` в `src/` | Нет соответствующего файла в `src/` |
| --- | ---: | ---: | ---: |
| `CPP/7zip/UI/FileManager` | 67 | 29 | 38 |
| `CPP/7zip/UI/Agent` | 7 | 2 | 5 |
| `CPP/7zip/UI/GUI` | 10 | 2 | 8 |
| `CPP/7zip/UI/Common` | 25 | 2 | 23 |
| **Всего** | **109** | **35** | **74** |

Текущие counts включают частичный Panel.cpp из P0-02.2a и ClassDefs.cpp из
P0-02.2b3a, partial FSDrives.cpp из P0-02.2b3b2b3 и PropertyName.cpp
из P0-02.2b3b3c2b2b, полный FormatUtils.cpp из P0-02.2b3c3b1,
partial ProgressDialog2.cpp (complete sync + presentation/taskbar/priority subset;
full modal/thread/cancel remains mandatory)
из P0-02.2b3c3b2a2/b2b2,
helper-only OpenArchive.cpp/ExtractCallback.cpp
из P0-02.2b3b3c2b2c;
P0-01 inventory зафиксировал прежний snapshot 27 mirrored/82 missing.
Все 35 совпадения путей
требуют проверки пропущенных функций/веток. Нельзя считать их полностью
завершёнными модулями. Из 74 отсутствующих файлов часть
содержит необходимую логику, часть обслуживает Win32 presentation или запуск
отдельной 7zG/console-программы. Они должны получить явное решение о переносе,
адаптации или неприменимости, а не автоматически копироваться целиком.

Дополнительно есть 7 зеркальных `.cpp` вне этой таблицы:
`CPP/Common/Wildcard.cpp`, `CPP/Windows/FileDir.cpp`,
`CPP/Windows/FileName.cpp`, `CPP/Windows/FileLink.cpp`, `CPP/Windows/DLL.cpp`, `CPP/Windows/ResourceString.cpp`,
`CPP/7zip/Common/MethodProps.cpp`.
Всего в `src/CPP` — 42 `.cpp`.
В корне `src/` — 42 `.cpp` Qt/integration; это не 42 неперенесённых компонентов:
часть является необходимой обвязкой, часть содержит миграционный долг.
Отдельно есть Apple-only Objective-C++ `qttaskbarprogress_mac.mm`: narrow native
Dock service с проверенными source state/value/cleanup вызовами; полный modal
workflow и rendered/platform acceptance остаются обязательными.
В `third_party/7zip` — 84 файла, побайтово совпадающих с теми же путями baseline,
плюс локальный README и License.txt, скопированный из upstream `DOC/License.txt`.
Это зависимости и ресурсы, не 84 завершённых пользовательских функций.
В таблице учитываются только файлы `src/`: переносимый `UI/Common/PropIDUtils.cpp`
собирается без изменений из `third_party/7zip` и входит в счётчик зависимостей.
Его дальнейшая runtime-интеграция и Windows-проверки остаются обязательными;
создавать вторую реализацию под `src/` не требуется.

**Оставшуюся работу планируем как 44 пакета**: P0 — 12, P1 — 16,
P2 — 10, P3 — 6. Каждый пакет имеет постоянный ID и отдельный результат.
Часть пакетов завершает частичный перенос; часть добавляет отсутствующие
модули; P3 закрывает обновляемость, проверки и поставку. Это не утверждение,
что осталось ровно 44 исходных класса или 44 чата: большой пакет можно разделить
на подшаги, сохранив родительский ID. Первоначально закрытых пакетов — 0/44:
это счётчик оставшейся работы, уже работающий MVP не обнуляется.

Текущий счёт после P0-08.2 (2026-09-30): **8/44 закрыты, 36 остаётся**;
P0: 8/12, P1: 0/16, P2: 0/10, P3: 0/6. Остальные mandatory workflow
owners сохраняются в чеклисте.

Нельзя честно вывести процент полного source port из числа файлов: размеры,
зависимости и объём частичных портов различаются. P0-01.1–.4 в audit содержит
реестр copied/adapted/omitted scope всех 109 UI и трёх supporting modules. Новые зависимости
добавляются в audit и к владельцу пакета, а не скрываются за прежним счётчиком.

### Уже работает и имеет перенесённую исходную основу

Области ниже описывают исходную основу и остаток обязательных веток. Точные
результаты новых проверок и статус каждого пакета находятся в чеклисте;
исторический результат теста не подтверждает изменённый checkout.

| Область | Сделано | Что не следует считать завершённым |
| --- | --- | --- |
| Архивная модель | `CProxyArc`/`CProxyDir`, дерево явных и неявных папок, размеры, `CAgentFolder`, flat traversal, раскрытие выбора в индексы | Полные Agent/ArchiveFolder, CRC/raw properties, CProxyArc2 и streams |
| Сортировка | Исходное сравнение имён, папки перед файлами, числовые имена, направление, flat tie-breaks | Полный набор свойств и состояния двух панелей |
| Выделение | `SelectSpec`, `SelectByType`, `SelectAll`, `InvertSelection`, wildcard helper, исходная модель отметок, мышь/диапазоны и клавиши P0-07 | Проверка Windows/Linux runtime и интеграция с ещё не перенесёнными командами |
| История/избранное | `CFolderHistory`, `CFastFolders`, команды истории/закладок, сохранение через INI | Полный CAppState и навигационный стек CPanel |
| Общие диалоги | `CComboDialog`, `CCopyDialog`, `CSplitDialog`, `CPasswordDialog`, `CListViewDialog`, `CEditDialog` и часть callers | Локализация всех форм, полные callers, непренесённые Browse/Options/Progress |
| Свойства | Сводка выбранного, доступные snapshot properties, подробное значение и clipboard | Raw/format-specific properties, общий исходный pipeline свойств |
| Файловые операции | `CFSFolder` create/rename/delete, recursive removal, часть `CCopyState`/`FSFolderCopy` | Полный workflow панели, overwrite callbacks, recycle bin, cross-volume move |
| Split/Combine | Parser размеров, presets, последовательность томов, циклы переноса, подготовка форм | Полная оркестрация CApp и единый progress/callback |
| Compress | Таблицы уровней/методов, словари, порядок/слово, solid, потоки, RAM, путь/история, password/encryption, profiles | Полный OnInit/OnOK, update/path modes, volumes, SFX, advanced options и codecs |
| Extract | История 16 путей, split destination, часть path modes, 5 overwrite modes, password/show | Интерактивный Ask, absolute paths, ElimDup, security/metadata |
| Settings | `CFmSettings`, real icons/grid/single click, часть `CSettingsPage` | Полные 6 страниц Options и остальные поля |
| Portable dependencies | Строки/числа/типы и другие файлы из upstream в `third_party` | Необходимые зависимости будущих портов ещё выявляются |

### Уже работает как MVP или интеграционная адаптация

- Две независимые панели F9/Tab/Alt+F1/F2, disk/archive и archive/archive
  (P0-08), четыре режима списка, файловая и архивная навигация, плоский вид,
  адресная строка, menus/toolbar/context menu и shortcuts. Реальные
  межпанельные операции остаются P1-01.
- Чтение вложенных архивов, открытие по содержимому по явной команде,
  временные файлы и возврат к родительскому архиву.
- Создание/изменение `.7z`/`.zip`, извлечение, тест и хеширование через `7zz`;
  staged archive replacement, отмена и журнал, проверки опасных путей.
- F3/F4 в собственном ограниченном текстовом редакторе; обновление файла
  незашифрованного верхнего архива. Системное открытие извлечённых файлов.
- Qt workers для transfer/delete/volumes, временные результаты и откат;
  filesystem enumeration порциями на GUI thread.
- Геометрические тесты/JSON, извлечение upstream UI resources и часть
  resource-to-widget contracts, CMake/CTest и `scripts/check.sh`.

Необходимая Qt/process обвязка останется. Свою функциональную реализацию
навигации, команд и операций надо заменить upstream bodies в соответствующих
владельцах, сохранив ограничения безопасности и явно документируя различия.

### Ограничения исходного снимка

В начале инвентаризации незакоммиченные изменения затрагивали `CMakeLists.txt`, list/settings
модули, `qtlistview.*`, `mainwindow_preferences.cpp`, несколько документов
и `tests/list_activation_tests.cpp`. Audit описывает single-click/hover общего
списка; основной table/icon view ещё не получил hover/track-select. План учитывает
этот код, но не утверждает, что его проверки выполнены в этой задаче.
Нельзя сбрасывать, присваивать себе или молча коммитить эти изменения.

В checkout отсутствует `.github/`: триплатформенная CI пока является работой
P3-03. Наличие проверок macOS в истории не подтверждает Windows/Linux или
визуальную идентичность с отрисованным Windows 7zFM.

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

### Размер итерации и коммита (уточнение 2026-09-29)

Пакет — граница результата, не обещание размера задачи. Рабочая итерация
завершает целый интегрированный компонент или пользовательский сценарий:
исходные тела, необходимые зависимости и адаптеры, подключение к текущим
consumers, UX и проверки. Для инфраструктуры результатом может быть полностью
подключённый контракт владельца с проверенными consumers. Перенос отдельного
helper, добавление ресурса, перенос пары методов или запись промежуточной
приёмки сами по себе не являются границей итерации.

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
и проверок. Focused проверки выполняются по ходу реализации; полный gate —
перед завершением итерации согласно AGENTS.md, а не после каждого helper.
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

Для каждого шага обязательно:

1. Проверить рабочее дерево, чеклист, relevant memory и текущий код.
2. Прочитать исходные bodies, headers, ресурсы и dependencies из точного baseline.
3. Скопировать функциональную основу с путями/именами/notice, локализовать
   Qt/platform/7zz adapters; не оправдывать новый алгоритм словом «COM».
4. Реализовать и проверить целый пользовательский сценарий этого шага.
5. Добавить regression, который падает без изменения; для операций — success,
   failure/cancel, Unicode/пробелы и relevant malicious paths.
6. Собрать affected targets, проверить direct consumers; для UI выполнить
   text specification, normal/compact geometry и review current-build JSON.
7. Для code/tests/build changes выполнить `./scripts/check.sh` по `AGENTS.md`;
   для документации — `git diff --check`. Обновить audit/parity и другие
   документы, чьи утверждения изменились, затем статус и handoff.

Шаг закрывается при доказанном source port его scope, выполненном UX,
актуальных проверках и записанных отклонениях. Файл в зеркальном пути,
проходящий smoke test или disabled action не являются завершённой функцией.
Платформенно неприменимая ветка получает обоснованный статус N/A с доказательством.
Для технически заблокированного сценария статус остаётся partial/blocked,
указываются причина и минимальный следующий шаг; это не считается готовностью.

## 3. P0 — критическая основа и ежедневный UX (12 шагов)

Сокращения источников: **FM** = `CPP/7zip/UI/FileManager`, **Agent** =
`CPP/7zip/UI/Agent`, **GUI** = `CPP/7zip/UI/GUI`, **Common** =
`CPP/7zip/UI/Common`. Все пути относятся к указанному upstream commit.
Перечень файлов ниже задаёт точки исследования; точный scope символов агент
фиксирует после чтения соответствующей реализации.

### P0-01. Реестр символов, границ и зависимостей

**Зависимости:** нет. **Статус:** done, 2026-09-28; audit P0-01.1–.4,
112 unique source paths, all owners/decisions, 33 vendor byte matches и
docs-only checks macOS. Функциональные порты и native acceptance остаются открыты.

Сверить 27 mirrored UI-модулей и 3 остальных mirrored `.cpp` с baseline.
В audit добавить для каждого перечень copied/adapted/omitted symbols,
зависимости, исходную ревизию и причины локальных замен типов/контейнеров.
Для 82 отсутствующих `.cpp` зафиксировать владельца из карты в разделе 7
и решение: нужен целиком, частичный scope, process/platform adapter или N/A.
Не требуется переписывать эти 82 файла в первом чате.

**Готово:** каждый модуль инвентаризации имеет owner и понятный scope;
выявлены нужные portable types/adapters без второго authoritative алгоритма;
уточнены пакеты и счётчик. `git diff --check`; отсутствие runtime-изменений
не требует app build. В новых шагах читать тела повторно по своей области.

### P0-02. Исходные владельцы CApp/CPanel и состояние панелей

**После:** P0-01. **Источники:** FM `App.cpp/.h`, `AppState.h`, `Panel.cpp/.h`,
`FM.cpp`; локальные `MainWindow`, `mainwindow_panel.cpp`, `mainwindow.cpp`.

Перенести необходимые функциональные тела CApp/CPanel, lifecycle и состояние
местоположения/фокуса/selected state. Вывести эти решения из MainWindow в
исходных владельцев; Qt окно остаётся presentation и signal adapter.
Сохранить уже работающие selection/history/backend interfaces. Подготовить
независимые экземпляры состояния панели для P0-08, без глобального singleton.

**Готово:** одна панель работает через новые owners без регрессии; переход,
обновление, начало/ошибка/отмена операции и закрытие окна сохраняют корректное
состояние. Обновлена architecture; consumer tests навигации/команд/выделения.

### P0-03. Единая доступность и маршрутизация команд

**После:** P0-02. **Источники:** FM `PanelMenu.cpp`, `MyLoadMenu.cpp`,
`PanelOperations.cpp`, `PanelCopy.cpp`, `PanelKey.cpp`, `App.cpp`.

Перенести исходные проверки capabilities folder/selection/active panel.
Заменить собственные ветки `refreshActions` и дубли menu/context/keyboard
одним источником правил. Отдельно учитывать read-only/nested/encrypted archive,
busy/loading state, выбранную папку, пустую панель и возможности backend.
Неперенесённая операция остаётся disabled с документированной причиной.

**Готово:** меню, toolbar, context menu и клавиши вызывают один command owner;
отключённую команду невозможно выполнить shortcut. Тестирована матрица состояний
диск/архив/вложенный архив/операция, focused item и selection.

### P0-04. Переходы, адрес и стек открытых папок

**После:** P0-02, P0-03. **Источники:** FM `PanelFolderChange.cpp`,
`PanelItemOpen.cpp`, `OpenCallback.cpp`; Agent `ArchiveFolderOpen.cpp`.

Перенести BindToPath/BindToPathAndRefresh, parent/root/open/close-level flow,
восстановление выделения и виртуальные пути. Qt адрес и async backend адаптируют
исходный flow вместо собственного навигационного алгоритма. Сохранить историю,
закладки, исключение temp paths и открытие по содержимому.

**Готово:** Enter/Backspace/Up/root/address ведут одинаково; переход в nested
archive и назад восстанавливает folder/focus; ошибка, неверный пароль и отмена
не оставляют полуприменённое состояние. Unicode, relative/root paths и tar.gz.

### P0-05. Файловая модель, перечисление и flat view

**После:** P0-02, P0-04. **Источники:** FM `FSFolder.cpp/.h`, `PanelItems.cpp`,
`Panel.cpp`; при необходимости Common `EnumDirItems.cpp`.

Перенести LoadSubItems и relevant enumeration/flat logic, filters и property
accessors. `FilesystemScanner` сохраняется как узкий scheduling/cancellation
adapter при необходимости; его независимая функциональная логика заменяется.
Не блокировать UI большими перечислениями и не следовать циклическим ссылкам.

**Готово:** hidden/empty/Unicode entries, inaccessible folders, flat prefixes,
stale batch после перехода и отмена соответствуют source scope. Проверить
публичный результат модели и отзывчивость на детерминированном большом fixture.

### P0-06. Столбцы, сортировка, формат времени и автообновление

**После:** P0-05. **Источники:** FM `PanelItems.cpp`, `PanelSort.cpp`,
`PanelListNotify.cpp`, `ViewSettings.cpp`, `FormatUtils.cpp`, `PropertyName.cpp`,
`Panel.cpp`; Windows `TimeUtils.*` как dependency/adapter.

Довести поддерживаемые properties и сортировку до исходных правил, выбрать
колонки по folder capabilities, сохранить width/order/sort/view mode.
Перенести format/precision меню времени и auto-refresh policy, сохранить
focus/selection/scroll при refresh. UI не парсит отформатированный размер
или дату обратно для сравнения.

**Готово:** сортировка по типизированным значениям, null/undefined, одинаковым
именам и flat prefixes; столбцы/время persist и restore; refresh не рвёт выбор.
Геометрия четырёх режимов и context column menu на двух размерах.

### P0-07. Выделение, клавиши и все общие настройки списка

**После:** P0-03, P0-05. **Источники:** FM `PanelSelect.cpp`, `PanelKey.cpp`,
`PanelListNotify.cpp`, `SettingsPage.cpp`, `SettingsPage2.rc`, `RegistryUtils.cpp`.

Завершить relevant mouse/range/alternative selection и keyboard branches,
ShowDots/FullRow/AlternativeSelection/ShowSystemMenu и другие list flags.
Довести single-click и hover/track-select до основной table/icon панели,
не активируя файл при Ctrl/Shift/Meta selection/drag. Учесть уже изменённый
общий `qtlistview` и не вводить конкурирующую реализацию настроек.

**Готово:** keyboard-only и mouse workflows во всех четырёх режимах и disk/archive;
OK/Cancel/persistence, ranges, modifiers, leave, row removal и scroll во время
hover. Узкие Qt-отличия записаны; tooltip/focus/accessibility проверены.

### P0-08. Две панели как основной файловый UX

**После:** P0-02, P0-03, P0-04, P0-07. **Источники:** FM `App.cpp/.h`,
`Panel.cpp/.h`, `AppState.h`, `ViewSettings.cpp`, `resource.rc`, `PanelKey.cpp`.

Перенести one/two-panel переключение F9, active panel, Tab и Alt+F1/F2,
раздельные location/view/selection и восстановление раскладки. Команды
работают с активной панелью, второй путь готов для Copy/Move. Реальные transfer
сценарии закрывает P1-01; в этом шаге нельзя обещать их готовность.

**Готово:** обе панели независимы, переключение не теряет путь/выбор,
shortcut/toolbar target однозначен. Disk/disk, disk/archive, archive/archive,
compact layout и скрытая вторая панель покрыты tests/JSON.

### P0-09. Меню, toolbar, status bar и контекстное меню 7zFM

**После:** P0-03, P0-06, P0-08. **Источники:** FM `resource.rc`, `MyLoadMenu.cpp`,
`PanelMenu.cpp`, `App.cpp`, `SysIconUtils.cpp` и исходные bitmap resources.

Перенести command tables, grouping/order, checks, shortcuts, toolbar options,
status summary и context menu. Устранить самостоятельные заменители исходных
команд там, где они меняют workflow. Неперенесённые команды видимо disabled;
их enablement выполняется в собственном шаге функции. Системные icons через
Qt/platform adapters; перенесённые ресурсы сохраняют provenance.

**Готово:** source/resource-derived спецификация всей оболочки, keyboard access,
active panel routing, status totals; current-build normal/compact JSON. Не
утверждать pixel parity без rendered Windows reference.

### P0-10. Прогресс, сообщения, подтверждение отмены, pause/background

**После:** P0-02, P0-03. **Источники:** FM `ProgressDialog2.cpp/.h`,
`ProgressDialog2.rc/2a.rc`, `MessagesDialog.*`, callbacks и GUI `UpdateCallbackGUI2.*`.

Перенести CProgressSync и relevant CProgressDialog flow/statistics, ошибки и
closing policy; заменить разрозненные QProgressDialog и собственный journal UI.
Показывать текущий файл, totals, speed/elapsed/remaining, ошибки и итог.
Pause/background нуждаются в настоящих worker/process capabilities: заранее
проверить механизм каждой ОС; неподдерживаемая кнопка не имитирует паузу.
Если current `7zz` adapter не обеспечивает сценарий, создать явный prerequisite
адаптера внутри шага и оставить parent partial до решения, без переписывания engine.

**Готово:** общий progress для archive/filesystem/volumes, cancel-confirm и
закрытие/ошибка/partial result без потери сообщения, рабочая pause/resume по
capabilities, корректный background priority или записанная N/A платформы.
Детерминированные callback tests, destruction/cancel races и geometry всех состояний.

### P0-11. Общий OverwriteDialog и конфликты Copy/Move

**После:** P0-10. **Источники:** FM `OverwriteDialog.*`, `FSFolderCopy.cpp`,
`ExtractCallback.*`, `UpdateCallback100.*`, Common `FilePathAutoRename` dependency.

Перенести отображение двух файлов, исходные решения yes/no/all/auto-rename/cancel,
состояние решения на операцию и relevant callback flow. Worker ждёт решения
без прямого доступа к widgets; подтверждение безопасно закрывается при отмене.
Транзакционность/rollback локального adapter сохраняются и описываются.

**Готово:** одиночные/повторные конфликты и All, occupied rename target,
cancel во время prompt, Unicode/длинные имена, файловый и directory conflict.
Правильный результат байтов и исходный набор controls на двух размерах.

### P0-12. Интерактивный Ask при извлечении

**После:** P0-11. **Источники:** GUI `ExtractDialog.cpp`, `ExtractGUI.cpp`,
FM `ExtractCallback.cpp`, Common `ArchiveExtractCallback.cpp`, `Extract.cpp`.

Заменить адаптацию Ask → отказ исходным интерактивным flow через общий overwrite.
Сначала исследовать `7zz` prompt/protocol: можно адаптировать процессный диалог
или понадобится capability extension за ArchiveBackend. Не распознавать ответы
по хрупкому локализованному тексту без проверенного протокола. Сохранить validation,
selected extraction, rename-existing/extracted и password retry/cancel.

**Готово:** Ask/YesAll/NoAll/AutoRename/Cancel при реальном extract `.7z`/`.zip`,
failure и cancellation без deadlock; UI отражает частичный результат и ограничения
rollback. Tests backend и dialog consumers; история/Cancel не регрессируют.

Рабочие результаты: P0-12.1 закрепил общий интерактивный Ask на `7zz` transport;
P0-12.2 подключил незашифрованный root FullPaths FM consumer к исходным
`CPanel::CopyTo`/`CAgentFolder::Extract`/`CheckExistFile` и проверил source UX.
P0-12.3 подключил root NoPaths/Ask с выбранной папкой к тому же source callback;
P0-12.4 завершает nested/encrypted, `ExtractGUI.cpp`/`Common/Extract.cpp`
и parent приёмку. Точные критерии и evidence — в checklist; незавершённое не
считается готовым из-за совпадения поведения одного маршрута.

## 4. P1 — основные сценарии и полноценный UX (16 шагов)

### P1-01. Copy/Move routing между диском, архивом и панелями

**После:** P0-08, P0-11, P0-12, P1-04. **Источники:** FM `App.cpp::Copy`, `PanelCopy.cpp`,
Agent `ArchiveFolderOut.cpp`, `AgentOut.cpp`; локальный `mainwindow_fileoperations`.

Перенести исходную оркестрацию F5/F6: destination default второй панели,
selected/focused items, disk↔disk, archive→disk, disk→archive и archive→archive
через безопасный temp staging и backend capabilities. Одним пакетом не скрывать
разные типы transfer: при необходимости выделить по одному подшагу на маршрут.
Запреты nested/encrypted writes снимаются только после соответствующего owner.

**Готово:** destination и summary исходные; обе панели refresh/restore;
failure/cancel не удаляют source и не выдают partial copy за успешный Move.
Routing matrix tests и byte-level integration fixtures.

### P1-02. Cross-volume move и завершение FSFolderCopy

**После:** P1-01. **Источники:** FM `FSFolderCopy.cpp`, `FSFolder.cpp`,
`PanelCopy.cpp`; Windows FileIO/FileDir/FileName adapters.

Довести исходные move/rename/copy-delete branches, overwrite и callback semantics.
Сохранить narrow staging adapter вместо нового алгоритма переноса дерева.
Метаданные/ссылки, пока не поддержанные, обрабатываются явно и без ложного успеха.

**Готово:** same-volume rename, cross-volume fallback, ошибка удаления source,
occupied target, cancel между copy и delete, частично выполненный набор.
Тестировать fallback инъекцией файлового adapter и отдельно реальными томами
на доступных ОС; не требовать второй физический диск для CTest.

### P1-03. Полные Create/Rename/Delete и корзина

**После:** P0-03, P0-10, P0-11. **Источники:** FM `PanelOperations.cpp`,
`BrowseDialog.cpp`, `FSFolder.cpp`, `UpdateCallback100.cpp`; Windows FileDir/Shell.

Перенести CorrectFsPath/name validation, inline rename, создание составного пути,
исходные confirmations и Delete/Shift+Delete semantics. Подключить recycle bin
через узкий platform adapter с исходным Windows flow и явным поведением macOS/Linux.
Не потерять уже перенесённый recursive deletion без traversal symlinks.

**Готово:** inline accept/cancel/error сохраняет имя и выбор; занятые имена,
read-only/permissions, корзина vs permanent delete и partial failure проверены.
Archive mutation ветки передаются owner P1-04, не пишут `7zz` syntax в panel.

### P1-04. Запись, Rename/Delete/CreateFolder и comment в архиве

**После:** P0-03, P0-10, P0-11, P1-03. **Источники:** FM `PanelOperations.cpp::ChangeComment`,
`PanelCopy.cpp`; Agent `AgentOut.cpp`, `ArchiveFolderOut.cpp`, `UpdateCallbackAgent.cpp`;
GUI `UpdateGUI.cpp`, Common `Update*.cpp`, `SetProperties.cpp`.

Перенести relevant update orchestration/validation/selection recovery, включая
защищённые верхние архивы при наличии capabilities. Сохранить transactional
replacement за ArchiveBackend. Comment доступен только формату/handler, реально
поддерживающему свойства: если `7zz` не даёт нужной записи, нужен documented
capability prerequisite, а не самостоятельное изменение формата.

**Готово:** все поддержанные действия сохраняют bytes/metadata/пароль и исходный
архив при ошибке; busy и nested ограничения видимы. Unsupported handler branch
имеет доказательство и статус; операции/refresh и prompts проверены.
Этот шаг доступен раньше P1-01 и предоставляет ему archive-write capability;
не ждать завершения transfer routing для реализации самих архивных мутаций.

### P1-05. Полные update/path modes Add to Archive

**После:** P1-04. **Источники:** GUI `CompressDialog.cpp/.h/.rc`, `UpdateGUI.cpp`,
Common `UpdateAction.cpp`, `UpdatePair.cpp`, `UpdateProduce.cpp`, `ArchiveName.cpp`.

Перенести update-mode и path-mode controls/flow, relevant OnInit/OnOK,
поведение существующего архива и изменение compression options в Add.
Профили/история/пароль используют уже перенесённые методы. Функциональная
логика выбора файлов/операций берётся из upstream; process switches — backend.

**Готово:** Add/Update/Freshen/Synchronize в поддерживаемом scope сохраняют
правильный набор newer/older/deleted files; relative/full path modes и имена
проверены. Cancel/invalid options не сохраняют профиль и не меняют archive.
Compress resource geometry включает новые controls и compact scrolling.

### P1-06. Многотомные архивы и связанные диалоги

**После:** P1-05. **Источники:** GUI `CompressDialog.*`, `UpdateGUI.cpp`,
Common `Update.cpp`, `OpenArchive.cpp`; FM `SplitUtils.*` для reused volume parser.

Перенести volume-size control/validation/history и исходный сценарий создания,
открытия, проверки и извлечения архивных томов. Различать обычный Split и
многотомный archive. SFX control не обещает результат без P2-02; его prerequisite
можно выполнить раньше, если требуется для полного исходного сценария.

**Готово:** тома имеют верный набор/нумерацию, missing/occupied/corrupt volume
обрабатывается понятно, cancel убирает staged outputs, пароль работает.
Volume creation/list/test/extract integration и geometry формы.

### P1-07. Advanced compression options, время и RAM dialogs

**После:** P1-05, P0-10. **Источники:** GUI `CompressDialog.cpp` (включая
CompressOptionsDialog implementation), `CompressOptionsDialog.rc`, `UpdateGUI.cpp`,
Common `ZipRegistry.cpp`; FM `MemDialog.*`, `SettingsPage.*`.

Перенести параметры времени/точности, preserve access time, archive mtime,
share-for-write/delete-after-compress, advanced parameter field и relevant
memory prompts. Inspect original checks/SetOutProperties, не создавать второй
parser. Опции уровня ОС доступны по capability с объяснённой адаптацией.

**Готово:** timestamps проверены round-trip, unsupported/invalid parameters
дают исходный отказ; source удаляется только после доказанного успешного archive;
memory retry/cancel и profile persistence не теряют настройку. UI contracts
advanced/memory dialogs, tests success/failure/cancel.

### P1-08. Внешние viewer/editor/diff и возврат изменённого файла

**После:** P0-04, P1-04. **Источники:** FM `PanelItemOpen.cpp`, `PanelItems.cpp`,
`EditPage.cpp`, `RegistryUtils.cpp`, `VerCtrl.cpp`, Common `TempFiles.cpp`.

Перенести View/Edit/Diff command semantics, external app launch, ожидание/наблюдение
изменённой временной копии, вопрос об update и cleanup. Собственный ограниченный
текстовый редактор — явная дополнительная Qt-возможность, а не замена upstream
external workflow. Command templates/quoting не проходят через shell без нужды.

**Готово:** spaces/Unicode, app launch failure, changed/unchanged file,
accept/reject update, cancel и lifetime temp проверены controlled process fixture.
Editor/viewer/diff settings применяются и доступны с клавиатуры.

### P1-09. Обратное обновление цепочки вложенных архивов

**После:** P1-04, P1-08. **Источники:** FM `PanelItemOpen.cpp`, `PanelFolderChange.cpp`,
Agent `ArchiveFolderOpen.cpp`, `ArchiveFolderOut.cpp`, `UpdateCallbackAgent.cpp`.

Перенести исходный parent-link update flow от внутреннего файла к внешнему архиву,
prompt/password handling и удержание temp chain. Capabilities каждого handler
проверяются до записи; `tar.gz` и immutable/read-only levels не заменяются
самостоятельным packer. При недоступном process механизме — prerequisite adapter.

**Готово:** two-level update, ошибка внутренней/внешней записи, wrong password,
отмена prompt и возврат в родителя не уничтожают исходный внешний archive.
Проверить bytes на всех уровнях и восстановление listing/selection.

### P1-10. Полные свойства и исходное форматирование значений

**После:** P0-06, P1-04. **Источники:** FM `PanelMenu.cpp`, `PanelItems.cpp`,
`PropertyName.cpp`, `FormatUtils.cpp`, `ListViewDialog.cpp`, `EditDialog.cpp`;
Agent `AgentProxy.cpp`, `ArchiveFolder.cpp`; Common `PropIDUtils.cpp`.

Перенести перечисление folder/item/archive properties и relevant raw value
formatting, CRC, totals и full-value display. Snapshot process listing дополняется
только теми данными, которые реально даёт engine adapter; отсутствие raw values
не заполнять выдуманными значениями. Решение CProxyArc2 оформить по provenance.

**Готово:** undefined/zero/UInt64, Unicode, multi-selection и explicit/implicit
folders отображаются правильно; clipboard и detail не обрезают данные.
Resource comparator/JSON и deterministic property fixtures.

### P1-11. Корни, диски, сеть и Browse dialogs

**После:** P0-04, P0-05. **Источники:** FM `RootFolder.*`, `FSDrives.*`,
`NetFolder.*`, `BrowseDialog.*`, `BrowseDialog2.*`; Windows FileSystem/Net adapters.

Перенести иерархию root/drives/network, address dropdown и relevant Browse flows.
Для macOS/Linux сохранить workflow с mount roots/native folder picker там,
где Win32 services неприменимы; не имитировать несуществующий Windows namespace.
Remote credentials/real services не использовать в tests.

**Готово:** drive/mount disappearance, unreadable root, cancel picker и history
не ломают панель. Fake mounts/network adapter fixtures; native поведение
проверяется на соответствующей ОС и отражается в parity.

### P1-12. Полный OptionsDialog, Settings и Folders

**После:** P0-07, P0-08, P1-07. **Источники:** FM `OptionsDialog.cpp`,
`SettingsPage.*`, `FoldersPage.*`, `RegistryUtils.*`, `ViewSettings.*`,
Common `WorkDir.*`, `ZipRegistry.*`.

Перенести исходную оболочку шести страниц System/Menu/Folders/Edit/Settings/Language,
Apply/OK/Cancel, changed-state и перечитывание обеих панелей. В этом шаге довести
Settings/Folders: temp/work folder, large-pages/RAM flags и relevant paths.
Остальные страницы имеют owners P1-13/P1-14/P2-08/P2-09; их неполные controls
явно disabled. Не маркировать весь Options завершённым до этих шагов.

**Готово:** изменённые страницы применяются, Cancel не пишет pending values,
Apply затем Cancel сохраняет уже применённое; temp settings реально используются.
Page switching/focus и normal/compact JSON, invalid config и round-trip tests.

### P1-13. Страницы Edit и Menu

**После:** P1-08, P1-12. **Источники:** FM `EditPage.*`, `MenuPage.*`,
`RegistryUtils.cpp`, `MyLoadMenu.cpp`; Explorer `RegistryContextMenu.*` для dependency.

Перенести viewer/editor/diff configuration и menu policies/flags. System-shell
часть Menu использует adapter P2-09; обычные app options работают сейчас.
Настройка обязана менять поведение, а не просто сохранять checkbox.

**Готово:** launch settings применены к соответствующим командам; invalid app
path/templates и Cancel/Apply обработаны; menu flags дают наблюдаемый результат.
Для platform-only controls — explicit capability state и tests.

### P1-14. Language и локализация всего интерфейса

**После:** P0-09, P1-12. **Источники:** FM `LangPage.*`, `LangUtils.*`,
`TextPairs.*`, `StringUtils.*`; Common `Lang.*` и upstream language resources.

Перенести выбор/чтение language files, fallback IDs, смену меню/toolbar/dialogs
и исходные тексты команд. Убрать жёсткие русские captions в перенесённых forms,
сохраняя Qt standard buttons/platform conventions как адаптацию. UI state не
теряется при смене языка; сообщения backend остаются с явным переводом/контекстом.

**Готово:** минимум source English и Russian, missing/malformed file fallback,
hot switch/restart, accelerator collisions и длинные labels. Geometry normal/
compact в обоих языках и доступность controls, без новых algorithm duplicates.

### P1-15. Help/About и управление временными файлами

**После:** P0-09, P1-08, P1-14. **Источники:** FM `HelpUtils.*`, `AboutDialog.*`,
`App.cpp`, `ProgramLocation.*`, Common `TempFiles.*`, upstream help/license resources.

Перенести F1/context help/About и Delete Temporary Files command. Windows CHM
заменяется narrow help adapter на других ОС с сохранением разделов/ссылок;
описать источник и доступность документации. Cleanup удаляет только owned temp
с корректным active-open/edit lifetime, не произвольные файлы пользователя.

**Готово:** command routing, offline/unavailable-help error, version/license,
cleanup confirm/cancel/partial failure и сохранение active temp fixtures.
Help/About/cleanup UX закрыт в P1, не оставлен на этап выпуска.

### P1-16. Полный drag/drop между панелями и внешними приложениями

**После:** P1-01, P1-09, P0-07. **Источники:** FM `PanelDrag.cpp`,
`EnumFormatEtc.*`, `PanelCopy.cpp`, `App.cpp`; локальные filetable/drop adapters.

Перенести исходные drag effect/selection/destination rules: disk→disk,
disk→archive, archive→disk/outside, между панелями и relevant archive→archive.
OLE IDataObject заменяется Qt mime/platform promise adapter; archive drag-out
реально подготавливает временные bytes и удерживает их для получателя.
Mouse hover/single-click не открывает файл во время drag.

**Готово:** copy/move modifiers, отказ/отмена extract, outside receiver failure
и lifetime temp тестированы fake recipient; native file managers отдельно
на доступных ОС. Unsupported native promise branch остаётся documented partial.

## 5. P2 — расширенная функциональность и платформы (10 шагов)

### P2-01. Форматы, codec capabilities и Open Inside */#

**После:** P0-04, P1-05. **Источники:** FM `FilePlugins.*`, `FileFolderPluginOpen.*`,
`RegistryPlugins.*`, `PanelItemOpen.cpp`, Agent `ArchiveFolderOpen.cpp`,
Common `LoadCodecs.*`, `OpenArchive.*`, `DefaultName.*`, `ArchiveName.*`.

Заменить самостоятельный `archiveformats.h` capabilities из engine/handler;
перенести KeepName/default extension, supported update formats и `*`/`#` open modes.
Process backend может читать engine capabilities, но не выдавать произвольное
расширение за writable. GUI codec DLL loading требует separate adapter, P2-02.

**Готово:** unknown extension/content open, handler override/parser mode,
read-only/update capability, missing codec и version mismatch проверены;
dialog filters/default names следуют той же metadata.

### P2-02. SFX и внешние handlers/plugins

**После:** P2-01, P1-06. **Источники:** GUI `CompressDialog.cpp`, `UpdateGUI.cpp`,
Common `LoadCodecs.*`, FM `FilePlugins.*`, `RegistryPlugins.*`, `ProgramLocation.*`;
upstream SFX modules как отдельная runtime dependency.

Перенести SFX options/name/validation и plugin discovery/loading policy, исходную
проверку совместимости. Не переписывать SFX engine: использовать официальный
модуль, проверять его наличие и происхождение. Windows executable target не
объявлять native macOS/Linux application; выбрать и записать supported scope.

**Готово:** supported SFX создание/test, missing/incompatible module, cancel,
extension transitions и user prompt; external codecs реально доступны engine.
Принятые platform N/A не маскируют неперенесённую functional branch.

### P2-03. CRC/Hash workflow и тест архивов

**После:** P0-10, P1-10, P2-01. **Источники:** FM `PanelCrc.cpp`, GUI `HashGUI.cpp`,
Common `HashCalc.*`, `Extract.*`; engine digest algorithms остаются в `7zz`.

Заменить собственную orchestration исходным выбором методов/объектов, scan totals,
hash result presentation и test callback handling. Переносить application logic,
не вторую реализацию хешей. Проверить совпадение списков capabilities/меню.

**Готово:** одиночные/множественные методы и `*`, folders/selection в архиве,
known digests, corrupt archive, wrong password, failure/cancel и messages UI.

### P2-04. Benchmark

**После:** P0-10, P1-14. **Источники:** GUI `BenchmarkDialog.*`,
Common `Bench.*`, FM menu dispatch; `7zz b` или отдельный engine adapter.

Перенести dialog/state/result logic, threads/dictionary/CPU/ratings и
start/stop/restart. Engine выполняет benchmark, Qt отображает исходный набор
метрик. Сначала проверить process output capabilities; недоступные данные
нельзя вычислять придуманной формулой.

**Готово:** lifecycle и controls source-compatible, отказ по памяти, stop/close,
deterministic fake results и маленький bounded engine smoke. Геометрия normal/
compact; численные performance результаты машины не являются golden fixture.

### P2-05. Links, junctions и metadata файловой системы

**После:** P1-02, P1-03. **Источники:** FM `LinkDialog.*`, `FSFolder.*`,
`FSFolderCopy.cpp`, Windows `FileLink.*`, `FileIO.*`, `FileName.*`.

Перенести Link command, symlink/hardlink/junction capabilities, source validation
и metadata preservation. Запрет обхода опасных links при extract остаётся отдельным
security boundary. Для Unix adapters документировать различия inode/permissions.

**Готово:** relative/absolute/dangling links, cycle, unsupported privilege,
hardlink bytes/identity, copy/move/delete и cancel; platform fixtures без live data.
Не следовать ссылкам неявно и не путать link creation с extract policy.

### P2-06. Alternate streams и расширенные archive proxies

**После:** P1-10, P2-05. **Источники:** FM `AltStreamsFolder.*`,
`PanelFolderChange.cpp::OpenAltStreams`, `FSFolder.*`, Agent `AgentProxy.*`
(CProxyArc2), `ArchiveFolder.*`.

Перенести Windows streams namespace/enumeration/actions и архивное представление
streams/raw props. NTFS ADS не подменять Unix xattrs без documented decision.
Backend capabilities должны возвращать настоящие данные; если process listing
не даёт их, завершение требует adapter extension или доказанной N/A ветки.

**Готово:** streams list/open/copy/delete/property, occupied/cancel/error;
Windows fixtures на подходящей FS. Другие ОС имеют явный unsupported state,
не создают неожиданные `name:stream` файлы.

### P2-07. Оставшиеся Extract modes, права и security metadata

**После:** P0-12, P2-05, P2-06. **Источники:** GUI `ExtractDialog.*`,
`ExtractGUI.cpp`, FM `ExtractCallback.*`, Common `ExtractingFilePath.*`,
`ArchiveExtractCallback.*`, `Extract.*`, Windows `SecurityUtils.*`.

Перенести ElimDup, remaining path modes (включая absolute по source scope),
NtSecurity/attributes/timestamps/streams и metadata toggles. Source algorithm
копируется, ограничения небезопасных путей остаются explicit integration
policy. Absolute extraction требует предварительного определения допустимой
границы и подтверждения в исходном workflow, не просто снятия validation.

**Готово:** каждый control имеет effect/round-trip; root elimination,
ACL/permissions/timestamps при поддержке, malicious paths/links/collisions,
failure/cancel и partial writes. Differences исходного Windows и Unix recorded.

### P2-08. System page и file associations

**После:** P1-12, P2-01. **Источники:** FM `SystemPage.*`,
`RegistryAssociations.*`, `ProgramLocation.*`; platform registration adapter.

Перенести formats/association state, current-user/all-users distinction и
changed/apply flow. На macOS/Linux использовать платформенный registration
mechanism и соответствующие ограничения без прямого Win32 registry imitation.
Для разработки только fake registration tests; реальные системные изменения
выполняются как явно выбранное пользователем действие приложения.

**Готово:** detect/apply/revert state, denied permissions, handler absent и
external association changes; geometry страницы и typed adapter tests.

### P2-09. Shell/context integration и завершение Menu page

**После:** P1-13, P2-08. **Источники:** upstream
`CPP/7zip/UI/Explorer/ContextMenu.*`, `RegistryContextMenu.*`, `MyMessages.*`,
FM `MenuPage.*`, `MyLoadMenu.*`.

Исследовать Explorer functional command generation и перенести применимый scope;
Win32 COM extension не строится на Unix. Platform services/extension packaging
изолировать от widgets и ArchiveBackend. Полный native shell UX обязателен там,
где предусмотрен scope; недоступную интеграцию платформы описать явно.

**Готово:** menu selections/paths/quoting и flags ведут к правильному запросу,
registration/unregistration проверены fake environment; native extension
проверяется на ОС с установленной тестовой сборкой. Не регистрировать её на
машине пользователя только ради автоматического теста.

### P2-10. Большие списки и операции без блокировки интерфейса

**После:** P0-05, P0-06, P0-10, P1-09, P1-16. **Источники:** FM
`PanelItems.cpp`, `FSFolder.cpp`, Agent `AgentProxy.cpp`, Common
`EnumDirItems.cpp`, `Update.cpp`; Qt/backend scheduling adapters.

Проверить large/deep fixtures, перенести оставшуюся relevant scan logic и
устранить GUI-thread archive-copy preparation. Если нужна Qt model/view
адаптация, сохранять upstream folder owners/indices/selection, не переписывать
их алгоритмы ради QTableWidget replacement. Ограничить queued progress/messages.

**Готово:** navigation/cancel/selection отзывчивы, deep archive не переполняет stack,
stale responses отбрасываются, память temp chain очищается. Reproducible benchmark
данные/пороги и tests lifecycle; UI structure после оптимизации не регрессирует.

## 6. P3 — обновляемость, все платформы и приёмка (6 шагов)

### P3-01. Воспроизводимый upstream baseline и процедура обновления

**После:** P0-01; выполнять maintenance по мере всех переносов.
Для каждого borrowed module закрепить exact commit/path/repository, scope
omissions, dependency revision и adaptation record. Подготовить воспроизводимую
проверку vendored equality и сравнение old→new upstream отдельно от Qt diff;
проверить в disposable snapshot, не изменяя sibling upstream. Reuse scripts,
добавлять reusable automation только при реальной повторяемой необходимости.

**Готово:** baseline извлекается без старого локального checkout; borrowed modules
не имеют неизвестного происхождения/двух authoritative implementations;
демонстрация update review одного adapted и одного unchanged модуля.

### P3-02. Сквозная regression и security матрица

**После:** P0/P1; advanced rows после соответствующих P2. Проверки каждой функции
уже должны существовать. Собрать traceability scenario→owner→CTest и закрыть
пробелы: success/failure/cancel, wrong password, temp lifetime, interrupted
update/rollback, traversal/absolute/links/case/Unicode collision, process errors,
close-window races. Fixtures local/deterministic, без реальных архивов/сервисов.

**Готово:** regression воспроизводит критические сбои, в logs/reports нет passwords,
сквозной цикл create/list/edit/update/extract/test/transfer проверен. Tests не
зависят от предыдущего запуска; известные неподдержанные ветки имеют evidence.

### P3-03. CI Windows/macOS/Linux и native проверки

**После:** первая реализация возможна уже после P0-02; финальная матрица после P2.
Добавить отсутствующую в исходном снимке CI-конфигурацию: reproducible Qt/toolchain/
official 7zz, format/build/CTest, diagnostics artifacts. Запуск на GitHub требует
опубликованного репозитория/соответствующего user task, локальная настройка — нет.

**Готово:** фактические результаты трёх ОС с versions/date/revision; native
dialogs/shortcuts/icons/drag/registration/FS differences проверены отдельно.
При недоступной ОС записать unverified; config файл сам по себе не доказывает
триплатформенную готовность. Windows visual reference — отдельное свидетельство.

### P3-04. Поставка приложения, движка, ресурсов и лицензий

**После:** P2-01, P2-02, P1-14, P1-15, P3-03.
Выбрать модель официального `7zz` (bundled/system), detection/version errors,
upstream notices/third-party licenses и лицензию собственного UI. Подготовить
packages/deployment Qt/plugins/lang/help/resources для трёх ОС и smoke сценарий
на чистой среде. Подписание/публичный выпуск — отдельное явно разрешённое действие.

**Готово:** portable/installable artifacts воспроизводимы; app на чистой среде
открывает/create/extract archive, корректно сообщает missing dependency;
применимые лицензии/notice доступны пользователю, решения не остаются TODO.

### P3-05. Закрытие omitted modules, branches и интеграционного долга

**После:** P0–P2, P3-01. Повторить полную инвентаризацию 109 `.cpp`, headers,
resources и фактически добавленных dependencies. Для omitted `ProgressDialog`
legacy/CE, standalone GUI/console entrypoints, ClassDefs/StdAfx и Win32 adapters
нужен documented scope/N/A, а не «файл не нужен потому что пока не используем».
Использовать upstream makefile/FM.mak/7zip.mak как карту зависимостей.

**Готово:** нет бесхозных modules/unknown provenance; самостоятельная functional
логика мигрирована или обоснована отсутствием usable upstream implementation.
Каждый claimed full module проверен по bodies, не по pathname. Неперенесённые
обязательные branches создают конкретные новые подшаги, не закрываются N/A.

### P3-06. Итоговая функциональная и UX приёмка

**После:** все обязательные шаги. Пройти исходные menus/toolbar/dialogs и
end-to-end workflows 7zFM. Проверить оба режима панелей, клавиатуру, compact/
normal layout, оба языка, accessibility/focus, prompts/error/progress/help.
Сопоставить resources и current-build reports, rendered Windows там, где доступен.

**Готово:** опубликована матрица checked/partial/unverified/N/A с доказательствами
по ОС и source scope; README не обещает лишнего; счётчик checklist актуален.
Только реальные закрытые обязательные пакеты дают 44/44 (или новое честное число
после decomposition/additions). Известные обязательные пробелы не объявляются
«полным портом» ради завершения плана.

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

Следующий доступный результат — **P0-09**; далее по приоритету и
зависимостям. UX идёт вместе с каждым пакетом.
