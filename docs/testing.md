# Regression strategy

Для изменения поведения выбрать сценарий в [regression matrix](regression-matrix.md),
найти его suites в каталоге ниже и проверить текущие public API/callers.
Требования к regressions, fixtures и focused/full checks — в
[AGENTS.md](../AGENTS.md#validation-and-ui-reference); команды и opt-in slow suites —
в [development](development.md#local-commands).

## Каталоги покрытия

- [Поведение и suites](testing/coverage.md): поиск по CTest name, классу или ID.
- [UI и геометрия](testing/geometry.md): producer targets, состояния и widget JSON.
- [UI contracts](ui-layout-contracts.md): требования к конкретному виду.

Каталоги содержат накопленные свидетельства разных ревизий. Сопоставлять их
с текущими tests/CMake и кодом; старое PASS или «unverified» не определяет
текущую приёмку. Датированные результаты новых запусков писать в
[source audit](upstream-port-audit.md), а здесь менять описание покрытия.

```sh
rg -n -F 'archive_transfer' docs/testing/ tests/ docs/regression-matrix.md
./scripts/build.sh dev <targets>
./scripts/ctest.sh --preset dev -R '<regex>' --output-on-failure
```

## What to test

[Подробное покрытие и исторические stages](testing/coverage.md#what-to-test).

## Проверка компоновки Qt

[Geometry suites и отчёты](testing/geometry.md#проверка-компоновки-qt).
