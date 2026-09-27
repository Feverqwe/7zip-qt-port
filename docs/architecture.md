# Architecture

## Repository boundary

`sevenzip-desktop` owns the graphical application, platform integration, archive backend adapter, tests, and build configuration. `../7zip-upstream` is the unmodified official source checkout for reference and later integration work. The application must not depend on a fixed absolute path to that checkout.

## Components

```text
Qt Widgets (window, file list, dialogs)
          |
ArchiveBackend interface (operations, progress, errors)
          |
SevenZipProcessBackend (official 7zz executable)
          |
7-Zip original compression and format code
```

`src/archivebackend.h` defines archive entries and asynchronous operations. `src/archivebackend.cpp` owns command arguments, `7zz l -slt` parsing, progress, cancellation and errors. `src/mainwindow.h` owns dialogs, table presentation and the explicit no archive / loading / viewing / operating / error states. All command availability is updated from these states and the table selection. A direct 7-Zip API adapter can replace this backend later.

## Functional target

The Windows 7-Zip File Manager (`7zFM`) is the UI and interaction reference. New Qt Widgets features should follow its window structure, commands, dialogs, and workflow; necessary platform deviations must be documented. The MVP implements listing, add and extract. Test, copy/move, delete, properties, passwords and system integration remain disabled.

## Data and safety

Archive paths and filenames are runtime input. `QProcess` receives structured arguments, not a shell string. Before extraction the backend rejects absolute paths, `..` segments and link entries reported by `7zz`. Extraction only starts after a successful listing of the same archive. Passwords are not accepted or persisted in this MVP.
