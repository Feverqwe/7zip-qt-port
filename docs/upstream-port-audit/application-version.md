# Application and upstream version identity

Date: 2026-10-18.

Source repository: <https://github.com/ip7z/7zip.git>, commit
`9128b80e3a471108678e2b3ec3c985861c8d7b0f`.
Reviewed complete `CPP/7zip/UI/FileManager/AboutDialog.cpp/.h/.rc`,
`AboutDialogRes.h`, `MyLoadMenu.cpp::ExecuteCommand`, `CPP/7zip/MyVersion.h`
and `C/7zVersion.h`, together with the current Qt command and dialog consumers.
The About files are unchanged from the port's recorded source revision
`0766b733fe3e06dd2a7f9a3cfbf2108ac73abd17`.

The original dialog shows the 7-Zip version/CPU, release date and copyright,
reports codec errors, opens the license/help and links to the upstream homepage.
Those branches, dependencies and command connections are retained. No additional
upstream code was copied or omitted. The existing notices and baseline remain
authoritative.

Upstream has no separate Qt-port release identity. The adaptation extends the
existing application identity row with the Sevenzip release and the upstream
source version. CMake reads the latter from the retained upstream version header;
the same display version is supplied to Qt application metadata, macOS bundle
information and package metadata. Numeric release identifiers remain unchanged.
This identifies the build's source baseline, rather than probing a user-selected
external engine at runtime. Translation changes explanatory text, not either
version number. Existing height-for-width sizing protects the longer identity.

Validation on macOS arm64 with Qt 6.12: incremental builds of the application,
`help_source_tests`, `language_standard_buttons_tests` and `help_ui_tests` passed.
All three focused suites passed, including opening About through the application
command. Current About widget reports show both versions and no clipping
in EN/RU at requested normal/compact sizes and with 8/12-point fonts. Generated
macOS bundle and package metadata contain the combined display version, while
the numeric release version is preserved. Windows/Linux and rendered Windows
pixel parity were not checked for this change.
