# Separate SDK and Linux/GPL archive-reader variants

User request, 2026-10-22: replace the numerous `SEVENZIP_LINUX_*` branches with
separate module variants. The split starts from repository commit
`17b72715e8385fd13d29b1eb4e969e4ce6a49030`. Original 7-Zip sources remain pinned to
<https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`; Linux read-side sources remain pinned to
<https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>, commit
`58161ead29e50d70a317ea26b472e0c7508e69da`. Original notices are retained; this is
a source/build separation, without changing the upstream licenses.

## Source ownership and transfer

SDK adaptations remain under `src/CPP/`. Linux reader variants live under
`src/gpl/CPP/`, with the same upstream-relative paths, filenames, classes and
symbols within each root. The existing [baseline](../upstream-baseline.json)
records both adapted variants and moved local integration units. Its uniqueness
check now applies per variant; duplicate owners within either variant remain
errors. Reusable upstream headers and implementations remain shared.

The split covers the conditional archive units for ext, FAT, NTFS, HFS,
cramfs, SquashFS, ISO, UDF, MBR and initramfs/CPIO. Both active source branches
are retained in concrete files, including the ISO/UDF headers needed by their
callers. GPL-only ISO/UDF integration dependencies move with those components.
The source selection was mechanical: resolve only the known Linux reader
switches in both directions, preserve unrelated preprocessor branches, then
format the resulting C++ units. SDK registration guards keep the original
`Z7_EXTERNAL_CODECS` condition. GPL registration is unconditional where the
former Linux switch selected it.

Relative includes in GPL units are anchored to the upstream-shaped include
root so they continue to select adapted portable dependencies. ISO/UDF module
include roots also select their own variant headers when an unchanged upstream
header includes a sibling. CMake chooses the concrete source files and no
longer defines the Linux reader switches. Ext password callback GUIDs have an
explicit module-only compilation unit instead of relying on a conditional
include in the shared module entrypoint. IUnknown remains initialized once by
that entrypoint, using the upstream GUID protocol.
The UDF input headers now include their registration-declaration dependency
explicitly, preserving compilation when include anchoring/formatting changes
the caller's prior include order.

## Runtime behavior and limits

The application's dynamic engine loads the existing Linux/GPL format modules
on Windows, macOS and Linux. Their Linux C readers and archive behavior are
preserved. The SDK engine snapshot and standalone client use the SDK source
variants; the dynamic core keeps its container/probe consumers without duplicate
filesystem registrations. `runtime_adapted` cannot select a GPL-root source:
the snapshot preparer rejects that before touching an existing snapshot.

No new decoder, filesystem feature, recovery mode or runtime selection is added.
Existing engine/profile differences and read-only limitations remain as recorded
in each format's owning audit. Platform compilation guards, header guards and
the limited container/probe/frame build roles remain ordinary source concerns.
They do not choose between the SDK and Linux filesystem implementations.

## Validation scope

Source comparison checks both concrete variants against the corresponding
pre-split branch, accounting separately for include anchoring and formatting.
A provenance/snapshot regression verifies that both variants can be recorded
while only the SDK adaptation is overlaid into an engine snapshot, and that an
attempt to overlay the GPL adaptation leaves the existing snapshot unchanged.
Existing format CLI/ABI/stream suites and codec consumers cover the resulting
modules and dynamic/standalone engine registrations. Runtime acceptance is
recorded after those checks complete; source separation alone is not acceptance.

Validation, 2026-10-22: all 42 selected format/stream/codec/provenance suites
passed on macOS and Linux. Windows passed 41 in the same batch; the remaining
`linux_frames_stream` backend nested-extraction case reported a codec-discovery
timeout. The unchanged suite passed in isolation with its original timeouts and
the same incremental build/runtime (batch receipt `20261022T164830Z-22d5efc1`,
isolated Windows receipt `20261022T165924Z-01f8f7e6`). The initial timing failure
is retained as evidence, rather than reported as an entirely green Windows
batch. The local application build passed. All 39 concrete source/header units
were compared with their pre-split branches after include anchoring and
formatting; both UDF headers additionally have the explicit declaration include
described above. The complete provenance verification passed with both variants.

Existing runtime snapshots, build trees and compiler caches were reused; source
and necessary configuration changes caused normal incremental compilation.
Disposable format fixtures were absent on all three platforms after validation.
No full gate, slow suite, new native mount/device check, UI appearance acceptance
or release installation was run. This refactor does not close the parent Linux
filesystem/format audit.
