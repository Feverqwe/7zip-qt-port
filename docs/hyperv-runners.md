# Hyper-V runners

Sevenzip Desktop reuses `runner-linux` and `runner-windows` on Windows 11
Hyper-V host `192.168.1.14`, host SSH account `anton`. VM storage stays under
`D:\runner`; Windows guest `C:` is inside its VHDX on that disk. These are the
same shared x64 guests used by htopQt, with separate project snapshot directories.

## Storage policy (user instruction, 2026-10-07)

Do not create or attach new virtual disks, including VHD/VHDX files, to solve
capacity problems. Do not grow disks, repartition or reformat volumes without
a separate user request. Use the existing configured volumes; the historical
data-disk creation recorded below is not authorization to repeat it.

Before substantial builds, check current free space on the host and selected
guest volume. If space is insufficient, delete obsolete Sevenzip Desktop run
payloads, build outputs and temporary data, then measure again. This project
cleanup is explicitly authorized by the user and takes precedence over the
global runners skill's default restriction on automatic deletion for warnings.
It does not authorize compaction or VM storage reconfiguration.

Identify completed runs from their metadata and check running processes,
interactive scheduled tasks and workspace references before deleting anything.
Keep the three newest runs per project workspace, every active or referenced
run (including an rsync basis in use), and artifacts needed for current review
or failure diagnosis. In older inactive runs, retain small logs and metadata;
remove reproducible source snapshots, runtime/build trees and run-local temp
files. Apply this to both legacy and configured project workspaces. Preserve
toolchains, installed Qt/formatter/rsync, other projects and unrelated data.
Record removed paths and free space before/after. If safely removable data is
insufficient, report the remaining capacity requirement; do not add a disk.

## Runner configuration and evidence

Fresh Linux and Windows runs enable compact test binaries through the build
wrapper; [development](development.md#local-commands) describes the retained
diagnostics and the opt-out for debugger rebuilds. This reduces per-run artifacts;
completed snapshots still require the storage cleanup policy above.
Default runner gates also exclude tests labeled `slow`. Long suites require an
explicit request and opt-in as described in AGENTS.md; do not count excluded
matrices as platform acceptance.
The helper forwards `SEVENZIP_INCLUDE_SLOW_TESTS` (0 by default; only 0/1 are
accepted) to both guest environments. Prefix a specifically requested exhaustive
runner command with `SEVENZIP_INCLUDE_SLOW_TESTS=1` to opt in.

The helper starts from htopQt's `scripts/hyperv-runner.py` at commit
`f57b93a72d765f738a6b779b4b3c94e16c0ef64a`, adapted for Sevenzip resources,
MSVC, pinned engine builders, formatter and SSH stdin transport.
CLI, parallel probes, shared Bash command payload and discovery-error reporting
were refreshed from htopQt's modified worktree on 2026-10-04 (base commit
`56320ebb4d62b3bac83f38719278cae6a5c454c1`), retaining these Sevenzip adaptations.
The source script's SHA256 was
`f4f53f0123a503dcc4fb85f17821d79e1cf156f29cbe015b99d561b45b1903ba`.

Current runner selection follows CI's Qt6.12.0. It explicitly sets
`CMAKE_PREFIX_PATH` and prepends the selected kit's bin directory:
`/home/runner/sevenzip-desktop/tools/Qt/6.12.0/gcc_64` and
`C:/runner/sevenzip-desktop/tools/Qt/6.12.0/msvc2022_64`.
`probe` queries that kit's qmake rather than the system default. Install/select
the kit before checks; the helper does not download Qt or change global PATH.
The Linux project kit currently links to the already installed
`/home/runner/htopQt/qt612-release-20261004/qt`, including QtDBus. Windows kit
was installed with the repository's verified aqt adapter; both live probes
confirm6.12.0. Linux full240/240 passed; Windows runtime validation is pending.
Earlier systemQt6.4.2 and
WindowsQt6.8.3 results below remain historical evidence, not Qt6.12 checks.

Every fresh snapshot creates its own `runner-tmp` directory before extraction.
Its process environment sets `TEMP`, `TMP` and `TMPDIR` there, including Windows
archive extraction, MSVC setup and the interactive scheduled-task child.
Temporary files stay with that run and are excluded from its Git snapshot.
This does not change global environment variables, move old files or delete
completed runs; capacity still needs a live preflight before substantial builds.
Linux SSH checks default `QT_QPA_PLATFORM` to `offscreen` when it is unset;
an explicit platform selection and per-suite CTest overrides are retained.

### Linux project data volume

The existing private mount is `/mnt/sevenzip-desktop`, filesystem UUID
`9fd6574a-fe35-4af5-82fc-f1b16d78afb8`. Its setup and old capacity samples are
[historical evidence](history/hyperv-runner-evidence.md), not instructions to
create a disk or proof of current free space. Apply the storage policy above.

The ignored local `build/runners/hyperv/workspaces.json` selects the data root:

```json
{
  "linux": {
    "root": "/mnt/sevenzip-desktop/runs",
    "filesystem_uuid": "9fd6574a-fe35-4af5-82fc-f1b16d78afb8"
  },
  "windows": {"root": "C:/runner/sevenzip-desktop/runs"}
}
```

Without this file, both legacy roots remain the defaults. The loader accepts
only these project roots and canonical UUIDs. A selected data workspace checks
the actual mounted filesystem UUID before creating a snapshot or transferring
source; a missing or different mount fails without falling back to the OS disk.
Rsync basis selection stays within the selected workspace. Each run records its
directory and required UUID. The helper does not create, format or mount disks.

```sh
python3 scripts/hyperv-runner.py probe linux
python3 scripts/hyperv-runner.py probe windows
python3 scripts/hyperv-runner.py probe all
python3 scripts/hyperv-runner.py check linux
python3 scripts/hyperv-runner.py check windows
python3 scripts/hyperv-runner.py check all
python3 scripts/hyperv-runner.py check linux --preset release
python3 scripts/hyperv-runner.py check windows --preset release
```

Native Windows window/focus/Properties checks need the logged-on user's desktop,
not SSH's session0. Use `check windows --interactive`, `check all --interactive`,
or `run windows --interactive -- <project-script> [args]`. The runner creates a
unique `Sevenzip-check-...` scheduled task with the current runner identity and
an InteractiveToken, runs the same snapshot `check.cmd`/toolchain there, records
the session and exit code, then removes the completed task. It needs an existing
active runner logon; it does not log in, supply credentials or fall back to a
noninteractive desktop. Linux in `all` keeps its normal path. Session0 runs may
verify backend/COM data consumers, but hidden windows do not prove UI acceptance.

The 2026-10-10 native control diagnosis confirms this distinction: session 0's
service window station reports flags 0 and an invisible desktop. A disposable
Win32 window gains WS_VISIBLE after ShowWindow, yet IsWindowVisible remains
false, just as for the actual Qt startup window. Do not replace the visibility
assertion with a style-bit check to accept that environment. The
[diagnostic receipt](upstream-port-audit.md#p3-03--windows-service-desktop-visibility-diagnosis-2026-10-10)
records the tested snapshot and retained probes; an active logon is still
required for interactive checks.

Full Windows gates also include privileged native symlink/security fixtures.
Use `check windows --interactive --elevated` or `check all --interactive --elevated`
for those gates; custom validation wrappers accept the same flags before `--`.
`--elevated` selects `Highest` for this snapshot's owned scheduled task and checks
the actual administrator token before running the command. Session evidence
records requested run level and actual elevation. The default interactive mode
keeps `Limited`; noninteractive/Linux-only elevation requests are rejected.
No developer-mode, UAC, user-rights, account or machine policy is changed.

`native_menu_integration` exercises real per-user COM registration under an
ordinary user token. When privileged link/security fixtures require an elevated
parent gate, this test launches its own child using the desktop process as its
parent, following [Microsoft's parent-process API technique](https://devblogs.microsoft.com/oldnewthing/20190425-00/?p=102443).
It first verifies the same user SID and session and an unelevated desktop token;
the child independently checks elevation before registration and activation.
The fixture waits for its child, reports its assertions and removes only registry
entries bearing that child's ownership marker. It changes no UAC/security policy,
uses no alternate credentials and retains the ordinary-token path for the default
interactive runner. Its required Vista API declarations apply only to this test.

The general entry point is `run <linux|windows|all> [runner-options] --
<project-script> [script-arguments]`. Examples:

```sh
python3 scripts/hyperv-runner.py run all -- scripts/check.sh
python3 scripts/hyperv-runner.py run linux -- scripts/build.sh dev
python3 scripts/hyperv-runner.py run all --skip-runtime -- scripts/check_ui_parity.py --help
```

Scripts must be indexed project files in the synced directories, with paths
relative to the project root. `.sh` uses Bash on both platforms (Git Bash on
Windows); `.py` uses the guest Python; `.ps1` is supported for Windows only.
Arguments after `--` are passed literally, including spaces, Unicode and shell
metacharacters. Scripts run from the snapshot root in the existing platform
toolchain environment. `--skip-runtime` skips engine/SFX compilation when the
selected script does not need it; by default runtime preparation is retained.
Each action has its own `--help` and accepts only its applicable runner options.
`--preset` is accepted only by the `check` shortcut; `run` passes build/test options through
the script arguments. `scripts/build.sh` takes `dev` or `release` positionally.

The default transport is `rsync` over the verified SSH connection. The local
snapshot is an uncompressed `source.tar`; the latest successful rsync run's
archive is copied into the new guest run directory as a private delta basis.
Rsync transfers changed blocks, then the guest verifies the full archive SHA256
before extraction. The previous run is untouched, concurrent runs have separate
destinations, and deleted source files cannot survive extraction into the fresh
directory. The first run sends the full archive. `--transport scp` selects the
original gzip archive transport and does not require rsync.

Windows rsync 3.5.1 (protocol 33) was installed on request on 2026-10-03 in
`C:\runner\sevenzip-desktop\tools\rsync\msys64`, using the
[official MSYS2 portable setup](https://www.msys2.org/docs/ci/#other-systems)
and signed `pacman` packages (`pacman --noconfirm -Syu rsync`). The downloaded
official `msys2-base-x86_64-latest.tar.xz` had SHA256
`51612cb7ee5117b68bf342fb7885212537e19c2aa6bb8b989ba5498f39efcb22`.
The helper addresses `usr/bin/rsync.exe` explicitly and maps destination paths
to `/c/runner/...`. Its MSYS2 installation is separate from htopQt's Git Bash,
MSVC and Qt, and is used only for transport; builds retain Git Bash and MSVC.
Linux uses its installed `/usr/bin/rsync`; the Mac client needs `rsync` in PATH.
Checks do not install or upgrade packages automatically.

`all` creates and hashes the source archive once, then runs the two guests in
parallel. Failure on one guest does not prevent the other from completing.
The command exits nonzero if either fails. Each platform has `run.json`, a local
`transport.log`, and a downloaded full `check.log` when available under
`build/runners/hyperv/<run-id>/<platform>/`; the parent `run.json` records the
shared snapshot hash, source revision, command and aggregate result. Failure
output includes the last 20 log lines with a platform prefix. Log retrieval
errors are recorded separately from script results.
`probe all` also checks both guests in parallel, printing each guest's captured
output as one block and returning nonzero if either probe fails. Preparation
errors captured by discovery are retained in the run record and transport log.

Every invocation uses a fresh snapshot and build directory; it does not reuse
an earlier build. A script invoking CTest must build its targets first in that
same invocation, for example in a project script that calls `build.sh` followed
by `ctest.sh`. There is no `--run-id` reuse mode yet. Interrupting the client is
not a guaranteed remote process cancellation; parallel workers may finish
before the client exits. No automatic remote cleanup is performed.

The global skill at `~/.codex/skills/runners` must be installed. Its
`scripts/hyperv_ready.py <platform> --start` starts only the requested existing
guest when Off, rediscovers DHCP and verifies authenticated guest identity.
The project helper uses the returned `ssh_argv`, strict host-key checking,
the Mac key `~/.ssh/id_ed25519` and stable guest keys in
`~/.ssh/known_hosts_codex_runners`. Keys and credentials are never transferred.

## Source and runtime scope

Native desktop availability is separate from SSH readiness. On2026-10-14 the
existing Windows credential was used only inside the host's anton process to
enter the observed Runner console password form through Msvm_Keyboard. This
normal login required no autologon, password reset, RDP enablement or reboot.
Do not print credentials or include them in command arguments/logs. The
[native acceptance receipt](upstream-port-audit.md#p3-03p3-04--native-linux-package-workflow-and-desktop-recovery-2026-10-14)
records Session1 and the unchanged logon settings.

After a Linux kernel update, missing matching `linux-cloud-tools-$(uname -r)`
can make hv-kvp-daemon fail and leave Hyper-V's reported IP list empty even
while guest SSH works. Verify the exact VM MAC/host ARP mapping and pinned SSH
identity before a fallback connection; do not guess a new address or reconfigure
networking. On2026-10-14 installing the missing6.8.0-146 package restored KVP.
The same receipt records the temporary real-console Xorg/Openbox/PCManFM scope.
Linux bulk TypeText lost input events in that setup; paced TypeKey input and
inspection of the actual destination preceded the accepted transfer.

Each check creates a fresh `/home/runner/sevenzip-desktop/runs/<id>` or
`C:\runner\sevenzip-desktop\runs\<id>`. The SHA256-verified archive contains
current tracked project sources, resources, tests, docs, skills and build
configuration, including modifications and excluding deleted files. Add new
files to the Git index before running; the runner and its transport helper are
included explicitly.
Untracked project files are listed as omitted with a reminder to use `git add`;
selecting an untracked script is rejected before runner startup. Ignored state
and the project's `.git` are excluded.

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
They also cover a shared snapshot with independent platform failures and
literal command argument transport through Linux and the Windows Bash payload.
An actual local rsync delta fixture checks reconstructed bytes and preservation
of the previous basis; transport tests require local Bash, Git and rsync.
Runner orchestration tests remain outside the application's CTest gate.

SSH checks cover the project gate, not interactive desktop/Finder behavior,
distribution or rendered Windows 7zFM parity. Historical ARM64 QEMU checks
are separate evidence; those removed htopQt VMs are not the current runners.

## Validation on 2026-10-03

[Historical evidence](history/hyperv-runner-evidence.md#validation-on-2026-10-03).

## General entry point validation on 2026-10-03

[Historical evidence](history/hyperv-runner-evidence.md#general-entry-point-validation-on-2026-10-03).

## CLI refresh validation on 2026-10-04

[Historical evidence](history/hyperv-runner-evidence.md#cli-refresh-validation-on-2026-10-04).
