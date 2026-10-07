# Карта документации

Каждый вид сведений имеет основной документ. В остальных местах оставляем
краткое описание и ссылку; не переносим туда историю проверок и текущие счётчики.

| Что нужно узнать или обновить | Основной документ |
| --- | --- |
| Обзор проекта и быстрый старт | [README](../README.md) |
| Общие правила работы агента и source port | [AGENTS.md](../AGENTS.md) |
| Scope пакетов, зависимости, размер итераций и критерии приёмки | [План](porting-plan.md) |
| Текущие статусы, счётчик и выбор следующего доступного шага | [Чеклист](porting-checklist.md) |
| Короткое поручение продолжить план | [Стартовый запрос](porting-chat-prompt.md) |
| Происхождение модулей, copied/adapted/omitted scope, причины отклонений и датированные свидетельства переноса | [Source audit](upstream-port-audit.md) |
| Bodies/callers и границы omitted UI modules при итоговой сверке | [Omitted scope](omitted-scope-audit.md) |
| Машиночитаемый baseline borrowed files | [Manifest](upstream-baseline.json) |
| Процедура сравнения и обновления upstream | [Upstream update](upstream-update.md) |
| Реальные сценарии, команды и платформенные ограничения | [7zFM parity](7zfm-parity.md) |
| Обязательные открытые UX-коррекции | [UX audit](ux-port-audit.md) |
| Владельцы компонентов, lifetime и границы UI/backend | [Архитектура](architecture.md) |
| Установка, toolchain, сборка, запуск и native runtime | [Разработка](development.md) |
| Поведение, покрытие и выбор регрессионных проверок | [Тестирование](testing.md) |
| Сценарий → source owner → CTest, критические отказы/Cancel/security | [Regression matrix](regression-matrix.md) |
| Контролы, состояния, normal/compact геометрия и widget JSON | [UI contracts](ui-layout-contracts.md) |
| Извлечение ресурсов и resource-to-widget comparison | [UI reference tooling](ui-reference.md) |
| Сбор настоящего Windows runtime reference | [Windows reference](windows-ui-reference.md) |
| Общие Windows/Linux runners, transport и snapshot checks | [Hyper-V runners](hyperv-runners.md) |
| CI jobs, pinned tools/runtime, диагностика и границы native acceptance | [CI](ci.md) |
| Локальная память, проверка старых свидетельств и handoff | [Memory protocol](agent-memory.md) |

Skills в `.agents/skills/` содержат профильные приёмы и ссылки на эти правила.
Их assets задают начальные страницы локального vault; `.codex/` — экземпляр
локальной памяти, а не ещё одна версия документации.

## Как выбрать минимальный контекст

Начинать с `AGENTS.md` и индекса памяти, затем читать основной документ
по задаче. Большие audit, parity и testing содержат датированные дополнения;
их начало не заменяет текущие статусы в чеклисте.

| Задача | Что читать дальше |
| --- | --- |
| Продолжить портирование | Правила выбора и строку ID в [чеклисте](porting-checklist.md), соответствующий пакет и правила итерации в [плане](porting-plan.md) |
| Найти владельца или границу адаптера | [Разделы архитектуры](architecture.md#reading-guide), затем bodies/callers в коде |
| Исправить поведение и выбрать проверки | [Regression matrix](regression-matrix.md), описание нужной suite в [testing](testing.md), [команды сборки и тестов](development.md#local-commands) |
| Изменить форму или layout | Контракт затронутого вида в [UI specifications](ui-layout-contracts.md), [resource tooling](ui-reference.md) и открытые коррекции в [UX audit](ux-port-audit.md) |
| Сверить происхождение кода | [Manifest](upstream-baseline.json) и раздел audit по пути, символу или ID; для обновления — [процедуру](upstream-update.md) |
| Проверить Windows/Linux | [Hyper-V runners](hyperv-runners.md); для долгих suites — [явный opt-in](development.md#long-suites) |
| Обновить инструкции или память | [Memory protocol](agent-memory.md), нужный skill и его assets; продуктовые факты — в основном документе из таблицы выше |

Для поиска раздела без загрузки всего документа:

```sh
rg -n '^#{1,3} ' docs/architecture.md
rg -n -F 'CAgent::Open' docs/upstream-port-audit.md
rg -n -F 'archive_transfer' docs/testing.md docs/regression-matrix.md
```

Читать найденный раздел и связанные зависимости, а не все совпадения подряд.
При расхождении проверить текущий код, upstream и ревизию evidence; порядок
разделов и исторические слова «active/accepted» не определяют текущую приёмку.

## Как поддерживать документы

- Менять основной документ, когда изменился соответствующий факт. В чеклисте
  достаточно статуса, краткого результата и ссылки на подробный audit.
- Новые результаты проверок записывать с датой, ОС, ревизией/snapshot и scope
  в соответствующем разделе audit; в testing описывать, что проверяют suites.
  Ссылки из parity, architecture и UI contracts должны объяснять сценарий,
  владельца или геометрию, а не повторять полный журнал приёмки.
- Не удалять уникальные full commits, source symbols, omitted branches,
  ограничения и причины отклонений ради сокращения текста. Исторические
  инвентаризации находятся в [history/](history/); прошлые проверки
  не подтверждают текущий checkout.
- Стартовый запрос и README не закрепляют следующий ID или старый набор
  результатов: выбирать работу по текущему чеклисту и фактическим capabilities.
  Локальный индекс содержит ссылки на заметки; детали handoff остаются в заметке.
