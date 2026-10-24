# Ранние свидетельства tooling

> Архив завершённого Qt-портирования, 2026-10-17. Эти записи перенесены
> из рабочих инструкций; они не подтверждают текущие toolchains или guests.
> [Рабочая документация](../../README.md).

## Upstream baseline, macOS arm64, 2026-09-30

A temporary clone made with
`git clone --no-hardlinks --no-checkout ../7zip-upstream` was verified on macOS
arm64: 192 exact, 135 adapted and three local paths at commit
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`. This proves the check does not depend
on the original checkout location; it does not prove availability of the public
remote during offline development.

The review command was exercised in a disposable descendant of the recorded
commit with synthetic changes to one adapted module (`NetFolder.cpp`) and one
exact dependency (`Net.cpp`). It reported both old→new revisions and the
separate review instructions; neither this repository nor the sibling upstream
was changed by that demonstration. A real upstream update had not been applied
in that investigation.

## Windows ARM64 runtime builders

The original development guide recorded unchanged SFXWin compilation on
Windows11 ARM64 with MSVC14.44 and SDK10.0.26100.0 in a scoped Visual Studio
developer environment. Console/Format7zF/Alone2 compiled there; both console
clients reported26.03. The old VM was separate from the shared x64 Hyper-V
runners. These build samples do not validate the current guests.

## Early Qt kit selection

The runner guide recorded both Qt6.12.0 probes, Linux full240/240 PASS and
Windows runtime pending. Earlier systemQt6.4.2/WindowsQt6.8.3 receipts remained
historical. The pending statement was superseded by subsequent
[platform acceptance](ux-port-audit.md#final-acceptance-of-product3a0b3b7).

The removed GitHub workflow selected Python3.12 and Ninja1.12.1.
The imported Windows Qt installer retained the aqtinstall3.3.0 adapter
for Qt6.12's compiler-specific repository layout. These historical pins do
not establish an active CI job or replace the current installer configuration.
