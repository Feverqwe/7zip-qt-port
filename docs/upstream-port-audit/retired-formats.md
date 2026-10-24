# Retired format support — 2026-10-24

The user selected these compatibility reductions to reduce maintained format
code. The same-day review narrowed the ZIP change to creation: legacy ZIP
reading is retained. This is an intentional product difference from upstream
7-Zip, not an upstream decoder limitation or a completed library migration.

| Removed scope | Retained scope and source decision |
| --- | --- |
| SquashFS 1–3, big-endian and patched legacy magic/layouts | Standard little-endian SquashFS 4.0 and its existing zlib/LZO/LZ4/XZ/Zstd paths. Removed old superblock/inode/directory/ID/fragment parsing, eager metadata and block maps, heuristic LZMA selection and private LZO decoder. The Linux v4 metadata services and partial-access behavior stay connected. |
| Plain classic HFS | HFS+, HFSX and HFS wrappers containing those volumes. Removed `OpenClassic`, its C input adapter and the separate `src/fs/hfs` read implementation. Wrapper bounds/protection and HFS+ metadata, forks, compression and hardlink readers remain. |
| ZIP PPMd creation | Reading Shrink, Implode, PPMd, LZFSE and Zstandard method 20 remains supported. Zstandard creation uses method 93. Explicit PPMd writer requests are rejected before output; saved PPMd UI profiles fall back to a supported method. WinZip AES and other retained methods remain supported. |
| CramFS | Removed both handlers, C reader, module/build/install/device integration and dedicated positive suites. A generated valid image is retained as a rejection fixture. |
| High Sierra | ISO9660, Joliet, Rock Ridge, zisofs, logical blocks, multi-extent files and El Torito remain. Removed the CDROM descriptor probe, record-layout conversion and high-Sierra arguments/state from the Linux C boundary. |

## Source comparison and ownership

7-Zip original baseline: `b88c6bc09339de32f7d18bbab900686b244f42ba`,
retrievable from <https://github.com/ip7z/7zip.git>. Before edits, compared the
selected handler branches, their data types, codec creation, ZIP writer property
resolution, UI method selection, CMake consumers and Console/Format7zF makefile
closure. After removal, reviewed the remaining v4 and HFS+ branches and checked
that removed formats cannot be supplied by the built-in runtime fallback.

ZIP `ZipHandler.cpp` and `ZipHandlerOut.cpp` are adapted owners under
`src/CPP/7zip/Archive/Zip`; their original hashes stay pinned in
`upstream-baseline.json`. The decoder matches the pinned upstream body except
that method 20 shares the existing method 93 Zstandard decoder. Writer selection
rejects PPMd (and the already unsupported legacy writer IDs) before output.
`ZipUpdate.cpp` returns to an unchanged dependency, and `ZipAddCommon.cpp`
returns to its pre-removal adaptation. This avoids maintaining redundant changes
to encoding and resource calculations behind the writer gate.

The complete original `CPP/7zip/Bundles/Format7zF/Arc.mak` and `Arc_gcc.mak`
units are newly pinned by original SHA256 in that manifest, at the same commit,
and adapted under the corresponding `src` paths. Only the CramFS and SDK
SquashFS handler entries are removed. Both runtime builders consume the
overlays, including Console, so the sidecar has the same reduced format scope.
The SDK SquashFS handler is omitted entirely from these bundles: the deployed
GPL v4 module is the authoritative reader. No GPL source is copied into the SDK
runtime overlay. No sibling upstream files were changed.

Configuration also retires the former project-built `sevenzip_cramfs` plugin
from this checkout's generated runtime, so incremental builds cannot rediscover
that stale module. External runtime directories are not modified.

Shrink, Implode and PpmdZip units are restored unchanged from the same pinned
baseline, with their existing hashes/revision recorded in the manifest.
The complete upstream PpmdZip unit contains both decoder and encoder bodies;
its Ppmd8 encoding dependency is retained to avoid splitting/forking the unit.
The public ZIP writer gate, including the sidecar, disables PPMd creation.
Standalone `.pmd`, PPMd for 7z and shared LZFSE/LZO/LZMA/Deflate/XZ/Zstd remain.

Linux source baseline remains
`58161ead29e50d70a317ea26b472e0c7508e69da` from
<https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git>.
The HFS and CramFS manifests retain complete original inventory hashes for
historical/revision review, marking removed adaptation owners as retired.
The ISO and SquashFS originals/hashes are unchanged; their local adaptations
have the explicitly narrowed scope above. Original LGPL/GPL notices remain on
retained units; the retired CramFS module no longer needs a package notice entry.

Repeatable baseline verification and proposed-revision review use the existing
tool, for example:

```sh
python3 scripts/upstream_baseline.py verify --upstream /path/to/7zip-reference
python3 scripts/upstream_baseline.py review --upstream /path/to/7zip-reference \
  --new-revision <candidate-commit> --path src/CPP/7zip/Bundles/Format7zF/Arc.mak \
  --output build/review/retired-formats/upstream-review.md
```

Use the existing Linux `reference-verify`/`reference-review` commands and each
component manifest for its original units. Do not promote these baselines merely
because local compatibility was narrowed.

## Behavior evidence and validation

The ZIP compatibility fixtures for Shrink/Implode/LZFSE produced `A` with the
pre-removal 26.05 runtime; PPMd bytes came from its official ZIP encoder for
30,000 `A` bytes. The CramFS fixture also extracted correctly before removal.
These fixtures check decoded bytes through static ZIP extraction and the
independent sidecar, with Unicode/spaces in entry/archive names and unchanged
input bytes. A mismatched entry CRC must fail verification. Negative writer
rows cover explicit numeric/name requests without output.
Existing AES/Zstd and other retained format tests protect shared consumers.

The classic HFS and generated High Sierra fixtures now verify rejection.
Existing SquashFS v3 LE/BE fixtures verify rejection; public-handler tests
cover removed version/magic requests followed by a valid v4 opening. The
saved ZIP PPMd UI profile regression checks usable fallback and Cancel without
modifying saved settings. Retired positive matrices are removed; retained
format tests keep error, cancellation, independent/retained streams and bounds
coverage.

The HFS backend and filesystem-consent matrices reuse one immutable deployment
catalog through independent backend instances. Each row still opens, lists and
extracts its own source and owns its operation state. Repeated deployment
discovery belongs to codec-discovery coverage; these format matrices discover
the deployment once without increasing their suite timeouts.

Baseline verification passes at the pinned 7-Zip revision (412 exact units,
271 adapted units and 15 local units); original baseline comparison tooling
is unchanged.
The affected libraries, desktop application and operation host build on macOS.
Focused macOS runs pass the removal tests, retained ZIP writer/AES/Zstd tests,
HFS+/SquashFS/ISO controls and compression-profile regressions. Reconfiguration
with a deliberately restored old project CramFS module also passes the runtime
rejection check, confirming incremental-build cleanup.

Before the ZIP scope revision, Linux and Windows runs passed the removal
checks and retained format controls. Two unrelated failures were reproduced
with unchanged main
checkout binaries on macOS: `compression_encryption::methodsAndShowPassword`
restores ZipCrypto instead of AES-256, and
`iso_stream::backendExtraction(names-joliet-surrogates)` cannot open an output
filename. The encryption failure also occurs on both guests; the ISO filename
failure occurs on Linux, while that suite passes on Windows. These failures
remain outside this change. Full/slow suites were not requested or run.

The final macOS HFS/backend and filesystem-consent run passes all four selected
suites. Linux and Windows pass `hfs_stream` and the focused English/Russian
`filesystem_hfs_read_ui`; the two broader `filesystem_read_ui` suites on both
guests still hit their existing 60-second limits. Those broad runs are incomplete
validation, not successful acceptance.

After the ZIP scope revision, macOS passes all five focused suites:
`legacy_zip_methods`, restored `zip_shrink`, `native_zip_writer`, `zip_zstd`
and `compression_profile_selection`. Coverage includes correct legacy payloads
through the static handler and shipped sidecar, CRC failure, Shrink dictionary
boundaries/malformed streams/cancellation, retained AES/Zstd and saved-profile
fallback. Static and sidecar PPMd creation fail without archive output.
The restored upstream units and method-ID alias are platform-independent;
Linux/Windows were not rerun for this revision, and their earlier evidence
above must not be read as acceptance of the restored ZIP paths.
