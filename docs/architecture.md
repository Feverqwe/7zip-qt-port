# Architecture

## Repository boundary

`sevenzip-desktop` owns the graphical application, platform integration, archive backend adapter, tests, and build configuration. `../7zip-upstream` is the unmodified official source checkout for reference and later integration work. The application must not depend on a fixed absolute path to that checkout.

## Planned components

```text
Qt Widgets (window, file list, dialogs)
          |
ArchiveBackend interface (operations, progress, errors)
          |
SevenZipProcessBackend (official 7zz executable)
          |
7-Zip original compression and format code
```

The process backend is a first integration step, not a permanent restriction. Keep command construction, output parsing, cancellation, and error mapping in the backend. Keep widgets unaware of command-line syntax. A direct 7-Zip API adapter can replace it if needed after the initial workflows are working.

## Functional target

The Windows 7-Zip File Manager is the interaction reference: file navigation, archive listing, add, extract, test, copy/move, delete, properties, progress, and keyboard shortcuts. Implement vertical slices. The current window is only a visual scaffold; every toolbar action is disabled until wired to a real operation.

## Data and safety

Archive paths, passwords, and filenames are runtime input. Pass process arguments as structured arguments, not through a shell string. Never persist passwords in logs or project files. For extraction, account for paths outside the chosen destination and symlinks when designing validation and tests.
