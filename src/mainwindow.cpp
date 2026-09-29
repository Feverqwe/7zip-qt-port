#include "mainwindow.h"
#include "qtapppresentation.h"
#include "qtarchivefoldermanager.h"
#include "qtassociatedfileservices.h"

MainWindow::MainWindow(ArchiveBackend *backend, std::function<bool(const QUrl &)> fileOpener)
    : fileOpener_(fileOpener ? std::move(fileOpener) : QDesktopServices::openUrl) {
    backend_ = backend ? backend : new ArchiveBackend(this);
    setWindowTitle("Sevenzip Desktop");
    resize(980, 620);
    app_.NumPanels = 1;
    folderManager_ = new QtArchiveFolderManager(*backend_);
    associatedFileServices_ = std::make_unique<QtAssociatedFileServices>(fileOpener_);
    for (auto &panel : app_.Panels) {
        panel.AttachFolderManager(folderManager_, toolbarSettingsPath());
        panel.AttachExternalFileServices(associatedFileServices_.get());
    }
    appPresentation_ = std::make_unique<QtAppPresentation>(*this);
    connectFilesystemScanner();
    COpenResult opened;
    int widths[2] = {width() / 2, width() - width() / 2};
    const HRESULT result = app_.Create(this, ToUString(QDir::currentPath()), UString(), widths,
                                       false, opened, *appPresentation_, toolbarSettingsPath());
    if (result != S_OK)
        throw result;
    installEventFilter(this);
    renderBoundFolder();
    refreshActions();
    connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] {
        if (app_.GetFocusedPanel().ShouldProcessNotifications())
            refreshActions();
    });
    connect(table_, &QTableWidget::currentCellChanged, this, [this] {
        if (app_.GetFocusedPanel().ShouldProcessNotifications())
            refreshActions();
    });

    connectBackend();
}

void MainWindow::connectFilesystemScanner() {
    filesystemScanner_ = new FilesystemScanner(this);
    connect(filesystemScanner_, &FilesystemScanner::folderReady, this,
            [this](IFolderFolder *folder) {
                if (app_.QtOperations().closed())
                    return;
                QtAppOperationState::CallScope call(app_.QtOperations());
                auto &panel = app_.GetFocusedPanel();
                panel.SetNewFolder(folder);
                panel.LoadFullPath();
            });
    connect(filesystemScanner_, &FilesystemScanner::batchReady, this,
            [this](const QFileInfoList &files, quint32 firstSourceIndex) {
                if (app_.QtOperations().closed())
                    return;
                QtAppOperationState::CallScope call(app_.QtOperations());
                quint32 sourceIndex = firstSourceIndex;
                for (const QFileInfo &file : files)
                    appendFilesystemEntry(file, sourceIndex++);
                statusBar()->showMessage(
                    QStringLiteral("Показано: %1 объектов…").arg(table_->rowCount()));
            });
    connect(filesystemScanner_, &FilesystemScanner::advanced, this, [this](quint64 count) {
        if (app_.QtOperations().closed())
            return;
        statusBar()->showMessage(QStringLiteral("Сканирование: %1 объектов…").arg(count));
    });
    connect(filesystemScanner_, &FilesystemScanner::completed, this, [this] {
        if (app_.QtOperations().closed())
            return;
        QtAppOperationState::CallScope call(app_.QtOperations());
        scanningDisk_ = false;
        table_->setSortingEnabled(true);
        applySortMode();
        if (scanSelectedState_)
            app_.GetFocusedPanel().RefreshListCtrl(*scanSelectedState_);
        scanSelectedState_.reset();
        setState(State::Filesystem);
        statusBar()->showMessage(QStringLiteral("%1 объектов").arg(table_->rowCount()));
    });
    connect(filesystemScanner_, &FilesystemScanner::failed, this, [this](qint32) {
        if (app_.QtOperations().closed())
            return;
        QtAppOperationState::CallScope call(app_.QtOperations());
        scanningDisk_ = false;
        scanSelectedState_.reset();
        table_->clearSelection();
        table_->setRowCount(0);
        app_.GetFocusedPanel().ReleaseFolder();
        table_->setSortingEnabled(true);
        setState(State::Filesystem);
        statusBar()->showMessage(QStringLiteral("Не удалось прочитать папку."));
    });
}
