# Hyper-V runners

Sevenzip Desktop reuses `runner-linux` and `runner-windows` on Windows 11
Hyper-V host `192.168.1.14`, host SSH account `anton`. VM storage stays under
`D:\runner`; Windows guest `C:` is inside its VHDX on that disk. These are the
same shared x64 guests used by htopQt, with separate project snapshot directories.
The helper starts from htopQt's `scripts/hyperv-runner.py` at commit
`f57b93a72d765f738a6b779b4b3c94e16c0ef64a`, adapted for Sevenzip resources,
MSVC, pinned engine builders, formatter and SSH stdin transport.

```sh
python3 scripts/hyperv-runner.py probe linux
python3 scripts/hyperv-runner.py probe windows
python3 scripts/hyperv-runner.py check linux
python3 scripts/hyperv-runner.py check windows
python3 scripts/hyperv-runner.py check linux --preset release
python3 scripts/hyperv-runner.py check windows --preset release
```

The global skill at `~/.codex/skills/runners` must be installed. Its
`scripts/hyperv_ready.py <platform> --start` starts only the requested existing
guest when Off, rediscovers DHCP and verifies authenticated guest identity.
The project helper uses the returned `ssh_argv`, strict host-key checking,
the Mac key `~/.ssh/id_ed25519` and stable guest keys in
`~/.ssh/known_hosts_codex_runners`. Keys and credentials are never transferred.

## Source and runtime scope

Each check creates a fresh `/home/runner/sevenzip-desktop/runs/<id>` or
`C:\runner\sevenzip-desktop\runs\<id>`. The SHA256-verified archive contains
current tracked project sources, resources, tests, docs, skills and build
configuration, including modifications and excluding deleted files. Add new
files to the Git index before running; the helper itself is included explicitly.
Unrelated untracked files, ignored state and the project's `.git` are excluded.

The helper also includes an upstream Git bundle at the exact revision recorded
in `docs/upstream-baseline.json`. `--upstream /path/to/7zip` overrides the default
read-only sibling checkout. A temporary local mirror creates the bundle ref;
the original checkout's refs, index and files remain untouched. No upstream
network fetch is needed. The guest clones the bundle into snapshot-local
`upstream/` and uses existing project builders to prepare dynamic engine,
codec module and SFX. Linux also builds standalone `7zz` from `Bundles/Alone2`;
Windows's runtime builder produces it. All runtime binaries match the guest
architecture and pinned source. Environment changes apply only to this check.

Before transfer, the helper checks actual local staged/worktree diffs for
whitespace errors. The guest creates a snapshot-local Git commit for the full
gate, so original upstream notices with historical whitespace are baseline
content rather than newly staged additions. The client Git state is unchanged. Runtime
preparation and `scripts/check.sh` write to `check.log` in the guest directory.
`--preset release` runs the full Debug gate first, then Release build/CTest;
an earlier failure stops later stages. Connection records and per-run SHA256,
project revision, directory, preset and status stay in ignored
`build/runners/hyperv/` on the Mac. Local source/toolchain preparation failures
may occur before a remote run record exists. No automatic cleanup, VM creation,
package installation, shutdown or publishing is performed.

## Toolchains

Both guests use the external `Runner LAN` switch. HTTPS probes on 2026-10-03
returned HTTP 200 from PyPI (`/simple/clang-format/`) and GitHub on both guests.
`https://download.qt.io/` timed out on both guests with an explicit IPv4
12-second limit; Linux's default address selection also timed out after 20 seconds.
External connectivity is available, but package sources must be checked
individually before provisioning. Formatter installation can now use guest pip
directly when PyPI is reachable; the earlier Mac wheel transfer was a setup
workaround. The pinned upstream bundle remains intentional: each gate uses
the recorded revision without depending on an external Git server.

Linux: Ubuntu 24.04.5 LTS x86_64, GCC 13.3, Qt base/DBus 6.4.2,
CMake 3.28.3, Ninja, Git/Python and DBus daemon. P2-08 association services
require `pkg-config` and `libglib2.0-dev` (GIO); these build dependencies were
installed on the shared Linux runner on 2026-10-03 without upgrading existing
packages. GIO is 2.80.0 there.
Software rendering is selected for this guest without an accelerated GPU.

Windows: Windows 11 Pro 10.0.26300 x64, MSVC 2022 Build Tools,
Qt 6.8.3 `msvc2022_64`, Python/CMake/Ninja and Git Bash. The installed shared
toolchain is under `C:\runner\htopqt`: `VS`, `Python`, `Git`,
`Qt\6.8.3\msvc2022_64`. Sevenzip uses `vcvars64.bat` and `cl.exe` for C/C++;
htopQt's clang-cl selection is not inherited. PATH and Qt/module discovery are
scoped to the check process. The helper does not change shared packages or toolchains.
Both guests build with four jobs and run CTest sequentially.

Sevenzip uses clang-format 23.1.2, matching the local source formatting. Version
18 in htopQt's environment rejects existing source formatting before compilation.
Separate Python virtual environments contain the pinned PyPI formatter under
`/home/runner/sevenzip-desktop/tools/formatter` and
`C:\runner\sevenzip-desktop\tools\formatter`. Initial Linux provisioning also
installed `python3-venv`; existing compilers/Qt and htopQt's formatter remain in place.
To recreate the formatter, run the guest's `python3 -m venv <directory>` (Windows:
`C:\runner\htopqt\Python\python.exe -m venv <directory>`), then that venv's
Python with `-m pip install --no-cache-dir clang-format==23.1.2`.

Offline transport tests run with `python3 tests/hyperv_runner_tests.py`; they
check Unicode/spaced filenames, modified/deleted/untracked content, exact pinned
bundle retrieval without upstream mutation, local whitespace rejection, large
Windows scripts through SSH stdin, Linux failure exit/log propagation, and
failed/interrupted run records.
Runner orchestration tests remain outside the application's CTest gate.

SSH checks cover the project gate, not interactive desktop/Finder behavior,
distribution or rendered Windows 7zFM parity. Historical ARM64 QEMU checks
are separate evidence; those removed htopQt VMs are not the current runners.

## Validation on 2026-10-03

Checked the current worktree based on `4721f9da34abcaa7edea6940cf583711c1ba1457`
with runner tooling/docs additions; application sources were unchanged.

| Platform | Result |
| --- | --- |
| macOS arm64, Qt 6.11.2 | Full local gate: formatting/build and 207/207 CTest, 79.01 s |
| Ubuntu 24.04.5 x64, GCC 13.3 / Qt 6.4.2 | Identity/formatter, source hash, official runtime/SFX and formatting passed; app/test build failed before CTest |
| Windows 11 x64, MSVC 14.44 / Qt 6.8.3 | Identity/formatter, source hash, official runtime/SFX and formatting passed; app build failed before CTest |

Linux snapshot: `/home/runner/sevenzip-desktop/runs/20261003T083145Z-9873cd5b`.
The existing `tests/ui_update_modes_tests.cpp:189` uses `QTimeZone::UTC`, absent
in guest Qt 6.4.2. The full gate requires a newer matching Qt kit or a separately
reviewed compatibility change.

Windows snapshot: `C:\runner\sevenzip-desktop\runs\20261003T083237Z-a48e8d7b`.
The existing `src/qtexternalprocess.cpp:38` fails MSVC compilation because
`QueryFullProcessImageNameW` is undeclared. Windows API declarations/build
configuration need separate investigation. Full logs remain as `check.log` in
these snapshot directories; local run records correctly report failure.

Final readiness probes and five offline runner tests passed after localized
transport/logging changes; Python syntax and `git diff --check` passed. Host
storage monitoring reported no warnings after builds (about 770.8 GiB free on D:).
Neither guest Debug gate is accepted; Release was not attempted, and desktop,
distribution and visual parity remain unverified. No application fixes were made
as part of runner setup.

Review of commit `1adbeac31ad8f17e370a64d6ed3e1d102f1be6b7` after external
network access became available: both identity/toolchain probes and all five
offline helper tests passed. Installed Qt versions remain Linux 6.4.2 and
Windows 6.8.3. Network access alone does not resolve either recorded compilation
blocker. No packages were installed and the application gates were not rerun
during this documentation review.
