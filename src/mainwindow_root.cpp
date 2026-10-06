#include "7zip/PropID.h"
#include "Windows/PropVariant.h"
#include "mainwindow.h"
#include "qtlanguageservices.h"
#include "qtpaneladdresscombo.h"

// Source namespace folders own item names, order and binding. This file presents
// their IFolderFolder items in the same list used by disk and archive views.
void MainWindow::renderRootFolder() {
    auto &panel = app_.GetFocusedPanel();
    const bool drives = panel.IsFSDrivesFolder();
    const bool network = panel.IsNetFolder();
    const bool streams = panel.IsAltStreamsFolder();
    if ((!panel.IsRootFolder() && !drives && !network && !streams) || !panel.GetFolder())
        return;
    auto disableNotify = std::make_unique<CPanel::CDisableNotify>(panel);
    IFolderFolder *folder = panel.GetFolder();
    UInt32 count = 0;
    if (folder->LoadItems() != S_OK || folder->GetNumberOfItems(&count) != S_OK) {
        statusBar()->showMessage(
            QtLocalText("Could not read root folder.", "Не удалось прочитать корневую папку."));
        return;
    }
    filesystemScanner_->cancel();
    listProjection_->cancel();
    scanningArchive_ = false;
    scanningDisk_ = false;
    scanSelectedState_.reset();
    scanSelectedPaths_.reset();
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
    static_cast<QtPanelAddressCombo *>(pathCombo_)
        ->ShowCurrentPath(drives || network || streams
                              ? QString::fromWCharArray(GetFolderPath(folder))
                              : QString(),
                          false);
    recordVisitedPath(ToQString(panel.GetCurrentFolderPrefix()));
    configureColumns();
    for (UInt32 index = 0; index < count; ++index) {
        NWindows::NCOM::CPropVariant name;
        if (folder->GetProperty(index, kpidName, &name) != S_OK || name.vt != VT_BSTR)
            continue;
        const QString nativeName = QString::fromWCharArray(name.bstrVal);
        NWindows::NCOM::CPropVariant directory;
        const bool isDirectory = folder->GetProperty(index, kpidIsDir, &directory) == S_OK &&
                                 directory.vt == VT_BOOL && directory.boolVal != VARIANT_FALSE;
        const int row = table_->rowCount();
        table_->insertRow(row);
        for (int column = 0; column < table_->columnCount(); ++column) {
            auto *item = new ArchiveTableItem;
            item->setData(Qt::UserRole + 6, index);
            item->setData(Qt::UserRole + 3, nativeName);
            item->setData(Qt::UserRole + 1, isDirectory);
            if (!isDirectory)
                item->setData(Qt::UserRole, path_->text() + nativeName);
            if (column == 0) {
                item->setText(nativeName.normalized(QString::NormalizationForm_C));
                item->setIcon(QApplication::style()->standardIcon(!isDirectory ? QStyle::SP_FileIcon
                                                                  : drives ? QStyle::SP_DriveHDIcon
                                                                  : network
                                                                      ? QStyle::SP_DirIcon
                                                                      : QStyle::SP_ComputerIcon));
            } else if (drives || network || streams || column >= 7)
                item->setText(sourceColumnText(index, column));
            table_->setItem(row, column, item);
        }
    }
    table_->setSortingEnabled(true);
    applySortMode();
    // Restore the row-rebuild guard before End releases operation guards.
    // Otherwise this guard would restore its saved disabled state afterward.
    disableNotify.reset();
    setState(State::Namespace);
    statusBar()->showMessage(QtLocalText("%1 items", "%1 объектов").arg(table_->rowCount()));
}

void MainWindow::goRoot(bool remember, RootLocation location) {
    if (app_.QtOperations().closed() || state_ == State::Loading || state_ == State::Operating)
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (remember && state_ == State::Filesystem && !directory_.isEmpty())
        history_.append(directory_);
    else if (remember && state_ == State::Viewing && !returnDirectory_.isEmpty())
        history_.append(returnDirectory_);
    setState(State::Operating);
    if (location == RootLocation::Drives)
        app_.OpenRootFolder();
    else
        app_.GetFocusedPanel().OpenRootFolder();
    renderBoundFolder();
}
