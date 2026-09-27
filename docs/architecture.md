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

The backend separates `7zz` output into complete lines across process reads, emits progress and current-file detail, and forwards each line to the UI. The window keeps the complete log of the current or last operation, including the final result. Its nonmodal progress dialog leaves the main window and log available while commands remain disabled by the window state. The Windows `7zFM` progress dialog shows the operation and current filename and has a messages view; the Qt menu command exposes the same information without changing the file-list layout.

## Functional target

The Windows 7-Zip File Manager (`7zFM`) is the UI and interaction reference. New Qt Widgets features should follow its window structure, commands, dialogs, and workflow; necessary platform deviations must be documented. The MVP implements listing, add and extract. Test, copy/move, delete, properties, passwords and system integration remain disabled.

## Data and safety

Archive paths and filenames are runtime input. `QProcess` receives structured arguments, not a shell string. Before extraction the backend checks every listed entry and rejects absolute or parent paths, links reported by `7zz`, nonportable names, case or Unicode equivalent duplicates, and file/directory conflicts. It also requires an existing destination directory, rejects a final symlink, and rejects existing target paths and symlink ancestors inside the destination. Extraction only starts after a successful listing of the same archive. This is a preflight check; concurrent external changes to the archive or destination can invalidate it before `7zz` writes. Passwords are not accepted or persisted in this MVP.
