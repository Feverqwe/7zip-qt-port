# P3-03 — Interactive runner process context (2026-10-16)

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

[Index](../upstream-port-audit.md). This infrastructure prerequisite belongs to
P3-03.3; it does not establish a cause or fix for cold codec-loading delays.

`scripts/hyperv_interactive.py::interactive_command` now requests priority4 for
its own ephemeral scheduled task. Microsoft documents default7 as background
`BELOW_NORMAL_PRIORITY_CLASS`, and4–6 as interactive `NORMAL_PRIORITY_CLASS`:
[TaskSettings.Priority](https://learn.microsoft.com/en-us/windows/win32/taskschd/tasksettings-priority).
The child records/guards its actual priority before `check.cmd` and sets the
explicit normal-context expectation consumed by `process_priority::initTestCase`.
RunLevel, administrator-token guard, active-session requirement, cleanup,
timeouts, source snapshot and product priority implementation are unchanged.
No upstream code is copied or modified in this infrastructure correction.

Actual RED snapshot `20261016T062233Z-3e13a086` observes own and child native
priority classes16384 (below normal); `process_priority` fails its initial
normal-class assertion against32. GREEN `20261016T062543Z-449816c5` observes32
for both, and the same public CTest consumer passes1/1 in0.72s. Both use the
official interactive/elevated fresh-snapshot transport. Offline launcher tests
fail twice before the fix (Limited/Highest) and pass27/27 afterward. The native
probe exports only numeric classes of its own processes, without unrelated
process information, credentials or OS/security-setting changes.

Required unsandboxed macOS Qt6.12 gate passes323/323 in262.65s (eleven slow
suites excluded). The integrated secondary-dialog snapshot
`20261016T062702Z-f814576a` repeats the actual normal-priority Windows CTest
consumer in a passing10/10 focused set. Linux also passes10/10, including
unchanged POSIX priority behavior. These checks accept the runner context
contract, without accepting whole Windows timing or cold codec stability.

Evidence is retained under ignored
`build/review/final-availability-8e9343b-20261014/runner-*priority*`, with official
run metadata/logs under `build/runners/hyperv/<run-id>`. Current full/native
acceptance and codec deadlines remain separate requirements of P3-03.
