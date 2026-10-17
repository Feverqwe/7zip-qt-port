# P3-05.11 — Current local-UNC fallback acceptance (2026-10-16)

> Архив завершённого Qt-портирования, 2026-10-17. Статусы и проверки
> относятся к указанным ревизиям. [Рабочая документация](../../../README.md).

The bounded source/consumer component introduced in
`c8176f75f5a2cf5e654bad736db1f6e4444fb9ca` is accepted for current product
`a2254e5b139632cb2427d57f085510d9fa1c28ea`. Original source provenance,
RED E_NOTIMPL/crash, first-integration Copy refusal correction and historical
full-gate failures remain in the
[original receipt](p3-05-2026-10-06.md#p3-0511--source-unc-root-binding-and-qt-consumer-validation-active-2026-10-06).

Current `CRootFolder::BindToFolder(const wchar_t *, IFolderFolder **)` is
recompared with the pinned read-only sibling checkout at
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`,
[ip7z/7zip](https://github.com/ip7z/7zip). The normalize→CFSFolder Init→
separator-led CNetFolder fallback, original null-result/HRESULT and Detach
ordering remain. Subsequent address-combo, ADS error and namespace-icon changes
have their own source receipts. NetFolder/NWindows::NNet and source panel binding
remain authoritative; no new implementation, dependencies or formatting changes
are made for this acceptance. The Windows branch is conditional; Unix retains
its separate mounted-network policy and current root/navigation consumers.

Windows snapshot `20261016T192542Z-6e94ed8d` runs only network_folder and
native_network_navigation with the scoped administrator token needed by the
existing SDK fixture. It passes2/2 default in24.44s and2/2 native in2.72s,
10Qt/0fail/0skip each. All current code inputs match; private helpers/session
records and subsequent documentation are the only snapshot differences.
Source guard and byte-for-byte generated CTest restoration pass.

The fixture publishes a UUID-named share to its disposable directory, with
access only for the current runner SID. Direct provider/root fallback open the
owned share, check exact Unicode/space binary bytes, remove its publication and
refuse/reload the stale entry. EN/RU normal/flat actual-window consumers cover
server address, disabled mutations/Copy without source operations, opening its
share, address dropdown, Parent focus, post-deletion refusal/Refresh and no
archive-engine calls. Explicit NetShareDel and RAII cleanup affect only that
publication; original control bytes remain. No existing share, external server,
credential, firewall, account or security policy is changed.

All eight current980×620/640×400 JSON reports are inspected against the
[existing contract](../../../ui-layout-contracts/shared.md): menu/toolbar/address/list/
status order is retained; the address and table are fully exposed in both
languages/modes. Compact address bounds38,91,569,22 and table4,121,632,248 fit
640×400. No PNG or Windows pixel-parity claim is made.

Current macOS default324/324 and Linux default323/323/broad XCB235/235
receipts cover portable root/navigation/direct consumers after the old pending
Linux capacity check. These results and the separately verified fourteen
privileged Windows suites do not convert the failed Limited Windows whole gate
into a pass. Cold codec timing, remaining source-owner/native workflows and
rendered final acceptance belong to P3-03/P3-05/P3-06 and remain open.

Ignored evidence is
`build/review/final-availability-8e9343b-20261014/windows-final-native-network-v47-evidence`:
actual default/native LastTest, eight JSON reports, exact source hashes, runtime
reuse and configuration-restoration receipts. This closes P3-05.11's source
fallback/local-UNC consumer scope, not the parent package or external SMB scope.
