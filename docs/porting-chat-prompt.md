# Сообщение для каждого нового чата

Скопируйте текст ниже целиком. Обычно ID менять не нужно: агент выбирает
следующий доступный шаг по чеклисту. Для конкретного шага замените предложение
о выборе на «Выполни шаг P0-02» с нужным ID.

```text
Продолжай портирование Sevenzip Desktop по docs/porting-plan.md и
docs/porting-checklist.md. Прочитай AGENTS.md, индекс .codex/memory/README.md
и только relevant notes; проверь git status, текущий код, upstream-port-audit
и 7zfm-parity. Не сбрасывай и не присваивай себе чужие изменения.

Выбери первый незавершённый незаблокированный шаг минимального приоритета, для
которого выполнены фактически нужные dependencies/capabilities. Назови его ID
и конкретный результат, затем выполни его до конца. Сейчас следующий ID —
P2-01: formats/codec capabilities и Open Inside */#, source LoadCodecs/OpenArchive/
FilePlugins owners; engine metadata вместо самостоятельных extension guesses.
Native bridge из этого пакета также разблокирует mandatory P1-17/18; их не
закрывать synthetic tests или console-only capability.
P1-16 implementation accepted: native OnDrag/Drop/transfer/CompressDropFiles и
CApp Begin/End, Qt MIME transport; focused6/6PASS0.99s macOS arm64 Qt6.11.2
2026-10-02. Actual ZIP binary bytes/known+unconfirmed receiver/refusal/drag+extract
cancel и native disk Copy/Move проверены. Live both views/lazy F9 panel, popup/
confirmation/Add, Yes/No/Cancel, close/SingleClick/hover guards уже подключены;
focused11/11PASS7.44s до последних naming/header fixtures, actual archive→disk/
archive также проверены. Полный unsandboxed check.sh formatting/build/179 tests
PASS67.66s macOS arm64 Qt6.11.2 2026-10-02. Нативная проверка Finder вынесена
в отдельный обязательный P3-03.1; она не блокирует приёмку реализации P1-16
или работу над P2, но требуется для P3-03/P3-06. Computer Use app access отказан,
запрос ожидает ответа; native transfer не выполнялся. Baseline218 exact/159 adapted/3 local.
Current audit/note содержат точный scope; eager materialization documented,
GUI-thread preparation/large-list responsiveness остаётся mandatory P2-10.
P1-15.2 и parent приняты: actual native BrowseDialog2/modal command, creation
ownership registry, native/Qt/nested/process leases, private spool publication,
Yes/No/Cancel/first failure/replaced identity и active child-editor protection.
Source ShowDots/SingleClick/hover/FullRow/Grid, context/props, navigation/Help/F1
подключены; EN/RU900×580/500×320 current-build JSON reviewed. Focused18/18
PASS12.43s; full unsandboxed check.sh formatting/build/175 tests PASS66.82s
macOS arm64 Qt6.11.2 2026-10-01; baseline217 exact/158 adapted/3 local verified.
User archives remain unowned. Cross-volume publication IO/late Finish cancellation
locally untested; Windows/Linux runtime/rendered parity unverified. Directory
Open Outside : 7-Zip disabled до P2-07 directory CLI consumer; это обязательный
сохранённый долг данного owner. File self-open/OS association подключены.
P1-15.1 принята: original HelpUtils/About и release HTML/CSS, все текущие
Options/Add/Extract/advanced contexts; focused11/11PASS4.70s, final F1/geometry
4/4PASS1.05s, final native About2/2PASS0.54s и full unsandboxed check.sh172/172
PASS63.63s macOS arm64 Qt6.11.2 2026-10-01; baseline215 exact/156 adapted/3 local.
Disabled System/Benchmark source Help callbacks закрываются с P2-08/P2-04;
cleanup context принят с P1-15.2.
P1-14.1/.2 и parent приняты: все текущие forms/messages/local context EN/RU,
static RC/dynamic LangString fallbacks, hot switch/restart/pending state, source
RAM/size/Auto/bookmarks, geometry normal/compact и keyboard/buddy/collision checks.
Full unsandboxed check.sh170/170PASS63.11s macOS arm64 Qt6.11.2 2026-10-01;
baseline210 exact/152 adapted/3 local verified. Final gate failure fixtures13/13
PASS3.00s, isolated modal consumers2/2PASS1.04s. Qt platform standard buttons/
macOS mnemonic policy сохранены, Windows/Linux runtime/rendered parity unverified.
Core language services — sevenzip_filesystem, widgets — sevenzip_panel; cancelled
result определяется HRESULT/typed completion, partial-move warning сохраняется.
Source baseline0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17. Счёт27/46, остаётся19.
P1-13 parent открыт до .2:
source shell policies/flags/registration требуют P2-09 consumer, controls disabled
с причиной. P2-09 зависит от P1-13.1 capability, не от открытого parent.
P1-12 parent открыт до .2 всех page owners; .1 capabilities не блокируют Language
и System. Windows ADS/runtime и Windows/Linux rendered parity не проверены.
ZIP comment P1-17 и реальные decoder RAM
prompts P1-18 заблокированы отдельно до source writer/decoder capability;
обе функции обязательны для полной итоговой приёмки.
Работай крупными итерациями: целый интегрированный компонент или пользовательский
сценарий вместе с зависимостями, адаптерами, подключением consumers, UX и тестами.
Если пакет велик, выдели несколько крупных результатов с простыми ID вида
P0-02.1, .2; только один уровень подшагов. Helpers, ресурсы и отдельные методы
остаются внутренними задачами одной итерации, без отдельных завершений/коммитов.
Следуй разделу «Размер итерации и коммита» плана. Можно завершать несколько
итераций за чат; если поручен весь план, продолжай весь доступный scope. Один
заблокированный пункт не останавливает другие пакеты. Не повторяй уже
подтверждённые переносы.

При перезапуске сначала сохрани historical IDs/commits/evidence и составь карту
оставшегося scope: несколько крупных результатов со ссылками на прежние открытые
строки. Не продолжай рекурсивное дробление и не закрывай старые строки только
ради новой структуры. Для текущего P0-02 проверь границы P0-04/P0-05/P0-10;
работу соседнего пакета учитывай под его ID с точной prerequisite-связью.
Сверь незакоммиченные изменения с текущим кодом и прежним handoff; не считай
их своими без проверки и не переписывай историю Git.

Нужен source port: сначала прочитай оригинальные bodies и dependencies из
точного upstream commit, скопируй функциональную основу с исходными путями,
именами, control flow и notice. Адаптируй только необходимые Qt/platform/7zz
границы. Совпадение поведения или имени файла не считается переносом.
../7zip-upstream остаётся read-only. ArchiveBackend остаётся границей операций;
widgets не строят 7zz arguments и не реализуют форматы/сжатие.

UX входит в этот же шаг: commands/menus/shortcuts, focus/selection, dialogs,
errors/progress/cancel и persistence. Не оставляй его на последок. Сверяй
исходные 7zFM resources и functional code. Для visual/text/resize changes
проверь text spec, normal/compact geometry и current-build widget JSON;
screenshots только по правилам AGENTS.md. Не заявляй Windows pixel parity
или проверку всех ОС без соответствующих свидетельств.

Добавь meaningful regressions, для операций success/failure/cancel и relevant
Unicode/space/unsafe-path cases. Собери affected targets и проверь focused
CTest/direct consumers. Перед завершением code/tests/build changes выполни
./scripts/check.sh по AGENTS.md; для docs-only — git diff --check.
Старые test results не подтверждают новый код.

Обнови docs/upstream-port-audit.md (revision, paths, symbols, copied/adapted,
omitted scope/dependencies и причины), docs/7zfm-parity.md и другие документы,
чьи claims изменились. Запиши evidence/status своего шага в чеклист, обнови
счётчик и useful local handoff, сохрани чужие записи. Закрой parent только
после всех подшагов и интеграции. Недоступную обязательную ветку не объявляй
done или N/A ради закрытия шага: укажи blocker и следующий prerequisite.
Если одна функция блокирует составной пакет, проверь её реальные consumers и
выдели самостоятельный blocked ID по правилу плана; сохрани исторические
ID/evidence, уточни scope и зависимости остальных пакетов, обнови счётчик и
продолжи первый доступный шаг. Parent принимай только если оставшийся scope
самостоятельно проверен. Финальную полную приёмку с обязательным blocked ID
не закрывай.

В конце сообщи: ID и результат, что скопировано/адаптировано, точные checks
и проверенную ОС, оставшиеся ограничения и следующий доступный ID. Продолжай
доступную работу без промежуточных подтверждений, даже если другая функция
заблокирована.
Каждую завершённую крупную итерацию коммить отдельно после проверок. Отдельный
helper, новый dependency или промежуточная запись приёмки не завершает итерацию.
Focused checks запускай по ходу реализации, полный gate — перед завершением
итерации согласно AGENTS.md. Граница ответа/compaction не требует коммита.
Включай только
свои файлы/hunks, сохраняй чужие изменения и указывай hash локального коммита.
Публикация, push/PR, release и изменение upstream в это поручение не входят.
```
