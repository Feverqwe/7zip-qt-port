# Localization

Application messages use Qt Linguist. The English source text lives beside
its use; translations live in the `translations/` catalogs. Build with the
LinguistTools component of the same Qt SDK as the application.

Update catalogs after adding or editing translatable text:

```sh
./scripts/build.sh dev sevenzip_lupdate
```

Open the catalog in Qt Linguist, translate the new or changed messages, and
preserve placeholders, keyboard accelerators and intentional line breaks.
Use Qt parameters (`%1`, `%2`) and numerus forms (`%n` or `%Ln`) for quantities.
The English catalog contains source-language plural forms; maintain those forms
alongside new numerus messages even when English is the selected fallback.
The normal build runs `lrelease` and embeds the resulting catalogs; users do
not need Qt Linguist installed. Standard Qt widget translations from the SDK's
Qt Base catalog are merged into the application catalog. Install the matching
Qt Tools and Qt Translations SDK components when setting up a build machine.
Qt Base's English source strings are built in and need no separate English catalog.

Run the focused catalog, language-switch and diagnostic checks with:

```sh
python3 scripts/verify.py localization --platform local
```

The same profile supports configured Linux and Windows runners through
`--platform all`.

To add a language, create `translations/sevenzip_<locale>.ts` with the correct
TS language metadata, update it with the same command, and translate it in
Linguist. CMake discovers the catalog and the language chooser reads the compiled
catalogs. Regional locale names are accepted; unavailable translations fall back
to the English source. Additional compiled catalogs can be placed in a
`translations` directory beside the executable.

Use `tr()` in QObject classes or `QCoreApplication::translate()` with an
explicit context elsewhere. Deferred widget captions must retain the source
text and context so they can be translated again after a language change.
Mark those source literals with `QT_TRANSLATE_NOOP`. Translate complete
sentences and use placeholders for variable content rather than concatenating
sentence fragments. Use numerus messages for counts and `QLocale` for regional
number/date formatting. File names, archive member names and engine identifiers
are data and must retain their original values.

Upstream resource IDs remain behind an adapter to the same catalogs. The old
language-file parser is no longer a runtime text source. Preserve the resource
ID as a disambiguation comment when editing those catalog entries.
The Windows shell extension reads the same catalogs in its native host without
creating a Qt application; additional catalogs belong beside the extension in
its `translations` directory.

Upstream narrow diagnostic fields and exceptions retain their source types.
Mark their English keys with `QT_TRANSLATE_NOOP` in the `SevenZipDiagnostics`
context and translate at the Unicode presentation boundary. Translate diagnostic
prefixes before appending paths or user arguments; never use a composed message
containing user data as a translation key.

Selection counts use Qt numerus and regional number formatting. Engine protocol
keys and transport date syntax retain their required format.

The migration covers application captions, menus, dialogs, progress and diagnostics,
including standard Qt widgets and the native Windows shell host. Technical format
identifiers, paths and arbitrary external-tool/plugin output remain data. Native
system error messages retain the operating system's language. Source and runtime
evidence, platform coverage and exclusions are recorded in the
[localization audit](upstream-port-audit/qt-linguist.md).
