# P3-04 — current native icon/System delivery, 2026-10-17

Product `40b3439678b301fd722cb55bccfce198c6738696` is packaged Release with
BUILD_TESTING=OFF on macOS arm64, Linux x64 and Windows x64. It includes the
accepted GIO handler images, macOS own-bundle image and original application/
window/PE/bundle icon corrections. Official engine/SFX source remains
https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`; Qt6.12.0. No source or dependency
changes are made by this refresh.

All1056 product/build inputs match committed Git blobs before and after
packaging. Guest run `20261017T123526Z-14fe21fb` reuses persistent current Release
trees. Windows is Limited interactive; Linux GUI smoke uses Xvfb. The existing
private Mac Release tree first matches all1056 previous09a6f6d inputs; only15
changed committed files are copied. No foreign change or build cache is replaced.

| Artifact | Bytes | SHA256 | Relocated scenarios |
| --- | ---: | --- | ---: |
| macOS ZIP | 12,533,909 | `51b8872cb2a147992d111b458a6921f81c957394b092ac442ceefff26f902cd1` | 15 PASS |
| Linux tar.gz | 34,213,955 | `c8fa73909c44c73e2e364dd30fcd9f4c6503ffcfb1bd0ff48e0240f8b5e5348f` | 14 PASS |
| Windows ZIP | 28,568,704 | `0364756b3b703fac29dc71d83f30d83b1e02f481653e6706d1374bd5827ec0e8` | 14 PASS |

Relocated GUI create/test/extract and engine list cover ZIP/7z, Unicode/spaced
paths, binary/empty payloads, source preservation, broken archives and missing
engine module. Fresh extraction passes payload constraints, all38 pinned Qt
licenses byte-for-byte, own/upstream/ICU/CRT notices and SFX provenance/hash.
Windows includes five CRT DLLs; macOS passes strict deep ad-hoc signature checks.
These local private artifacts are neither publicly signed nor published.

Fresh dependency inspection resolves11 Mach-O objects/127loads, the bundled
private QKeyMapper import, all four Linux Qt libraries and origin-relative ELF
runpaths without development paths or missing dependencies. Windows26PE objects
have345 normal imports/zero delay imports; all55 external basenames resolve to
trusted system locations using the established no-DllMain inspection. Eight
official runtime/SFX input hashes remain unchanged per platform; guest inputs
also match their protected producers.

## Native icon boundary

Windows Limited run `20261017T123742Z-069cf369` reads the three actual packaged
EXEs, not dev binaries. Their native16px color pixels and transparency masks
match the original ICO. Its disposable extraction is removed after observation.

The Mac package's ICNS is byte-identical to the source-derived build asset,
SHA256 `e16ae4d76296a98a138db5a8e87b870f9dd74c39aaef22a45e6e1a6401285e95`.
An additional strict48px NSWorkspace-versus-raw-ICO pixel probe fails: the native
packaged icon contains the source7z graphic inside a rounded tile with shadow,
while the dev bundle's native icon still compares equal to the raw ICO. The
decoded asset rasters were inspected; they are not screenshots of a Mac view.
Apple documents system-applied rounded masking for macOS app icons in its
[app-icon guidance](https://developer.apple.com/design/human-interface-guidelines/app-icons).
The observed framing is consistent with native icon treatment; the exact reason
for the difference between these paths is not established. No system cache is
flushed and no image/product code is changed to force equality. The failed
strict probe remains failed; this delivery accepts retained source asset/native
logo identity, not Windows/Mac screenshot or raw-raster pixel parity.

Evidence: `build/review/goal-delivery-40b3439/`, with private archives, manifests,
source/runtime proofs, relocation/payload/dependency/icon reports and one-shot
drivers. Duplicate transport tars and three disposable validation extractions
are removed after hash/proof retention. Persistent builds/compiler caches,
protected reference/runtime producers and current failure evidence remain.
Host D: storage checks before/after are OK; no VM/disk configuration changes.

This refreshes bounded delivery. Clean-OS remains excluded by the user's prior
decision, not passed. Full native workflow, Windows cold stability and remaining
P3-03/P3-06 acceptance stay open; P3-04's dependency is not waived. The count
remains44/47. No push, publication or upstream modification occurs.
