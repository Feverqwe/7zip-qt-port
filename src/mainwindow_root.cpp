#include "7zip/PropID.h"
#include "Windows/PropVariant.h"
#include "mainwindow.h"

// RootFolder owns item names, order and binding. This file presents its
// IFolderFolder items in the same list used by disk and archive views.
void MainWindow::renderRootFolder() {
    auto &panel = app_.GetFocusedPanel();
    if (!panel.IsRootFolder() || !panel.GetFolder())
        return;
    IFolderFolder *folder = panel.GetFolder();
    UInt32 count = 0;
    if (folder->LoadItems() != S_OK || folder->GetNumberOfItems(&count) != S_OK) {
        statusBar()->showMessage(QStringLiteral("Не удалось прочитать корневую папку."));
        return;
    }
    filesystemScanner_->cancel();
    scanningDisk_ = false;
    scanSelectedState_.reset();
    archive_.clear();
    displayArchive_.clear();
    archivePassword_.clear();
    archiveFolder_.clear();
    entries_.clear();
    autoRefreshSnapshot_.clear();
    table_->clearSelection();
    table_->setSortingEnabled(false);
    table_->setRowCount(0);
    static_cast<FileTable *>(table_)->filesystem = false;
    path_->clear(); // The source root's current folder prefix is empty.
    configureColumns();
    for (UInt32 index = 0; index < count; ++index) {
        NWindows::NCOM::CPropVariant name;
        if (folder->GetProperty(index, kpidName, &name) != S_OK || name.vt != VT_BSTR)
            continue;
        const QString nativeName = QString::fromWCharArray(name.bstrVal);
        const int row = table_->rowCount();
        table_->insertRow(row);
        for (int column = 0; column < table_->columnCount(); ++column) {
            auto *item = new ArchiveTableItem;
            item->setData(Qt::UserRole + 6, index);
            item->setData(Qt::UserRole + 3, nativeName);
            item->setData(Qt::UserRole + 1, true);
            if (column == 0) {
                item->setText(nativeName.normalized(QString::NormalizationForm_C));
                item->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
            } else if (column >= 7)
                item->setText(sourceColumnText(index, column));
            table_->setItem(row, column, item);
        }
    }
    table_->setSortingEnabled(true);
    applySortMode();
    setState(State::Namespace);
    statusBar()->showMessage(QStringLiteral("%1 объектов").arg(table_->rowCount()));
}

void MainWindow::goRoot(bool remember) {
    if (app_.QtOperations().closed() || state_ == State::Loading || state_ == State::Operating)
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (remember && state_ == State::Filesystem && !directory_.isEmpty())
        history_.append(directory_);
    else if (remember && state_ == State::Viewing && !returnDirectory_.isEmpty())
        history_.append(returnDirectory_);
    setState(State::Operating);
    app_.GetFocusedPanel().OpenRootFolder();
    renderBoundFolder();
}
