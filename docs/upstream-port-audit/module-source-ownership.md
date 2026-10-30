# Module ownership and repository-owned dependency inputs

## Source scope — 2026-10-29

Sevenzip Desktop is an independent application based on 7-Zip. Project modules
own adapted implementations and integration; unchanged dependencies retain their
internal source layout under `third_party/`. The required directory organization
is described in [architecture](../architecture.md#module-directories-and-source-ownership).

Original repository: <https://github.com/ip7z/7zip.git>.
Full revision: `b88c6bc09339de32f7d18bbab900686b244f42ba`.
The complete original `Asm/`, `C/`, `CPP/`, `DOC/`, README and attributes inventory
is retained without byte changes inside `third_party/7zip/source.tar.xz`. Its
container hash and each complete member's SHA256 are pinned in the baseline.
Members retain original Git blob bytes rather than `git archive` export bytes:
the dependency's attributes request CRLF for old Visual Studio project files,
but their committed blobs use LF. This presentation conversion is not an import
adaptation and must not change the original baseline hashes.
Licenses remain readable outside the archive. Existing adapted
implementations are retained; the import does not overwrite project changes.
The original dependency README is an archive member; current project provenance
belongs here and in the baseline rather than in a replacement dependency README.

The existing [baseline](../upstream-baseline.json) now records explicit original
paths for borrowed source owners and SHA256 hashes for complete original units.
Its original-inventory records also identify the local bytes needed to prepare
native engine, console and SFX source trees without an external Git checkout.
Original copies corresponding to adapted units are immutable dependency inputs,
not additional project implementations selected by application targets. Native
runtime preparation still applies the recorded engine adaptations; full GPL
readers stay outside that core overlay. The shared external format modules remain
the authoritative readers used by both the application and console.

No new Windows UI or other original-only feature is exposed by retaining its
dependency source. Existing Qt/platform adapters, external reader ownership,
codec selection and omitted source scope remain in force. External libraries
required by adapted units keep their existing separate owners and baselines.
All original per-file notices and `DOC/License.txt` are retained, including LGPL,
public-domain/BSD exceptions and the unRAR restriction; no source is relicensed.

## Project modules and other dependencies

Adapted sources, Qt integration, C++ dependency facades and their module
interfaces now live with their functional owners. Tests follow the same
component boundaries. The window-operation suite retains its executable,
registration and test slots, with definitions divided by the exercised workflow.
Component CMake files retain their original directory scope and registration
order; the root file composes them rather than defining every component.
The source transfer changes paths and include spellings without changing
product algorithms or operation control flow.

Unchanged Brotli, libsmb2, Linux, LZ4, lzlib, XZ, zlib and zstd inputs are also
stored in local archives. `dependency_archives` in the existing baseline records
the containers, complete-member hashes and any project overlays. Their existing
reference manifests retain original repositories, revisions and inventory
hashes; `owners` locates each imported unit independently of project layout.
The archived layout preserves each dependency's include/build contracts.
The source helper verifies container/member bytes and refuses linked or unsafe
outputs, updates only changed source bytes and retains incremental build outputs.

Original release Help HTML/CSS is archived separately with its existing installer
and CHM provenance. The two already adapted pages stay in the Help module and
are selected as asset overlays in the generated view. Qt resource addresses and
relative links retain their previous spelling. No original release page is
rewritten while packaging the dependency.

The modified libsmb2 units have moved into the SMB dependency module. Generated
forwarding C/header units connect those live sources to the dependency's build
rules, preserving C compilation and compiler dependency tracking. No modified
libsmb2 source is presented as an unchanged archived dependency.
The adapted NTLM unit names its dependency's MD5 header explicitly to avoid
the case-insensitive collision with 7-Zip's `Md5.h`; protocol declarations
retain their required position before the dependency's API/internal headers.
The C++ facade uses a distinct `facade.h` name: MSVC's ancestor-directory search
for quoted includes must not select it instead of libsmb2's C protocol header.

The previously vendored zstd headers contained formatting changes despite being
classified as unchanged dependencies. Before archiving, those headers were
compared with Git blobs at `f8745da6ff1ad1e7bab384bd1f9d742439278e99` from the
recorded official repository. Canonical formatting removed all differences
except spacing inside one macro comment. The archive restores original bytes;
original hashes, revision, symbols and compression behavior are unchanged.

## Verification and update review

Use a read-only checkout containing the recorded revision:

```sh
python3 scripts/upstream_baseline.py inventory
python3 scripts/upstream_baseline.py verify --upstream <reference-checkout>
python3 scripts/upstream_baseline.py review --upstream <reference-checkout> \
  --new-revision <full-commit> --output <review-report>
```

The [update procedure](../upstream-update.md) governs dependency/omission review
and retention of local changes. Original hashes describe original units, not
the current adaptation, so relocating or refactoring a project owner must not
change those hashes. Original-reference comparisons may explicitly use an
external snapshot; normal runtime/SFX preparation uses repository-owned inputs.

Other dependency views are expanded by `scripts/dependency-sources.py` during
CMake configuration. Reference verification/review reads archived owners through
the same helper, so update review does not require restoring source files in the
checkout. Archived inputs are never fetched during an ordinary build.

## Validation boundary

Directory migration and local dependency packaging are implemented. Application
builds and the focused checks below passed on Linux, macOS and Windows.
The imported original inventory and the working native deployment are separate
claims; runtime validation is not established merely by matching source hashes.

Token comparison of 1,783 relocated C/C++ source units, accounting for include
spelling, comments and formatting, found no product-body changes. Two test
fixture loaders now receive their fixture directories explicitly instead of
deriving them from relocated generator paths. All window operation test-slot
bodies were compared separately after extraction and retained.
This is source-transfer evidence rather than runtime acceptance.

Read-only verification against the recorded Git revision passed for original
7-Zip units, exact dependencies and adapted/local owners. The zstd reference
verification also passed against its pinned official commit. Linux passed all
baseline-tool tests, including archive corruption/path refusal, live C/header
rebuilds, offline originals and Git-blob versus export-attribute handling.

Linux built the relocated native engine, console, SFX, application and required
format modules. After dependency packaging, focused checks passed for archive
operations/protocol, UI state/operations, SMB directory service, Brotli, LZ4,
Lzip, zlib, checksums, asymmetric signatures, runtime cache and runner transfer.
The subsequent Linux general gate passed formatting and built all targets. Its
non-slow runtime run was interrupted after finding relocated fixture-loader
failures; it is not a passing full gate. After those fixes, focused checks passed
for Ext and SquashFS streams, Ext fixtures, Help, QCOW and the baseline tooling.
Three other suites exceeded their normal timeouts on disk: UDF streams, nested
archive chains and UI copy routing. At timeout their workers were waiting for
filesystem journal commits. The same binaries and suites passed with their
normal timeouts using private temporary directories on the existing `/dev/shm`
filesystem. No timeout or product code was changed for that diagnostic.
Linux also passed seven focused consumers with Clang 19 named dependency modules:
XFS, EROFS, zlib, Brotli, LZ4, Lzip and the SMB directory service.
macOS built the application, native engine, console and SFX and passed 19 focused
suites covering the same archive, UI-state/operation, dependency and tooling
boundaries. Its SMB consumer passed again after the explicit-header adjustment.
Windows built the application, native engine, console, both SFX variants and the
selected dependency consumers. Seventeen focused archive, UI-state/operation,
Help, crypto, dependency and runner suites passed. Two tooling suites exposed
fixture portability issues: deleting read-only Git objects and native separator
spelling in a synthetic manifest. Their fixes passed Linux and macOS, then both
tooling suites passed on Windows. The original fixture checkout is now made
unavailable by renaming it; temporary-directory cleanup handles its Git metadata.
No slow suites or complete release/package acceptance are claimed by these
focused checks.

## Independent original-source verification — 2026-10-30

Before integration, source packages were downloaded again from the recorded
upstream repositories and the official lzlib/7-Zip releases. All 1,669 archived
original members and 26 additional visible-file/extracted-notice comparisons
matched byte for byte. The complete 7-Zip inventory also matched a fresh Git
fetch at the pinned commit; 42 legacy `.dsp`/`.dsw` members in GitHub's source
tarball differ only by export-time CRLF conversion. The archive retains their
original Git blob bytes, as described above.

The full Linux tarball returned HTTP 429 directly and through the supplied
proxy; kernel.org's full snapshot did not complete. Every selected original
Linux unit was instead downloaded from its pinned commit through raw-file and
contents endpoints. Linux notices were checked against those originals, and
the GRUB license matched the original GNU GRUB 2.12 package. Help HTML/CSS and
its visible license matched a fresh official 26.05 installer/CHM extraction.

Provenance JSON, the local attribution summary and assembled/extracted notice
files are explicitly project metadata, not complete upstream source files.
Three such Linux notices were removed from the source archive; their visible
copies and installation paths remain unchanged. The Linux archive now contains
only complete original upstream files. No borrowed implementation or license
text was changed by this verification.

After that separation, baseline, runtime-cache and package-tooling checks passed
on Linux, macOS and Windows. A staged Git checkout with `core.autocrlf=true`
preserved the exact bytes of all 44 dependency and adapted Help files; Git
attributes now preserve those bytes independently of the checkout platform.
