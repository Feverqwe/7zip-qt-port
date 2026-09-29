# Upstream UI reference and Qt comparison

To collect rendered Windows sizes, control IDs, fonts and optional PNGs from
official 7zFM, use the [Windows capture guide](windows-ui-reference.md) and
`scripts/capture-7zfm-reference.ps1`. Its runtime JSON is a separate schema,
not yet an input to the RC/Qt comparator below.

`scripts/check_ui_parity.py` extracts a desktop 7zFM resource reference and
compares it with a reviewed contract and the existing Qt widget JSON reports.
It runs without Windows. Python 3.9+ and the standard library are sufficient
for comparison; extraction additionally needs clang or GCC on PATH (or
`--compiler /path/to/compiler`). MSVC's preprocessor is not supported yet.

Maintained contracts cover Copy/Move (six upstream controls), the archive
password prompt (all five controls) and the progress presentation subset (all
26 controls), normal/compact reports and selected
spatial/state properties. Other dialogs
can be extracted, but require their own reviewed mappings and state coverage.
Main-window menus/toolbars built dynamically in C++ need separate contracts;
this is a dialog-resource reader, not a universal Win32 UI converter.

## Run the maintained comparison

```sh
./scripts/build.sh dev copy_dialog_tests
ctest --preset dev -R '^ui_parity_(tool|copy)$' --output-on-failure

./scripts/build.sh dev password_dialog_tests
ctest --preset dev -R '^ui_parity_password$' --output-on-failure

./scripts/build.sh dev progress_dialog_tests
ctest --preset dev -R '^ui_parity_progress$' --output-on-failure
```

CTest's `ui_reference_copy` fixture runs `copy_dialog` first, even with this
filter, so reports come from the current build. `ui_reference_progress` similarly
runs `progress_dialog` and writes `progress-parity.json` for its 16 reports.
Copy output is
`build/dev/ui-reports/copy-parity.json`: checked states, errors, recorded
adaptations and limitations. Exit codes: 0 passed, 1 comparison differences,
2 invalid inputs/extraction error. Tests use local disposable fixtures, no
user archive data or live services. `ui_parity_tool` exercises synthetic
resources and deliberately broken widget reports independently of upstream.

For direct comparison (generate fresh reports first):

```sh
python3 scripts/check_ui_parity.py compare \
  --reference docs/ui-reference/copy-dialog.reference.json \
  --contract docs/ui-reference/copy-dialog.contract.json \
  --reports build/dev/ui-reports \
  --output build/dev/ui-reports/copy-parity.json
```

## Extract and review an upstream baseline

```sh
python3 scripts/check_ui_parity.py extract \
  --upstream ../7zip-upstream \
  --resource CPP/7zip/UI/FileManager/CopyDialog.rc \
  --source CPP/7zip/UI/FileManager/CopyDialog.cpp \
  --output /tmp/copy-dialog.reference.json
```

Repeat `--source` for relevant handler/caller files. Resources and dependencies
are read-only. The tool stages temporary copies for the preprocessor; resource
IDs remain symbolic. Extracted input hashes must match the exact Git HEAD
files, otherwise extraction fails. Commit and dependency hash equality with the
reviewed contract is checked during comparison. Compare a new extraction with
the stored reference before updating either baseline or contract.

`progress-dialog.reference.json` records the exact desktop IDD_PROGRESS resource,
its shared inputs and reviewed handler navigation index. It is a source reference
prerequisite; Qt mapping/state contracts and current-build reports follow the full
progress view port. Its presence alone proves no rendering or workflow parity.

The maintained baseline is upstream
[`ip7z/7zip` at `0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`](https://github.com/ip7z/7zip/tree/0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17).
`copy-dialog.reference.json` is generated resource data and short source
navigation excerpts from Igor Pavlov's 7-Zip. Applicable notices/license:
`third_party/7zip/License.txt`. Python tooling is local inspection code; no
upstream UI functional algorithm is replaced or ported by this tool.

### Extraction scope

`overwrite-dialog` and `mem-dialog` references/contracts cover the extraction
prompt resources. Their current-build producer is `extract_prompt`; comparisons
are `ui_parity_overwrite` and `ui_parity_mem`. Both use normal/compact reports,
with extra overwrite actions and memory editing state varied. These dependencies
are included in the integrated P0-04.1 prerequisite and its full gate.

The real C preprocessor expands local includes, object/function macros and
conditional branches with `RC_INVOKED` defined and `UNDER_CE` undefined.
`windows.h`, `CommCtrl.h`, `winnt.h` and `WinUser.h` are deliberately opaque;
Windows style constants remain symbolic. Other SDK includes fail. Quoted
includes must stay inside the upstream root. This profile cannot evaluate
branches requiring SDK-defined values and is not a substitute for rc.exe.

The parser supports DIALOG/DIALOGEX, BEGIN/END or braces, labels, buttons,
groups, edits, combos, listboxes and generic CONTROL declarations. Unknown
statements and unresolved coordinate expressions fail rather than disappearing.
Coordinates support integers, parentheses, unary signs and +, -, * arithmetic.
ICON maps to a QLabel presentation; icon provider/rendering adaptations require
review, and no resource-to-pixel claim follows from that mapping.
CONTROL checkbox/radio styles map to Qt classes; other generic controls require
an explicit reviewed class adaptation. More syntax should be added with tests
when a real upstream resource needs it. LISTBOX's default mapping is QListWidget;
other Qt implementations require a class adaptation.

DLU bounds retain the resource's original units. Combo resource heights include
the dropdown extent. No DLU-to-pixel equality is asserted. `above`/`left_of`
validate upstream anchor order, then require separated Qt rectangles; `inside`
validates containment. `separate` is a Qt usability assertion and requires an
adaptation reason. Only explicitly listed relations are checked. Literal RC
captions may be changed by translation/handlers; the contract must account for
that rather than treating English literals as final displayed text.

## Contract schema (version 1)

Use the maintained Copy contract as the executable example:

- `upstreamCommit`, `dependencies`, `dialog`: exact extraction baseline and ID.
- `controls`: each RC ID maps to `selector`, or `omit` with a nonempty reason.
  Selectors match `name`, `class`, `text` (string or alternatives), and optionally
  a named ancestor `within`. Missing/ambiguous matches fail.
- `qtClass` overrides require `classAdaptation`; literal text differences require
  `textAdaptation`. Combo editability is checked automatically. Unmapped or
  unknown RC IDs fail. Internal Qt children/additional Qt controls are not
  automatically treated as errors; review them in the widget report.
- `relations`: `a`, `b`, `kind` (`above`, `left_of`, `inside`, `separate`). Without
  an `adaptation`, the relation must be supported by the extracted RC geometry.
- `states`: name, explicit report filenames, `evidence` with source/symbol and
  reviewed requirement, and `assert` entries (`id`, `property`, `equals`).
  Supported properties: visible, enabled, fullyVisible, editable, labelFits,
  itemCount, text. Missing reports/properties or mismatches fail.
- `behavioralTests`: review documentation naming existing Qt tests for
  transitions/results. The comparator does not execute these tests itself;
  the CTest producer fixture runs the Copy behavioral suite.

`behaviorSymbols` indexes common `bool`/`void`/`INT_PTR` member definitions,
including helpers called by On* handlers; declarations alone are excluded.
`behaviorCandidates` lists selected state/layout calls with source lines.
These are search hints, not semantic C++ parsing: branches, cross-function
dependencies, focus, initial editable values and transition results must be
reviewed and tested. The skill `$sz-ui-parity` defines that workflow.

Copy's shared prompt/init and resize requirements cite OnInit/OnSize. Existing
`copiesFieldsAndCommitsOnlyOnOK` and
`browseNormalizesDirectoryAndCancelPreservesText` cover OK/button/Cancel and
Browse cancellation/success/normalization. JSON omits editable values and cannot
prove these transitions, read-only state or actual selection contents.

Password uses the `ui_reference_password` producer fixture and writes
`build/dev/ui-reports/password-parity.json`. Its resource extraction also includes
PasswordDialog.cpp. OnInit, ReadControls, SetTextSpec and OnOK were reviewed;
`password_dialog` covers masked/shown transitions, raw values and confirmation.
`ui_archive_states` covers backend submission, cancellation and password lifetime.

Passing means this reviewed subset matches with declared adaptations. It does
not establish pixel parity, complete behavioral parity or Windows runtime
verification. Fonts, icons, colors and native rendering still require an
actual Windows reference. Current desktop ExtractDialog.rc uses combo boxes
for path/overwrite modes; historical descriptions mentioning radios must not
override the pinned resource.
