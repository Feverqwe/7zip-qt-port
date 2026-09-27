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

`src/archivebackend.h` defines archive entries, reported archive information and asynchronous operations. `src/archivebackend.cpp` owns command arguments, `7zz l -slt` parsing, progress, cancellation and errors. `src/mainwindow.h` owns dialogs, table presentation and the explicit filesystem / loading / viewing / operating states. The address bar and table show the current directory until an archive is opened. The window retains the complete archive listing and derives direct children of the current archive folder, including implicit intermediate folders and explicit empty folders. Table items retain full entry paths so sorting cannot change extraction targets. Selecting a folder expands to its listed descendants before calling the backend. Directory history and the source directory for an opened archive are kept in the window; a failed or cancelled listing restores the directory. All command availability is updated from these states and the table selection. A direct 7-Zip API adapter can replace this backend later.

The backend separates `7zz` output into complete lines across process reads, emits progress and current-file detail, and forwards each line to the UI. The window keeps the complete log of the current or last operation, including the final result. Its nonmodal progress dialog leaves the main window and log available while commands remain disabled by the window state. The Windows `7zFM` progress dialog shows the operation and current filename and has a messages view; the Qt menu command exposes the same information without changing the file-list layout.

Creation starts from the filesystem state. A single Qt dialog starts with the filesystem selection and accumulates files and folders from directory listings and shows the source set, archive name, format and destination before submission. The backend writes a uniquely named temporary archive in the destination directory, then renames it to the requested path only after `7zz` exits successfully. Failure, launch error and cancellation remove that temporary file. Existing targets are rejected, and a target inside a selected source directory is rejected. The window refreshes its filesystem listing after creation ends. Windows `7zFM` starts Add to archive from a filesystem selection and presents an options dialog; the Qt dialog keeps that review step while allowing a mixed source set from multiple directories.

Adding is a separate command available while viewing an open archive, including an empty one. The same source picker shows a fixed target, format and destination. The backend requires a successful listing of that archive and rejects matching paths using case folding and Unicode normalization, as well as links and unsafe source names. It copies the archive to a temporary peer, runs `7zz a` on the copy, then moves the original to a backup and the updated copy into place. Failure or cancellation removes the copy and leaves the original; the backup is kept if rollback cannot restore the target. The UI disables commands through the update and follow-up listing, then displays the reread contents. Copying the existing archive currently runs on the GUI thread, so unusually large archives can pause the window before `7zz` starts. The original Windows Add command begins from filesystem selection; the separate open-archive command is a Qt workflow adaptation requested for stage 4.2.

## Functional target

The Windows 7-Zip File Manager (`7zFM`) is the UI and interaction reference. New Qt Widgets features should follow its window structure, commands, dialogs, and workflow; necessary platform deviations must be documented. The MVP implements filesystem listing and folder navigation, archive folder navigation, column sorting, multiple selection, new archive creation, addition to an open archive, extraction, test and archive information. The Qt line edit replaces the Windows path combo box; inside an archive it displays the archive path and current internal folder. The toolbar has an Up action for internal folders. Double-clicking an archive file does not launch an external viewer yet. Copy/move, delete, passwords and system integration remain disabled.

## Data and safety

Archive paths and filenames are runtime input. `QProcess` receives structured arguments, not a shell string. Before full extraction the backend checks every listed entry; for selected extraction it checks only paths passed to `7zz` and rejects absolute or parent paths, links reported by `7zz`, nonportable names, case or Unicode equivalent duplicates, and file/directory conflicts. It also requires an existing destination directory, rejects a final symlink, and rejects existing target files and symlink ancestors inside the destination. Existing directories may be reused; files are never overwritten. Extraction only starts after a successful listing of the same archive. This is a preflight check; concurrent external changes to the archive or destination can invalidate it before `7zz` writes. Passwords are not accepted or persisted in this MVP.

`7zz t` runs asynchronously through the same adapter and cancellation path, without a destination. The window enables Test and Info for an open archive, including an empty one, and disables them during loading and other operations. The Info dialog uses `Type` and `Physical Size` only when `7zz l -slt` reports them in the archive header. File and folder counts and total original file size come from parsed entries. The extraction dialog states the no-overwrite rule and allows a typed or browsed destination. These Qt dialogs adapt the Windows `7zFM` Extract, Test and Info workflows; the toolbar position and file-list layout remain unchanged.
