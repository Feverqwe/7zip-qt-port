#include "7zip/PropID.h"
#include "Windows/PropVariant.h"
#include "mainwindow.h"
#include "qtlanguageservices.h"
#include "qtpaneladdresscombo.h"

// Source namespace folders own item names, order and binding. This file presents
// their IFolderFolder items in the same list used by disk and archive views.
HRESULT MainWindow::renderRootFolder(bool updateAddress) {
    auto &panel = app_.GetFocusedPanel();
    if (!panel.GetFolder())
        return S_OK;
    const bool drives = panel.IsFSDrivesFolder();
    const bool network = panel.IsNetFolder();
    const bool streams = panel.IsAltStreamsFolder();
    if ((!panel.IsRootFolder() && !drives && !network && !streams) || !panel.GetFolder())
        return E_NOTIMPL;
    auto disableNotify = std::make_unique<CPanel::CDisableNotify>(panel);
    table_->setSortingEnabled(false);
    IFolderFolder *folder = panel.GetFolder();
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
    static_cast<FileTable *>(table_)->filesystem = false;
    if (updateAddress)
        static_cast<QtPanelAddressCombo *>(pathCombo_)
            ->ShowCurrentPath(drives || network || streams
                                  ? QString::fromWCharArray(GetFolderPath(folder))
                                  : QString(),
                              false);
    const auto finish = [&](HRESULT result) {
        if (result != S_OK)
            table_->setRowCount(0);
        table_->setSortingEnabled(true);
        applySortMode();
        // Restore the row-rebuild guard before End releases operation guards.
        // Otherwise this guard would restore its saved disabled state afterward.
        disableNotify.reset();
        setState(State::Namespace);
        statusBar()->showMessage(
            result == S_OK ? QtLocalText("%1 items", "%1 объектов").arg(table_->rowCount())
                           : QtLocalText("Could not read root folder.",
                                         "Не удалось прочитать корневую папку."));
        return result;
    };
    UInt32 count = 0;
    HRESULT result = panel.LoadListItems();
    if (result == S_OK)
        result = folder->GetNumberOfItems(&count);
    if (result != S_OK)
        return finish(result);
    recordVisitedPath(ToQString(panel.GetCurrentFolderPrefix()));
    configureColumns(true);
    for (UInt32 index = 0; index < count; ++index) {
        NWindows::NCOM::CPropVariant name;
        if (folder->GetProperty(index, kpidName, &name) != S_OK || name.vt != VT_BSTR)
            continue;
        const QString nativeName = QString::fromWCharArray(name.bstrVal);
        QIcon icon;
        result = panel.GetItemIcon(index, icon);
        if (result != S_OK)
            return finish(result);
        const bool isDirectory = panel.IsItem_Folder(index);
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
#ifdef Q_OS_WIN
                if (!panel.IsRootFolder())
                    item->setIcon(icon);
#endif
            } else if (drives || network || streams || column >= 7)
                item->setText(sourceColumnText(index, column));
            table_->setItem(row, column, item);
        }
    }
    return finish(S_OK);
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
