# Hyper-V runners

Connections, machine names, workspaces and tools are selected through
[local runner configuration](runner-configuration.md). The helper supports
direct SSH or an optional VM discovery command, with one persistent Sevenzip
worktree per platform. Run IDs identify logs, not new source/build directories.
Historical installation records are linked from the archive; use local
configuration for the identities and paths of your installation.

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
top-level build configuration. Translation catalogs travel with the application
sources so localized checks use the same build inputs on every platform.
Deleted files disappear remotely. No staging or
commit is required. Local `.git`, `.codex`, credentials, build outputs and other
ignored state are not sent. The guest's `.git/`, `build/`, `ui-reports/`,
`runner-tmp/`, `upstream/` and runner control/log files are protected from deletion.
Do not use `--delete-excluded`. A SHA256 manifest verifies every supplied source
file/link before the command runs; unsafe external links are refused. The
manifest describes code at invocation start, not edits made during the run.
Matching files are skipped, including on repeated runs. The archive-mode subset
`-rlp` preserves recursive content, links and permissions; owner/group
and special-device replication are deliberately omitted across Linux/Windows.
Changed files receive the guest's current modification time so incremental
builds detect edits made before an earlier snapshot finished compiling.
Unchanged contents retain their existing guest timestamps and build outputs.

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
data-disk creation recorded in the archive is not authorization to repeat it.

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

`CMAKE_BUILD_PARALLEL_LEVEL` is forwarded to guest builds when set to a positive
integer. Use `1` when a guest runs out of memory during compilation; this
retains its incremental build and compiler cache.

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

Runner script provenance is retained in the [dated environment record](archive/runner-environment-2026-10-18.md#runner-script-provenance).

Runner selection sets `CMAKE_PREFIX_PATH` and prepends the kit and tool
directories from local configuration.
`probe` queries that kit's qmake rather than the system default. Install/select
the kit before checks; the helper does not download Qt or change global PATH.
Use `probe` to confirm the live kit; prior probe/gate results are
[historical evidence](archive/qt-port/workflow-evidence.md).

The persistent worktree contains `runner-tmp`. Its process environment sets
`TEMP`, `TMP` and `TMPDIR` there before MSVC setup and the interactive child.
Temporary files stay in the project workspace and are excluded from its Git baseline.
This does not change global environment variables, move old files or delete
completed runs; capacity still needs a live preflight before substantial builds.
Linux SSH checks default `QT_QPA_PLATFORM` to `offscreen` when it is unset;
an explicit platform selection and per-suite CTest overrides are retained.

### Workspace and Linux data volume

The local configuration explicitly selects each workspace. An optional Linux
mount/UUID pair is checked before acquiring the worktree or transferring source;
a missing or different mount fails without falling back to another disk.
See [configuration and migration](runner-configuration.md). The helper does not
create, format or mount disks. Prior mount setup and capacity samples remain
[historical evidence](archive/qt-port/history/hyperv-runner-evidence.md).

```sh
python3 scripts/hyperv-runner.py probe linux
python3 scripts/hyperv-runner.py probe windows
python3 scripts/hyperv-runner.py probe all
python3 scripts/hyperv-runner.py check linux
python3 scripts/hyperv-runner.py check windows
python3 scripts/hyperv-runner.py check all
python3 scripts/hyperv-runner.py run all -- scripts/build.sh release
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

The [service-desktop diagnosis](archive/qt-port/upstream-port-audit.md#p3-03--windows-service-desktop-visibility-diagnosis-2026-10-10)
explains why hidden windows in session 0 cannot establish UI acceptance.

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

Install rsync on the client and both guests, then select the guest executable
paths in [local configuration](runner-configuration.md). Windows transport uses
MSYS-compatible destination paths; builds use the configured Bash and MSVC tools.
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

An optional readiness command can start an existing VM, rediscover its address
and verify guest identity. The maintainer uses the separately installed runners
skill; direct SSH configurations do not require it. The helper uses the
configured SSH argument array and normal host-key verification. Keys and
credentials are never transferred.

## Source and runtime scope

Native desktop availability is separate from SSH readiness. Interactive checks
require an active desktop session as described above. Historical login and network
diagnoses are retained in the [environment record](archive/runner-environment-2026-10-18.md#desktop-and-network-recovery).

Each check updates `<selected-workspace>/current`. Source scope, untracked files
and exclusions are specified in [the synchronization contract](#persistent-worktree).

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
Use the `dev` preset for tests and `run ... -- scripts/build.sh release` for
a Release build; the Release preset disables test targets. Connection records and per-run SHA256,
project revision, directory, preset and status stay in ignored
`build/runners/hyperv/` on the Mac. Local source/toolchain preparation failures
may occur before a remote run record exists. No automatic cleanup, VM creation,
package installation, shutdown or publishing is performed.

## Toolchains

Select installed Qt, compiler, formatter and build tools through
[local configuration](runner-configuration.md), and use `probe` to inspect the
selected kit. Install dependencies before running checks; the helper does not
provision toolchains. Linux builds need the dependencies listed in
[development](development.md). Historical kit versions and installation records
are retained in the [environment record](archive/runner-environment-2026-10-18.md#toolchain-installations).

Ccache is selected automatically by `build.sh` for C/C++ (and Objective-C++
where applicable). All runner checks, including full gates, follow the
[cache-preserving validation rules](../AGENTS.md#validation-scope); a full-check
request does not authorize clearing or bypassing ccache or recreating build trees.
Keep compiler caches outside the synchronized source tree and select their
paths through local configuration. MSVC uses embedded `/Z7` debug information
through CMake CMP0141 so cached objects retain debugging information.
Use a formatter compatible with the repository's formatting configuration,
in a separate tool environment when other projects require a different version.

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
