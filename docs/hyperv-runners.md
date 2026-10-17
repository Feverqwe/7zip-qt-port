# Hyper-V runners

Sevenzip Desktop reuses `runner-linux` and `runner-windows` on Windows 11
Hyper-V host `192.168.1.14`, host SSH account `anton`. VM storage stays under
`D:\runner`; Windows guest `C:` is inside its VHDX on that disk. These are the
same shared x64 guests used by htopQt, with one persistent Sevenzip worktree
per platform. Run IDs identify logs, not new source/build directories.

## Persistent worktree

User instruction, 2026-10-17: synchronize the local current code into one guest
directory and reuse its build outputs; do not create per-run build directories.
This supersedes the former fresh-snapshot instructions, including the global
runners skill's default fresh-directory workflow.

The selected workspace root contains `current/`, with `build/dev`, `build/release`,
runtime and temporary files retained there. Existing dated runs are historical;
the retention policy below still applies to their disposable payloads. Preserve
`current`, its incremental outputs, compiler caches and any active lock.

Before transfer, an atomic `current.lock` directory reserves the entire cycle,
including synchronization, build/tests and log retrieval. A second invocation
fails with the owner run ID before touching source. An existing nonempty
`current` without the project ownership marker is refused.

`rsync --checksum --compress --compress-level=1 --delete-delay --delay-updates`
reconciles an immutable local
copy of the current worktree. It includes modified tracked files and nonignored
new files in the allowlisted source/resource/test/doc/script directories and
top-level build configuration. Deleted files disappear remotely. No staging or
commit is required. Local `.git`, `.codex`, credentials, build outputs and other
ignored state are not sent. The guest's `.git/`, `build/`, `ui-reports/`,
`runner-tmp/`, `upstream/` and runner control/log files are protected from deletion.
Do not use `--delete-excluded`. A SHA256 manifest verifies every supplied source
file/link before the command runs; unsafe external links are refused. The
manifest describes code at invocation start, not edits made during the run.
Matching files are skipped, including on repeated runs. The archive-mode subset
`-rlpt` preserves recursive content, links, permissions and times; owner/group
and special-device replication are deliberately omitted across Linux/Windows.

After confirmed command exit the lock is released, including on a normal test
failure. An interrupted or disconnected invocation retains it if execution
completion is unknown. Interrupting the client is not guaranteed cancellation
of guest processes. Inspect with:

```sh
python3 scripts/hyperv-runner.py status all
python3 scripts/hyperv-runner.py recover windows
```

`recover` only releases the recorded owner's lock when its completion marker
matches. It never steals a live/unknown lock. If synchronization or the shell
died before writing completion, inspect guest processes, scheduled tasks and
the recorded owner before manually releasing that exact abandoned lock; age
alone is insufficient. The next normal sync repairs a partial source transfer.

## Storage policy (user instruction, 2026-10-07)

Do not create or attach new virtual disks, including VHD/VHDX files, to solve
capacity problems. Do not grow disks, repartition or reformat volumes without
a separate user request. Use the existing configured volumes; the historical
data-disk creation recorded below is not authorization to repeat it.

Before each new validation batch, inspect completed runs and remove obsolete
Sevenzip Desktop run payloads, build outputs and temporary data under the
retention rules below, even when space is sufficient. Before substantial builds,
also check current free space on the host and selected guest volume; measure
again after cleanup. This project
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

Linux and Windows runs enable compact test binaries through the build
wrapper; [development](development.md#local-commands) describes the retained
diagnostics and the opt-out for debugger rebuilds. This reduces per-run artifacts;
historical completed snapshots still require the storage cleanup policy above.
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

Runner selection explicitly sets
`CMAKE_PREFIX_PATH` and prepends the selected kit's bin directory:
`/home/runner/sevenzip-desktop/tools/Qt/6.12.0/gcc_64` and
`C:/runner/sevenzip-desktop/tools/Qt/6.12.0/msvc2022_64`.
`probe` queries that kit's qmake rather than the system default. Install/select
the kit before checks; the helper does not download Qt or change global PATH.
The Linux project kit currently links to the already installed
`/home/runner/htopQt/qt612-release-20261004/qt`, including QtDBus. Windows kit
was installed with the repository's verified aqt adapter. Use `probe` to confirm
the live kit; prior probe/gate results are [historical evidence](archive/qt-port/workflow-evidence.md).

The persistent worktree contains `runner-tmp`. Its process environment sets
`TEMP`, `TMP` and `TMPDIR` there before MSVC setup and the interactive child.
Temporary files stay in the project workspace and are excluded from its Git baseline.
This does not change global environment variables, move old files or delete
completed runs; capacity still needs a live preflight before substantial builds.
Linux SSH checks default `QT_QPA_PLATFORM` to `offscreen` when it is unset;
an explicit platform selection and per-suite CTest overrides are retained.

### Linux project data volume

The existing private mount is `/mnt/sevenzip-desktop`, filesystem UUID
`9fd6574a-fe35-4af5-82fc-f1b16d78afb8`. Its setup and old capacity samples are
[historical evidence](archive/qt-port/history/hyperv-runner-evidence.md), not instructions to
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
the actual mounted filesystem UUID before acquiring the worktree or transferring
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
an InteractiveToken, runs the worktree's `check.cmd`/toolchain there, records
the session and exit code, then removes the completed task. It needs an existing
active runner logon; it does not log in, supply credentials or fall back to a
noninteractive desktop. Linux in `all` keeps its normal path. Session0 runs may
verify backend/COM data consumers, but hidden windows do not prove UI acceptance.

Interactive tasks explicitly request Task Scheduler priority4 (normal process
priority); its default7 starts a background process and affects child fixtures.
The child records and verifies actual `Priority=Normal` before `check.cmd`, and
sets `SEVENZIP_RUNNER_EXPECT_NORMAL_PRIORITY=1` so `process_priority` also checks
the inherited CTest process context. This is scoped to the owned task, in both
Limited and Highest modes; it changes no global scheduler or account setting.
See [Microsoft's priority mapping](https://learn.microsoft.com/en-us/windows/win32/taskschd/tasksettings-priority)
and the [native regression receipt](archive/qt-port/upstream-port-audit/p3-03-runner-priority-2026-10-16.md).

The 2026-10-10 native control diagnosis confirms this distinction: session 0's
service window station reports flags 0 and an invisible desktop. A disposable
Win32 window gains WS_VISIBLE after ShowWindow, yet IsWindowVisible remains
false, just as for the actual Qt startup window. Do not replace the visibility
assertion with a style-bit check to accept that environment. The
[diagnostic receipt](archive/qt-port/upstream-port-audit.md#p3-03--windows-service-desktop-visibility-diagnosis-2026-10-10)
records the tested snapshot and retained probes; an active logon is still
required for interactive checks.

Full Windows gates also include privileged native symlink/security fixtures.
Use `check windows --interactive --elevated` or `check all --interactive --elevated`
for those gates; custom validation wrappers accept the same flags before `--`.
`--elevated` selects `Highest` for this invocation's owned scheduled task and checks
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

Scripts must be nonignored project files in the synced directories, with paths
relative to the project root. `.sh` uses Bash on both platforms (Git Bash on
Windows); `.py` uses the guest Python; `.ps1` is supported for Windows only.
Arguments after `--` are passed literally, including spaces, Unicode and shell
metacharacters. Scripts run from `current` in the existing platform
toolchain environment. `--skip-runtime` skips engine/SFX preparation when the
selected script does not need it; by default runtime preparation is retained.
Each action has its own `--help` and accepts only its applicable runner options.
`--preset` is accepted only by the `check` shortcut; `run` passes build/test options through
the script arguments. `scripts/build.sh` takes `dev` or `release` positionally.

The source transport is directory rsync over the verified SSH connection, with
deletion/protection and locking as described [above](#persistent-worktree).
Archive/scp source transport is removed. Scp is used only to retrieve logs.

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

`all` materializes and hashes current source once, then runs the two guests in
parallel. Failure on one guest does not prevent the other from completing.
The command exits nonzero if either fails. Each platform has `run.json`, a local
`transport.log`, and a downloaded full `check.log` when available under
`build/runners/hyperv/<run-id>/<platform>/`; the parent `run.json` records the
shared manifest hash, source revision, command, phase timings and aggregate result. Failure
output includes the last 20 log lines with a platform prefix. Log retrieval
errors are recorded separately from script results.
`probe all` also checks both guests in parallel, printing each guest's captured
output as one block and returning nonzero if either probe fails. Preparation
errors captured by discovery are retained in the run record and transport log.

Every invocation reuses `current/build/<preset>`. CTest still needs its targets
built after synchronization: use `verify.py`, or a script calling `build.sh`
followed by `ctest.sh`. There is no per-run build mode. Temporary local source
copies are removed after transfer/execution; only logs and metadata get run IDs.

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
[native acceptance receipt](archive/qt-port/upstream-port-audit.md#p3-03p3-04--native-linux-package-workflow-and-desktop-recovery-2026-10-14)
records Session1 and the unchanged logon settings.

After a Linux kernel update, missing matching `linux-cloud-tools-$(uname -r)`
can make hv-kvp-daemon fail and leave Hyper-V's reported IP list empty even
while guest SSH works. Verify the exact VM MAC/host ARP mapping and pinned SSH
identity before a fallback connection; do not guess a new address or reconfigure
networking. On2026-10-14 installing the missing6.8.0-146 package restored KVP.
The same receipt records the temporary real-console Xorg/Openbox/PCManFM scope.
Linux bulk TypeText lost input events in that setup; paced TypeKey input and
inspection of the actual destination preceded the accepted transfer.

Each check updates `<selected-workspace>/current`, normally
`/home/runner/sevenzip-desktop/runs/current` or
`C:\runner\sevenzip-desktop\runs\current`. Source scope, untracked files and
exclusions are specified in [the synchronization contract](#persistent-worktree).

The helper also includes an upstream Git bundle at the exact revision recorded
in `docs/upstream-baseline.json`. `--upstream /path/to/7zip` overrides the default
read-only sibling checkout. A temporary local mirror creates the bundle ref;
the original checkout's refs, index and files remain untouched. No upstream
network fetch is needed. Bundles are cached locally by revision. The guest keeps
a bare `upstream/` object store and fetches the supplied bundle only when its
pinned commit is missing. `prepare-runtime.py` verifies a toolchain/builder/source
fingerprint and artifact hashes before reusing the dynamic engine, codec module,
standalone client and SFX. Misses invoke the official project builders; temporary
upstream build trees are removed by those builders. All runtime binaries match
the guest architecture and pinned source. Environment changes are process-local.

Before transfer, the helper checks actual local staged/worktree diffs for
whitespace errors. The guest updates its local Git baseline for the full
gate, so original upstream notices with historical whitespace are baseline
content rather than newly staged additions. The client's Git index is unchanged. Runtime
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

Ccache is selected automatically by `build.sh` for C/C++ (and Objective-C++
where applicable). All runner checks, including full gates, follow the
[cache-preserving validation rules](../AGENTS.md#validation-scope); a full-check
request does not authorize clearing or bypassing ccache or recreating build trees.
Linux has `/usr/bin/ccache` 4.9.1; Windows has project-owned
`C:/runner/sevenzip-desktop/tools/ccache/ccache.exe` 4.14.1, installed on request
on 2026-10-17 from the [official release](https://ccache.dev/download.html).
The Windows ZIP SHA256 is
`6219f3865ca59aec41ee4b678df171d5d35855ecb2b6dbbbd20690b3a68af7b4`;
`tools/ccache/installation.json` records its URL/hash. A real MSVC two-compile
probe produced one miss followed by one direct hit. Windows PATH changes are
scoped to the runner environment. MSVC uses embedded `/Z7` debug information
through CMake CMP0141, retaining debug information while allowing cached objects.
Project caches live at `<project-root>/cache/ccache`, outside `runs/current`,
with a process-local 2GB limit. No existing shared compiler cache is deleted.

Sevenzip uses clang-format 23.1.2, matching the local source formatting. Version
18 in htopQt's environment rejects existing source formatting before compilation.
Separate Python virtual environments contain the pinned PyPI formatter under
`/home/runner/sevenzip-desktop/tools/formatter` and
`C:\runner\sevenzip-desktop\tools\formatter`. Initial Linux provisioning also
installed `python3-venv`; existing compilers/Qt and htopQt's formatter remain in place.
To recreate the formatter, run the guest's `python3 -m venv <directory>` (Windows:
`C:\runner\htopqt\Python\python.exe -m venv <directory>`), then that venv's
Python with `-m pip install --no-cache-dir clang-format==23.1.2`.

`python3 scripts/verify.py tooling` runs the registered offline workflow, runner,
runtime-cache, CTest selection and output suites. They cover a first build with
missing executables, fixture closure, incremental reuse, empty/slow selections,
Unicode/current/untracked/deleted source, actual rsync deletion and protected
outputs, lock ownership/completion and failure/context/quoting behavior. These
tests use temporary deterministic local fixtures and no VM/network access;
they require Bash, Git, CMake/Ninja, a compiler and rsync.

SSH checks cover the project gate, not interactive desktop/Finder behavior,
distribution or rendered Windows 7zFM parity. Historical ARM64 QEMU checks
are separate evidence; those removed htopQt VMs are not the current runners.

## Historical evidence

Early runner, entry-point and CLI checks are retained in
[the runner archive](archive/qt-port/history/hyperv-runner-evidence.md).
