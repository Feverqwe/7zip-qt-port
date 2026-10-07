#include "mainwindow.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "Windows/PropVariantConv.h"
#include "qtapppresentation.h"
#include "qtarchivefoldermanager.h"
#include "qtassociatedfileservices.h"
#include "qtfilemanagercommands.h"
#include "qtfilemanagerdrag.h"
#include "qtfilemanagerstartup.h"
#include "qtlangcontrols.h"
#include "qtlanguageservices.h"
#include "qtprogramfilemenu.h"
#include "qttrashservice.h"
#include <QScopeGuard>
#include <QScrollBar>

MainWindow::MainWindow(ArchiveBackend *backend, std::function<bool(const QUrl &)> fileOpener,
                       IQtTrashService *trashService, const QtFileManagerStartup *startup)
    : fileOpener_(fileOpener ? std::move(fileOpener) : QDesktopServices::openUrl) {
    if (trashService)
        trashService_ = trashService;
    else {
        nativeTrashService_ = std::make_unique<QtNativeTrashService>();
        trashService_ = nativeTrashService_.get();
    }
    backend_ = backend ? backend : new ArchiveBackend(this);
    ReloadLang(toolbarSettingsPath());
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
        [this](CPanel &panel, const QtArchiveUpdateBody &body) {
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
    const UString initialPath = startup         ? startup->FullPath
                                : panelsDefined ? UString()
                                                : ToUString(QDir::currentPath());
    const HRESULT result = app_.Create(this, initialPath, startup ? startup->ArcFormat : UString(),
                                       widths, startup && startup->NeedOpenArc, opened,
                                       *appPresentation_, toolbarSettingsPath());
    if (startup)
        CheckFileManagerStartup(result, *startup, opened);
    if (result != S_OK)
        throw result;
    commands_ = std::make_unique<QtFileManagerCommands>(*this);
    commands_->AttachActions();
    programFileMenu_ = std::make_unique<QtProgramFileMenu>(*this);
    drag_ = std::make_unique<QtFileManagerDrag>(*this);
    for (unsigned index = 0; index < kNumPanelsMax; ++index)
        app_.Panels[index].AttachInvalidationPresentation([this, index] {
            if (index == app_.GetFocusedPanelIndex())
                redrawSourceProperties();
        });
    installEventFilter(this);
    const unsigned focusedPanel =
        panelsDefined && windowInfo.currentPanel < kNumPanelsMax ? windowInfo.currentPanel : 0;
    if (focusedPanel != projectedPanel_)
        activatePanelUi(focusedPanel);
    app_.SetFocusedPanel(focusedPanel);
    appPresentation_->RequestInitialFocus(focusedPanel);
    QTimer::singleShot(0, this, [this, panelsDefined] {
        appPresentation_->CompleteInitialFocusIfReady();
        if (!panelsDefined || app_.NumPanels != 2 || !panelSplitter_)
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
    reloadLangItems();
    connect(backend_, &ArchiveBackend::codecCatalogReady, this,
            [this](bool, const QString &message) {
                refreshActions();
                if (!message.isEmpty())
                    statusBar()->showMessage(message);
            });
    if (!backend) {
        backend_->discoverCodecCatalog();
    }
}

void MainWindow::connectFilesystemScanner() {
    for (unsigned index = 0; index < kNumPanelsMax; ++index) {
        auto &ui = panelUi_[index];
        ui.listProjection = new QtFolderProjection(this);
        ui.filesystemScanner = new FilesystemScanner(this);
        auto *scanner = ui.filesystemScanner;
        connect(scanner, &FilesystemScanner::folderReady, this,
                [this, index](IFolderFolder *folder) {
                    withPanelProjection(index, [&] {
                        if (app_.QtOperations().closed())
                            return;
                        QtAppOperationState::CallScope call(app_.QtOperations());
                        auto &panel = app_.GetFocusedPanel();
                        panel.SetNewFolder(folder);
                        panel.LoadFullPath();
                        const HRESULT loaded = panel.LoadListItems(true);
                        if (loaded != S_OK) {
                            panelUi_[index].filesystemScanner->cancel();
                            failFilesystemListing(loaded);
                            return;
                        }
                        configureColumns(true);
                        appendParentEntry();
                    });
                });
        connect(scanner, &FilesystemScanner::batchReady, this,
                [this, index](const QFileInfoList &files, quint32 firstSourceIndex) {
                    withPanelProjection(index, [&] {
                        if (app_.QtOperations().closed())
                            return;
                        QtAppOperationState::CallScope call(app_.QtOperations());
                        quint32 sourceIndex = firstSourceIndex;
                        for (const QFileInfo &file : files) {
                            const HRESULT result = appendFilesystemEntry(file, sourceIndex++);
                            if (result != S_OK) {
                                panelUi_[index].filesystemScanner->cancel();
                                failFilesystemListing(result);
                                return;
                            }
                        }
                        statusBar()->showMessage(
                            QtLocalText("Shown: %1 items…", "Показано: %1 объектов…")
                                .arg(table_->rowCount()));
                    });
                });
        connect(scanner, &FilesystemScanner::advanced, this, [this, index](quint64 count) {
            if (index != projectedPanel_)
                return;
            if (app_.QtOperations().closed())
                return;
            statusBar()->showMessage(QtLangText(IDS_SCANNING) +
                                     QtLocalText(" %1 items", " %1 объектов").arg(count));
        });
        connect(scanner, &FilesystemScanner::completed, this, [this, index] {
            withPanelProjection(index, [this] {
                if (app_.QtOperations().closed())
                    return;
                QtAppOperationState::CallScope call(app_.QtOperations());
                scanningDisk_ = false;
                applySortMode(true);
                completeListSelection();
                restoreAutoRefreshViewport();
                setState(State::Filesystem);
                statusBar()->showMessage(
                    QtLocalText("%1 items", "%1 объектов").arg(table_->rowCount()));
            });
        });
        connect(scanner, &FilesystemScanner::failed, this, [this, index](qint32 result) {
            withPanelProjection(index, [this, result] {
                if (app_.QtOperations().closed())
                    return;
                QtAppOperationState::CallScope call(app_.QtOperations());
                app_.GetFocusedPanel().LoadListItems(true, HRESULT(result));
                failFilesystemListing(HRESULT(result));
            });
        });
    }
    filesystemScanner_ = panelUi_[projectedPanel_].filesystemScanner;
    listProjection_ = panelUi_[projectedPanel_].listProjection;
}

void MainWindow::failFilesystemListing(HRESULT result) {
    auto &panel = app_.GetFocusedPanel();
    scanSelectedState_.reset();
    scanSelectedPaths_.reset();
    autoRefreshTopPath_.clear();
    autoRefreshScrollValue_ = -1;
    table_->clearSelection();
    table_->setRowCount(0);
    table_->setSortingEnabled(true);
    // Keep the async publication pending until its error has been presented.
    // The original OnCopy continuation then restores source focus last.
    panel.MessageBox_Error_HRESULT(result);
    if (app_.QtOperations().closed())
        return;
    scanningDisk_ = false;
    setState(State::Filesystem);
    statusBar()->showMessage(QtLocalText("Could not read folder.", "Не удалось прочитать папку."));
}
