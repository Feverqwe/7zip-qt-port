#include "mainwindow.h"
#include "Windows/PropVariantConv.h"
#include "qtapppresentation.h"
#include "qtarchivefoldermanager.h"
#include "qtassociatedfileservices.h"
#include "qtfilemanagercommands.h"
#include "qttrashservice.h"
#include <QScopeGuard>
#include <QScrollBar>

MainWindow::MainWindow(ArchiveBackend *backend, std::function<bool(const QUrl &)> fileOpener,
                       IQtTrashService *trashService)
    : fileOpener_(fileOpener ? std::move(fileOpener) : QDesktopServices::openUrl) {
    if (trashService)
        trashService_ = trashService;
    else {
        nativeTrashService_ = std::make_unique<QtNativeTrashService>();
        trashService_ = nativeTrashService_.get();
    }
    backend_ = backend ? backend : new ArchiveBackend(this);
    setWindowTitle("Sevenzip Desktop");
    resize(980, 620);
    CWindowInfo windowInfo{};
    bool windowPositionDefined = false;
    bool panelsDefined = false;
    windowInfo.Read(windowPositionDefined, panelsDefined, toolbarSettingsPath());
    app_.NumPanels =
        panelsDefined && windowInfo.numPanels >= 1 && windowInfo.numPanels <= kNumPanelsMax
            ? windowInfo.numPanels
            : 1;
    app_.LastFocusedPanel =
        panelsDefined && windowInfo.currentPanel < kNumPanelsMax ? windowInfo.currentPanel : 0;
    splitterPos_ = panelsDefined ? static_cast<int>(windowInfo.splitterPos) : width() / 2;
    folderManager_ = new QtArchiveFolderManager(*backend_);
    associatedFileServices_ = std::make_unique<QtAssociatedFileServices>(
        fileOpener_,
        [this](CPanel &panel, const std::function<bool()> &body) {
            return runExternalArchiveUpdate(panel, body);
        },
        [this] { return canHandleExternalUpdate(); });
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
    const HRESULT result =
        app_.Create(this, panelsDefined ? UString() : ToUString(QDir::currentPath()), UString(),
                    widths, false, opened, *appPresentation_, toolbarSettingsPath());
    if (result != S_OK)
        throw result;
    commands_ = std::make_unique<QtFileManagerCommands>(*this);
    commands_->AttachActions();
    for (unsigned index = 0; index < kNumPanelsMax; ++index)
        app_.Panels[index].AttachReloadPresentation([this, index](const CSelectedState &selection) {
            if (!panelUi_[index].container || app_.QtOperations().active())
                return S_OK;
            const unsigned original = projectedPanel_;
            const QString originalStatus = statusBar()->currentMessage();
            if (index != original)
                activatePanelUi(index);
            const auto restore = qScopeGuard([&] {
                if (index != original) {
                    activatePanelUi(original);
                    statusBar()->showMessage(originalStatus);
                }
            });
            if (state_ != State::Filesystem)
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
    const unsigned focusedPanel =
        panelsDefined && windowInfo.currentPanel < kNumPanelsMax ? windowInfo.currentPanel : 0;
    for (unsigned index = 0; index < kNumPanelsMax; ++index) {
        if (!app_.Panels[index].PanelCreated)
            continue;
        if (index != projectedPanel_)
            activatePanelUi(index);
        app_.SetFocusedPanel(index);
        renderBoundFolder();
    }
    if (focusedPanel != projectedPanel_)
        activatePanelUi(focusedPanel);
    app_.SetFocusedPanel(focusedPanel);
    if (panelsDefined)
        QTimer::singleShot(0, this, [this, focusedPanel] {
            if (focusedPanel != projectedPanel_)
                activatePanelUi(focusedPanel);
            app_.SetFocusedPanel(focusedPanel);
            app_.Panels[focusedPanel].SetFocusToList();
            app_.RefreshTitle(true);
            if (app_.NumPanels != 2 || !panelSplitter_)
                return;
            const int width = panelSplitter_->width();
            const int left = qBound(1, splitterPos_, width - 1);
            panelSplitter_->setSizes({left, width - left});
        });
    autoRefreshTimer_ = new QTimer(this);
    autoRefreshTimer_->setInterval(1000);
    connect(autoRefreshTimer_, &QTimer::timeout, this, [this] { pollAutoRefresh(); });
    autoRefreshTimer_->start();
    refreshActions();
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
