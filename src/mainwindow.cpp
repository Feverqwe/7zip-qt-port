#include "mainwindow.h"

MainWindow::MainWindow(ArchiveBackend *backend, std::function<bool(const QUrl &)> fileOpener)
    : fileOpener_(fileOpener ? std::move(fileOpener) : QDesktopServices::openUrl) {
    backend_ = backend ? backend : new ArchiveBackend(this);
    setWindowTitle("Sevenzip Desktop");
    resize(980, 620);
    app_.NumPanels = 1;
    app_.AppState.Read(toolbarSettingsPath());
    QMenu *editMenu = createMenusAndToolbar();
    createFilePanel(editMenu);
    app_.GetFocusedPanel().AttachSelectionView(table_);
    installEventFilter(this);
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
                    QStringLiteral("Показано: %1 объектов…").arg(table_->rowCount()));
            });
    connect(filesystemScanner_, &FilesystemScanner::advanced, this, [this](quint64 count) {
        statusBar()->showMessage(QStringLiteral("Сканирование: %1 объектов…").arg(count));
    });
    connect(filesystemScanner_, &FilesystemScanner::completed, this, [this] {
        scanningDisk_ = false;
        table_->setSortingEnabled(true);
        applySortMode();
        if (scanSelectedState_)
            app_.GetFocusedPanel().RefreshListCtrl(*scanSelectedState_);
        scanSelectedState_.reset();
        refreshActions();
        statusBar()->showMessage(QStringLiteral("%1 объектов").arg(table_->rowCount()));
    });
    connect(filesystemScanner_, &FilesystemScanner::failed, this, [this](qint32) {
        scanningDisk_ = false;
        scanSelectedState_.reset();
        table_->setSortingEnabled(true);
        refreshActions();
        statusBar()->showMessage(QStringLiteral("Не удалось прочитать папку."));
    });
}
