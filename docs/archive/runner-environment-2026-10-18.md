# Historical runner environment records

Extracted from the runner guide on 2026-10-18. These are dated installation and
diagnostic records, not setup defaults or instructions to repeat provisioning.
Current workflows: [Hyper-V runners](../hyperv-runners.md). Connection and tool
selection: [local configuration](../runner-configuration.md). Versions and paths
below describe earlier environments and may differ from the current kits.

## Runner script provenance

The helper's source revisions, adaptations and retained license notice are in
[third-party notices](../../THIRD_PARTY_NOTICES.md#tooling-notice).

## Service-desktop diagnosis

The 2026-10-10 native control diagnosis confirms this distinction: session 0's
service window station reports flags 0 and an invisible desktop. A disposable
Win32 window gains WS_VISIBLE after ShowWindow, yet IsWindowVisible remains
false, just as for the actual Qt startup window. Do not replace the visibility
assertion with a style-bit check to accept that environment. The
[diagnostic receipt](qt-port/upstream-port-audit.md#p3-03--windows-service-desktop-visibility-diagnosis-2026-10-10)
records the tested snapshot and retained probes; an active logon is still
required for interactive checks.

## Transport installation

Windows rsync 3.5.1 (protocol 33) was installed on request on 2026-10-03 in
`C:\runner\sevenzip-desktop\tools\rsync\msys64`, using the
[official MSYS2 portable setup](https://www.msys2.org/docs/ci/#other-systems)
and signed `pacman` packages (`pacman --noconfirm -Syu rsync`). The downloaded
official `msys2-base-x86_64-latest.tar.xz` had SHA256
`51612cb7ee5117b68bf342fb7885212537e19c2aa6bb8b989ba5498f39efcb22`.
The helper addresses `usr/bin/rsync.exe` explicitly and maps destination paths
to `/c/runner/...`. Its MSYS2 installation is separate from the shared Git Bash,
MSVC and Qt, and is used only for transport; builds retain Git Bash and MSVC.
Linux uses its installed `/usr/bin/rsync`; the Mac client needs `rsync` in PATH.
Checks do not install or upgrade packages automatically.

## Desktop and network recovery

Native desktop availability is separate from SSH readiness. On2026-10-14 the
existing Windows credential was used only inside the host's anton process to
enter the observed Runner console password form through Msvm_Keyboard. This
normal login required no autologon, password reset, RDP enablement or reboot.
Do not print credentials or include them in command arguments/logs. The
[native acceptance receipt](qt-port/upstream-port-audit.md#p3-03p3-04--native-linux-package-workflow-and-desktop-recovery-2026-10-14)
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
exclusions are specified in [the synchronization contract](../hyperv-runners.md#persistent-worktree).

## Toolchain installations

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
toolchain includes `VS`, `Python`, `Git` and `Qt\6.8.3\msvc2022_64`.
Sevenzip uses `vcvars64.bat` and `cl.exe` for C/C++;
the shared clang-cl selection is not inherited. PATH and Qt/module discovery are
scoped to the check process. The helper does not change shared packages or toolchains.
Both guests build with four jobs and run CTest sequentially.

## Compiler cache and formatter installations

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
18 in the shared environment rejects existing source formatting before compilation.
Separate Python virtual environments contain the pinned PyPI formatter under
`/home/runner/sevenzip-desktop/tools/formatter` and
`C:\runner\sevenzip-desktop\tools\formatter`. Initial Linux provisioning also
installed `python3-venv`; existing compilers/Qt and the shared formatter remain in place.
To recreate the formatter, run the guest's configured Python with
`-m venv <directory>`, then that venv's
Python with `-m pip install --no-cache-dir clang-format==23.1.2`.
