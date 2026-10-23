# Selected native reader transfer

Source branch: `local-dev` at `d439ffdf29a0e3960942af7449aef2609b7c5761`.
Target baseline: `0d997ccad5e2500813aaf5f999500e631e76f404`.
This record covers the selected transfer requested on 2026-10-23; universal
extraction replay and a blanket restriction on nested editing are excluded.

## Progress controls

The completed transfer is recorded in [progress controls](progress-controls.md).

## Native enumeration and testing

Sources: `3e276e74cc04b80c11c1276827ed2f9640b6f3eb`,
`ae78a45a02f17a04c88a3514558f55cdba29aab6` and
`8c0c78acaecd82060dbc0ca2eaefe12ef2ecc2e1`, using their final combined
`QtArchiveReadOperation` rather than intermediate workers. Upstream source:
<https://github.com/ip7z/7zip.git>,
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`,
`CPP/7zip/UI/Common/OpenArchive.cpp` (`CArchiveLink::Open`, `CArc` properties),
`ArchiveExtractCallback.cpp` (`GetStream`, `SetOperationResult`,
`ReportExtractResult`, `RequestMemoryUse`), `CPP/7zip/UI/Agent/Agent.cpp`
and `CPP/7zip/UI/Console/List.cpp` (`GetPropName`, `PrintTime`,
`PrintItemInfo`, `PrintPropertyPair2`, `Print_OpenArchive_Props`).
The test-callback projection in the source branch originally cites
`b88c6bc09339de32f7d18bbab900686b244f42ba`; the relevant bodies and dependencies
were compared again with the available 9128 baseline.

Copied the Console property-name table and retained its technical property/time,
attribute and raw NT metadata conversion. Omitted console drivers, formatting
and output transport. Qt owns typed snapshots and worker delivery. Open/Extract,
format detection, item mappings and decoding remain official engine operations.
The worker uses original item/block error projection, an operation-scoped
password service and cooperative pause/cancel checkpoints; memory requests keep
the already allowed engine budget. No memory-limit consent is invented for the
standalone non-dialog API. Open diagnostics do not turn a partial test into a
successful one, and incomplete/cancelled listing snapshots are not published.

Folder navigation keeps the native catalog/reader instead of reopening through
CLI text listing. This retains distinct indexes for duplicate member names.
The Console projection belongs only to `adapted`, not `runtime_adapted`:
the official runtime retains its original console module. Existing CLI fallback
remains for no-catalog consumers. Materialized nested editing remains enabled;
automatic direct container chains retain their existing update boundary.

Focused runtime acceptance on 2026-10-23: macOS passed native listing/test,
opening/options, native folder and RAR-memory consumers, update and nested
editing scenarios. The complete application target also built successfully.
Linux passed the 13-suite selected set in `20261023T181315Z-e4d9023f`;
Windows passed that set in desktop session `20261023T181906Z-5145d185`.
The Windows SSH session-0 attempt was not accepted: codec discovery and archive
progress required the desktop repeat. Test-only corrections cover canonical
macOS filenames, actual native indexes, unavailable CLI and repeated dismissal
of source error dialogs; existing nested-edit behavior checks are retained.
No slow suites or full gate were run. These checks accept file-based native
enumeration/testing; direct stream chains and retained generations are separate
scope below and are not established by this result.

## QCOW

Transferred the QCOW portion of
`d439ffdf29a0e3960942af7449aef2609b7c5761`. Upstream reference:
<https://github.com/ip7z/7zip.git>,
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`,
`CPP/7zip/Archive/QcowHandler.cpp`, `HandlerCont.cpp/.h`,
`CPP/7zip/Archive/SquashfsHandler.cpp`, `C/ZstdDec.c/.h` and
`CPP/7zip/Common/CWrappers.cpp/.h`.

Compared the complete QCOW handler, image handler callers, tables, stream and
decoder dependencies. Formatting is separate from functional adaptation:
the handler retains upstream mappings, Deflate, sparse/zero reads and bounds
checks. The additional Zstandard cluster path uses the official caller-buffer
decoder, stops at the frame boundary before sector padding, and requires a full
cluster without extra output. External-data, extended-L2 and unknown incompatible
layouts are rejected; backing files and encryption retain their limitations.

One SDK adaptation is registered in both `adapted` and `runtime_adapted` in the
baseline manifest. Normal runtime preparation overlays that source onto the
official engine build; no second QCOW implementation is introduced. The existing
official decoder and LGPL notices remain authoritative. Test images use the
existing disposable MBR/ext4 source; the Zstandard image was produced by QEMU.

Runtime evidence: macOS, Linux and Windows `archive_qcow` pass all nine cases, including
stored/Deflate/Zstandard source-byte comparison, sparse tail and backward reads,
unsupported layout rejection and corrupted cluster failure. Linux and Windows
receipts are the focused runs above. Stream-chain validation remains pending.
