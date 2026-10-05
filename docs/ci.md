# CI and native acceptance runners

`.github/workflows/check.yml` configures the full source gate on Ubuntu24.04,
macOS15 and the authorized x64 Windows guest. It is local configuration: no
workflow has been published or run on GitHub, and no GitHub runner was installed
or registered on the shared VM. Actual SSH/native desktop results remain in the
port audit/checklist; they do not establish hosted CI execution.

Unix jobs use Qt6.8.3, Python3.12, aqtinstall3.3.0, clang-format23.1.2 and
Ninja1.12.1. Windows uses the recorded MSVC2022/Qt6.8.3/Python/formatter kit under
the paths owned by [Hyper-V runners](hyperv-runners.md). OS/compiler packages can
receive image updates; diagnostics record actual versions. This is a pinned
source/runtime and primary-tool configuration, not a hermetic OS image.

Both checkouts disable credential persistence. Upstream is checked out at the
full recorded commit and verified by `upstream_baseline.py`; existing builders
archive it into private build snapshots. Unix standalone 7zz also builds in a
temporary snapshot. Runtime, module and SFX must exist before configuring tests,
so native engine fixtures cannot silently become missing-runtime skips.
Every job invokes `scripts/check.sh`: formatting, all-target build and full
CTest through the project wrappers. Diagnostics retain Testing logs, widget JSON,
compile database/cache and runtime provenance; no binary release is uploaded.
The full-gate transcript and failure buffers are retained in task-owned
`build/ci-check.log` and `build/ci-tmp/sevenzip-script.*`; upload does not scan
the runner's general temporary directory. Pipe failure propagates through `tee`.
Independent platform failures do not cancel the other platforms.

Windows runs only on manual `workflow_dispatch` with the dedicated
`sevenzip-desktop` self-hosted label. Pull-request code never automatically runs
on the privileged shared guest. The job checks RUNNER-WINDOWS identity,
interactive process session and an elevated parent before compiling. It requires
an existing ordinary desktop process for the independently unelevated COM fixture.
Running the full native gate in a hosted service/session0 would not verify
focus, popups, shell integration or privileged link/security fixtures.
Registering a GitHub agent is a future separately authorized hosting operation;
the current workflow does not provision or start one. The existing SSH runner
continues to provide actual checks without GitHub hosting.

GitHub actions are pinned to complete commits, verified against their official
tag refs on2026-10-05: [checkout](https://github.com/actions/checkout/tree/3d3c42e5aac5ba805825da76410c181273ba90b1),
[setup-python](https://github.com/actions/setup-python/tree/ece7cb06caefa5fff74198d8649806c4678c61a1),
[upload-artifact](https://github.com/actions/upload-artifact/tree/b7c566a772e6b6bfb58ed0dc250532a479d7789f).
The Qt installer version is [aqtinstall3.3.0](https://github.com/miurahr/aqtinstall/releases/tag/v3.3.0).
`actionlint .github/workflows/check.yml` validates YAML, expressions, runner
labels and embedded shell commands; `.github/actionlint.yaml` declares the
dedicated custom label. A passing static check does not prove Qt downloads,
hosted image compatibility, native desktop access or a passing GitHub run.

Finder drag-out, installed clean-environment package tests and rendered EN/RU
normal/compact comparisons remain independent mandatory acceptance evidence.
