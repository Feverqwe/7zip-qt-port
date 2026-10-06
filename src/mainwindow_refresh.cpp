#include "mainwindow.h"
#include "qtarchivereopen.h"
#include "qtlanguageservices.h"
#include "qtlocationfolder.h"

#include <QScopeGuard>
#include <QScopedValueRollback>
#include <QScrollBar>

HRESULT MainWindow::reloadPanelPresentation(unsigned index, const CSelectedState &selection) {
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
    if (state_ == State::Viewing) {
        renderBoundFolder();
        restoreListSelection(selection);
        return S_OK;
    }
    if (state_ == State::Namespace) {
        renderRootFolder();
        restoreListSelection(selection);
        return S_OK;
    }
    if (state_ != State::Filesystem)
        return S_OK;
    if (!QFileInfo(directory_).isDir()) {
        autoRefreshSnapshot_.clear();
        autoRefreshSnapshot_.insert(directory_, filesystemSignature(directory_));
        table_->clearSelection();
        table_->setRowCount(0);
        statusBar()->showMessage(QtLocalText("Folder not found: ", "Папка не найдена: ") +
                                 directory_);
        refreshActions();
        return S_OK;
    }
    const int firstRow = table_->rowAt(0);
    autoRefreshTopPath_ =
        firstRow >= 0 && table_->item(firstRow, 1) ? table_->item(firstRow, 1)->text() : QString();
    autoRefreshScrollValue_ = table_->verticalScrollBar()->value();
    refreshCurrent(&selection);
    return S_OK;
}

void MainWindow::refreshCurrent(const CSelectedState *preserved) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (state_ == State::Namespace) {
        auto &panel = app_.GetFocusedPanel();
        CSelectedState selection;
        if (preserved)
            selection = *preserved;
        else
            panel.SaveSelectedState(selection);
        renderRootFolder();
        panel.RefreshListCtrl(selection);
        refreshActions();
        return;
    }
    if (state_ == State::Filesystem) {
        CSelectedState selection;
        if (preserved)
            selection = *preserved;
        else
            app_.GetFocusedPanel().SaveSelectedState(selection);
        showDirectory(directory_, false);
        restoreListSelection(selection);
        if (!scanningDisk_) {
            restoreAutoRefreshViewport();
            refreshActions();
        }
    } else if (state_ == State::Viewing) {
        auto &panel = app_.GetFocusedPanel();
        CSelectedState selection;
        if (preserved)
            selection = *preserved;
        else
            panel.SaveSelectedState(selection);
        CMyComPtr<IFolderFolder> oldFolder = panel.GetFolder();
        auto *folder = dynamic_cast<CAgentFolder *>((IFolderFolder *)oldFolder);
        if (!folder)
            return;
        const UString previousFolderPath = GetFolderPath(oldFolder);
        setState(State::Loading);
        CMyComPtr<IFolderFolder> refreshed;
        HRESULT result;
        {
            QScopedValueRollback<bool> source(sourceArchiveProgress_, true);
            const auto &parents = panel.GetParentFolders();
            result = ReOpenArchiveFolder(*folder, toolbarSettingsPath(), this, &refreshed,
                                         !parents.IsEmpty() && parents.Back().UsePassword,
                                         parents.IsEmpty() ? UString() : parents.Back().Password);
        }
        if (result == S_OK) {
            panel.SetNewFolder(refreshed);
            renderBoundFolder();
            restoreListSelection(GetFolderPath(refreshed) == previousFolderPath ? selection
                                                                                : CSelectedState());
            refreshActions();
        } else {
            // Native ReOpen releases the old proxy; never render stale folders.
            showDirectory(returnDirectory_, false);
            if (result != E_ABORT)
                QMessageBox::warning(
                    this, QStringLiteral("7-Zip"),
                    QtLocalText("Could not update archive (%1).", "Не удалось обновить архив (%1).")
                        .arg(quint32(result), 0, 16));
        }
    }
}

MainWindow::FilesystemSignature MainWindow::filesystemSignature(const QString &path) const {
    const QFileInfo info(path);
    return {info.lastModified(), info.metadataChangeTime(), info.size(), info.exists(),
            info.isDir()};
}

void MainWindow::pollAutoRefresh() {
    // Native modal commands hold panel timer/notification guards while their
    // workers can rebuild archive proxies. Do not change the Qt projection
    // (and query the other panel) before CPanel::OnTimer sees those guards.
    if (scanningDisk_ || app_.QtOperations().active() || app_.QtOperations().closed() ||
        app_.QtOperations().callActive() || sourceArchiveProgress_)
        return;
    const unsigned original = projectedPanel_;
    for (unsigned index = 0; index < kNumPanelsMax; ++index) {
        if (!app_.IsPanelVisible(index) || !panelUi_[index].container)
            continue;
        if (index != projectedPanel_)
            activatePanelUi(index);
        if (state_ == State::Filesystem) {
            CPanel &panel = app_.GetFocusedPanel();
            if (panel.AutoRefresh_Mode && !autoRefreshSnapshot_.isEmpty()) {
                for (auto it = autoRefreshSnapshot_.cbegin(); it != autoRefreshSnapshot_.cend();
                     ++it) {
                    if (!(filesystemSignature(it.key()) == it.value())) {
                        panel.MarkPortableFolderChanged();
                        break;
                    }
                }
            }
            panel.OnTimer();
        }
        if (app_.QtOperations().active())
            break;
    }
    if (original != projectedPanel_)
        activatePanelUi(original);
}

void MainWindow::restoreAutoRefreshViewport() {
    if (autoRefreshScrollValue_ < 0)
        return;
    int row = -1;
    for (int i = 0; i < table_->rowCount(); ++i) {
        if (table_->item(i, 1) && table_->item(i, 1)->text() == autoRefreshTopPath_) {
            row = i;
            break;
        }
    }
    if (row >= 0)
        table_->scrollToItem(table_->item(row, 0), QAbstractItemView::PositionAtTop);
    else
        table_->verticalScrollBar()->setValue(autoRefreshScrollValue_);
    autoRefreshTopPath_.clear();
    autoRefreshScrollValue_ = -1;
}
