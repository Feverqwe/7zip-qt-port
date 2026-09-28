#include "mainwindow.h"

MainWindow::MainWindow(ArchiveBackend *backend, std::function<bool(const QUrl &)> fileOpener)
    : fileOpener_(fileOpener ? std::move(fileOpener) : QDesktopServices::openUrl) {
    backend_ = backend ? backend : new ArchiveBackend(this);
    setWindowTitle("Sevenzip Desktop");
    resize(980, 620);
    QMenu *editMenu = createMenusAndToolbar();
    createFilePanel(editMenu);
    connectFilesystemScanner();
    showDirectory(QDir::currentPath(), false);
    refreshActions();
    connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this] { refreshActions(); });
    connect(table_, &QTableWidget::currentCellChanged, this, [this] { refreshActions(); });

    connectBackend();
}

void MainWindow::connectFilesystemScanner() {
    filesystemScanner_ = new FilesystemScanner(this);
    connect(filesystemScanner_, &FilesystemScanner::batchReady, this,
            [this](const QFileInfoList &files) {
                for (const QFileInfo &file : files)
                    appendFilesystemEntry(file);
                statusBar()->showMessage(
                    QStringLiteral("Сканирование: %1 объектов…").arg(table_->rowCount()));
            });
    connect(filesystemScanner_, &FilesystemScanner::completed, this, [this] {
        scanningDisk_ = false;
        table_->setSortingEnabled(true);
        applySortMode();
        selectPaths(scanSelection_);
        scanSelection_.clear();
        refreshActions();
        statusBar()->showMessage(QStringLiteral("%1 объектов").arg(table_->rowCount()));
    });
}
