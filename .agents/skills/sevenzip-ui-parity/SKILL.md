---
name: sevenzip-ui-parity
description: Compare Sevenzip Desktop Qt dialogs with upstream 7zFM resources and reviewed C++ state behavior. Use for UI parity audits, layout ports and extending resource-to-widget contracts; excludes pixel parity without a Windows runtime reference.
---

# Compare the 7zFM UI

Use [docs/ui-reference.md](../../../docs/ui-reference.md) for commands, supported
resource syntax and the contract schema. Run scripts from the repository root.
The working example is `docs/ui-reference/copy-dialog.contract.json`.

Read the exact upstream `.rc`, its shared includes, associated `.cpp` handlers
and relevant callers before writing a contract. Extract a reference with the
script; do not hand-author or silently refresh generated resource data. Keep the
full commit, input hashes and official repository baseline. A changed baseline
requires review of changed resources, handlers and dependencies before updating
the contract. Never modify the sibling upstream checkout.

Map every upstream control to a unique Qt selector, or record a specific omission
reason. Prefer objectName; disambiguate unnamed standard buttons by named parent
and explicit class/text. Record class/text deviations rather than suppressing
comparison generally. Derive spatial relations from the resource; dropdown
heights are popup extents, not closed-combo heights. Use narrow documented
adaptations for Qt layouts, native button ordering and platform services.

For behavior, `behaviorCandidates` is only a navigation index. Read whole
functions and their dependencies/callers. Describe each relevant initial,
conditional, error, disabled and compact state with source + symbol + observable
requirement. Link reports actually produced by focused Qt tests. Keep transitions,
confirmation/cancellation and values in public behavioral tests: static widget
JSON cannot prove them. Never invent visibility rules from control ID names.

Build the report producer and run `ui_parity_*` checks for current-build reports.
Inspect the resulting errors AND recorded adaptations. Do not modify contracts
merely to turn an unexpected difference green; decide whether to port the UI or
document an authorized necessary deviation. Add a failing regression for new
checker behavior and geometry/state coverage for newly ported UI behavior.

State precisely which dialogs, states and geometric relations were checked.
Passing a reviewed subset does not prove complete functional equivalence,
pixel parity, keyboard/tab order or Windows/Linux verification. Source porting
still requires preserving upstream functional bodies and the port audit.
