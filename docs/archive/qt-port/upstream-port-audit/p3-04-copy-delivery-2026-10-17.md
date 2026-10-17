# P3-04 — current source/Copy delivery, 2026-10-17

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

Product `09a6f6db021e349abb554e1bb9e0b339cd9d0a96` is packaged Release with
`BUILD_TESTING=OFF` on macOS arm64, Linux x64 and Windows x64. It includes the
accepted Options keyboard, namespace shell icons/Root history and narrow source
Copy Browse corrections. No source or dependency is changed by this delivery.
Official engine/SFX provenance remains https://github.com/ip7z/7zip.git at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`; Qt is6.12.0.

All1056 committed product/build inputs are verified by Git-blob hash before and
after packaging. This expands the earlier1047-input inventory with nine existing
tracked build/inspection tools, not nine new product features. Guest run
`20261017T111354Z-e1ffabcd` uses the persistent `current/build/release` trees;
Windows is Limited interactive, Linux uses Xvfb for the GUI smoke. The existing
private macOS Release tree is reused. Its1055 previous inputs match committed
`b4d7c3cc2410bafc84ac7c24aa64c266f2ab73da` before copying current inputs; the
older e3b3c65 receipt was correctly rejected as its baseline. No foreign changes
are overwritten. The exact full baseline is retained in `macos-prior-tree-proof.json`.

| Artifact | Bytes | SHA256 | Relocated scenarios |
| --- | ---: | --- | ---: |
| macOS ZIP | 12,532,844 | `55d9766d553fb83d21766b0398a8969ace68dbbc41df8972e5e54598f71b7beb` | 15 PASS |
| Linux tar.gz | 34,216,725 | `9010a0ee2e26b6e457d8d21a40ac5eb753691dde26ceb751dba6abc4abdd04cf` | 14 PASS |
| Windows ZIP | 28,562,721 | `c7a9a202d2621fd47fb2a904c6c46ef096bbb0f62d20b0e0ca5983af733f3876` | 14 PASS |

The relocated GUI creates, lists, tests and extracts ZIP/7z with Unicode/spaces,
binary and empty payloads, preserves source bytes and reports broken archives
and a missing engine module. macOS additionally passes strict deep ad-hoc
signature verification. Fresh archive extraction verifies payload constraints,
all38 Qt license files byte-for-byte against the pinned Qt source, product/
upstream notices, SFX provenance/hash and applicable ICU/CRT notices. Windows
includes its five CRT DLLs and redistribution notice. These are local private
artifacts; neither public signing nor publication is performed.

Fresh dependency checks inspect all11 Mach-O objects/127loads and resolve
non-system loads inside the bundle, with no development rpaths. The application's
private QKeyMapper import resolves in its bundled QtGui. Linux ldd/readelf finds
no missing library, resolves all four Qt dependencies from the extracted package
and verifies its QtGui private-symbol binding and origin-relative runpath.
Windows inspects26PE objects/345normal imports/zero delay imports; all55 external
basenames freshly map to trusted System32/WinSxS with
`LOAD_LIBRARY_SEARCH_SYSTEM32|DONT_RESOLVE_DLL_REFERENCES`. This inspection runs
no DllMain. Eight official runtime/SFX hashes remain identical after validation
on each platform; guest inputs also match their protected producer.

Evidence root: `build/review/goal-delivery-09a6f6d/`, with per-platform archives,
source/runtime manifests, smoke/payload/dependency proofs, drivers and storage
receipts. Duplicate transfer tars are removed after archive hash/size verification.
A Windows path-normalization error in the collector's final duplicate-cleanup
check is corrected locally; the already-collected artifact is independently
rehash-verified, without relabeling or repeating the successful guest checks.
Disposable validation extractions are removed after proofs are retained;
persistent build/compiler caches and protected runtime/reference producers remain.
Host D: storage checks before/after packaging are OK; no disk or VM configuration
is changed.

This refreshes the bounded delivery scope. Clean-OS checks remain excluded by
the user's decision, not passed. Earlier manual/native OS-input receipts retain
their original revisions. P3-04 still depends on the open P3-03/P3-06 cold/native/
full-application acceptance; the parent is not marked done and the count remains
44/47. No push, publication or upstream modification occurs.
