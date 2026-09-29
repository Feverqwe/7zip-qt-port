#include "mainwindow.h"
#include "Windows/PropVariantConv.h"
#include "qtapppresentation.h"
#include "qtarchivefoldermanager.h"
#include "qtassociatedfileservices.h"
#include "qtfilemanagercommands.h"
#include <QScrollBar>

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
    QSettings viewSettings(toolbarSettingsPath(), QSettings::IniFormat);
    const int timestampLevel =
        viewSettings.value(QStringLiteral("view/timestampLevel"), kTimestampPrintLevel_MIN).toInt();
    for (const int valid :
         {kTimestampPrintLevel_DAY, kTimestampPrintLevel_MIN, kTimestampPrintLevel_SEC,
          kTimestampPrintLevel_NTFS, kTimestampPrintLevel_NS}) {
        if (timestampLevel == valid)
            app_.SetTimestampLevel(timestampLevel);
    }
    g_Timestamp_Show_UTC = viewSettings.value(QStringLiteral("view/timestampUTC"), false).toBool();
    app_.AutoRefresh_Mode = viewSettings.value(QStringLiteral("view/autoRefresh"), true).toBool();
    app_.SetPanels_AutoRefresh_Mode();
    appPresentation_ = std::make_unique<QtAppPresentation>(*this);
    connectFilesystemScanner();
    COpenResult opened;
    int widths[2] = {width() / 2, width() - width() / 2};
    const HRESULT result = app_.Create(this, ToUString(QDir::currentPath()), UString(), widths,
                                       false, opened, *appPresentation_, toolbarSettingsPath());
    if (result != S_OK)
        throw result;
    commands_ = std::make_unique<QtFileManagerCommands>(*this);
    commands_->AttachActions();
    app_.GetFocusedPanel().AttachReloadPresentation([this](const CSelectedState &selection) {
        if (state_ != State::Filesystem || app_.QtOperations().active())
            return S_OK;
        if (!QFileInfo(directory_).isDir()) {
            autoRefreshSnapshot_.clear();
            autoRefreshSnapshot_.insert(directory_, filesystemSignature(directory_));
            table_->clearSelection();
            table_->setRowCount(0);
            statusBar()->showMessage(QStringLiteral("Папка не найдена: ") + directory_);
            refreshActions();
            return S_OK;
        }
        const int firstRow = table_->rowAt(0);
        autoRefreshTopPath_ = firstRow >= 0 && table_->item(firstRow, 1)
                                  ? table_->item(firstRow, 1)->text()
                                  : QString();
        autoRefreshScrollValue_ = table_->verticalScrollBar()->value();
        refreshCurrent(&selection);
        return S_OK;
    });
    installEventFilter(this);
    renderBoundFolder();
    autoRefreshTimer_ = new QTimer(this);
    autoRefreshTimer_->setInterval(1000);
    connect(autoRefreshTimer_, &QTimer::timeout, this, [this] { pollAutoRefresh(); });
    autoRefreshTimer_->start();
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
                configureColumns();
                appendParentEntry();
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
        restoreAutoRefreshViewport();
        setState(State::Filesystem);
        statusBar()->showMessage(QStringLiteral("%1 объектов").arg(table_->rowCount()));
    });
    connect(filesystemScanner_, &FilesystemScanner::failed, this, [this](qint32) {
        if (app_.QtOperations().closed())
            return;
        QtAppOperationState::CallScope call(app_.QtOperations());
        scanningDisk_ = false;
        scanSelectedState_.reset();
        autoRefreshTopPath_.clear();
        autoRefreshScrollValue_ = -1;
        table_->clearSelection();
        table_->setRowCount(0);
        app_.GetFocusedPanel().ReleaseFolder();
        table_->setSortingEnabled(true);
        setState(State::Filesystem);
        statusBar()->showMessage(QStringLiteral("Не удалось прочитать папку."));
    });
}
