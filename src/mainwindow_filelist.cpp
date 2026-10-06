#include "7zip/PropID.h"
#include "7zip/UI/FileManager/resource.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "CPP/7zip/UI/FileManager/BrowseDialog.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "CPP/7zip/UI/FileManager/PanelSort.h"
#include "CPP/7zip/UI/FileManager/SysIconUtils.h"
#include "Windows/PropVariant.h"
#include "mainwindow.h"
#include "qtarchivereopen.h"
#include "qtlangcontrols.h"
#include "qtlanguageservices.h"
#include "qtlocationfolder.h"
#include "qtpaneladdresscombo.h"
#include <QScopeGuard>
#include <QScopedValueRollback>
#include <QScrollBar>

HRESULT MainWindow::showDirectory(const QString &path, bool remember, bool sourceAlreadyBound,
                                  bool updateAddress) {
    if (app_.QtOperations().closed())
        return E_ABORT;
    QtAppOperationState::CallScope call(app_.QtOperations());
    const QFileInfo info(path);
    if (!info.isDir()) {
        statusBar()->showMessage(QtLocalText("Folder not found: ", "Папка не найдена: ") + path);
        path_->setText(state_ == State::Viewing ? archiveDisplayPath() : directory_);
        return E_FAIL;
    }
    const QString next = info.canonicalFilePath();
    if (remember && state_ == State::Namespace)
        history_.append(ToQString(GetFolderPath(app_.GetFocusedPanel().GetFolder())));
    else if (remember && !directory_.isEmpty() && directory_ != next)
        history_.append(directory_);
    filesystemScanner_->cancel();
    listProjection_->cancel();
    scanningArchive_ = false;
    scanningDisk_ = false;
    scanSelectedState_.reset();
    scanSelectedPaths_.reset();
    if (!sourceAlreadyBound)
        app_.GetFocusedPanel().CloseOpenFolders();
    directory_ = next;
    autoRefreshSnapshot_.clear();
    autoRefreshSnapshot_.insert(directory_, filesystemSignature(directory_));
    recordVisitedPath(directory_);
    archive_.clear();
    displayArchive_.clear();
    archivePassword_.clear();
    archiveFolder_.clear();
    {
        const QSignalBlocker blocked(flatView_);
        flatView_->setChecked(flatDisk_);
    }
    entries_.clear();
    renameEditingItem_ = nullptr;
    renameOriginalPath_.clear();
    table_->clearSelection();
    table_->setSortingEnabled(false);
    table_->setRowCount(0);
    if (updateAddress)
        path_->setText(directory_);
    static_cast<FileTable *>(table_)->filesystem = true;
    return startFilesystemListing(sourceAlreadyBound, updateAddress);
}

HRESULT MainWindow::startFilesystemListing(bool sourceAlreadyBound, bool updateAddress) {
    COpenResult opened;
    auto &panel = app_.GetFocusedPanel();
    HRESULT result =
        sourceAlreadyBound ? S_OK : panel.BindToPath(ToUString(directory_), UString(), opened);
    CMyComPtr<IFolderFolder> folder = panel.GetFolder();
    if (result == S_OK && !panel.IsFSFolder()) {
        return renderBoundFolder(updateAddress);
    }
    if (result == S_OK && updateAddress)
        static_cast<QtPanelAddressCombo *>(pathCombo_)->ShowCurrentPath(directory_, true);
    if (flatDisk_) {
        scanningDisk_ = true;
        setState(State::Filesystem);
        statusBar()->showMessage(QtLangText(IDS_SCANNING));
        filesystemScanner_->start(directory_);
    } else {
        if (result == S_OK) {
            auto *spec = static_cast<NFsFolder::CFSFolder *>((IFolderFolder *)folder);
            bool defer = false;
            {
                auto previous = std::move(spec->SetCompleted);
                const auto restore = qScopeGuard([&] { spec->SetCompleted = std::move(previous); });
                // Preserve immediate small-folder callers. Stop the original
                // load at its callback boundary, then rerun it in the existing
                // worker for a larger list instead of walking all files here.
                spec->SetCompleted = [&](quint64 count) {
                    defer = count > 256;
                    return !defer;
                };
                result = panel.LoadListItems();
            }
            if (result == E_ABORT && defer) {
                scanningDisk_ = true;
                setState(State::Filesystem);
                statusBar()->showMessage(QtLangText(IDS_SCANNING));
                filesystemScanner_->start(directory_, false);
                return S_OK;
            }
        }
        if (result == S_OK) {
            configureColumns(true);
            appendParentEntry();
            auto *spec = static_cast<NFsFolder::CFSFolder *>((IFolderFolder *)folder);
            for (UInt32 i = 0; i < spec->Files.Size(); ++i) {
                result = appendFilesystemEntry(
                    QFileInfo(QDir(directory_)
                                  .filePath(QtFilesystemPath(spec->GetRelPath(spec->Files[i])))),
                    i);
                if (result != S_OK) {
                    table_->setRowCount(0);
                    break;
                }
            }
        }
        applySortMode(true);
        setState(State::Filesystem);
        statusBar()->showMessage(
            result == S_OK ? QtLocalText("%1 items", "%1 объектов").arg(table_->rowCount())
                           : QtLocalText("Could not read folder.", "Не удалось прочитать папку."));
    }
    return result;
}

HRESULT MainWindow::appendFilesystemEntry(const QFileInfo &file, quint32 sourceIndex) {
    QIcon icon;
    RINOK(app_.GetFocusedPanel().GetItemIcon(sourceIndex, icon))
#ifdef Q_OS_WIN
    // Normal Windows folders already use source FindChangeNotification. Keep
    // the portable fallback for flat descendants, outside its nonrecursive watch.
    if (flatDisk_)
#endif
        autoRefreshSnapshot_.insert(file.absoluteFilePath(),
                                    filesystemSignature(file.absoluteFilePath()));
    IFolderFolder *folder = app_.GetFocusedPanel().GetFolder();
    NWindows::NCOM::CPropVariant size, name, prefix;
    folder->GetProperty(sourceIndex, kpidSize, &size);
    folder->GetProperty(sourceIndex, kpidName, &name);
    folder->GetProperty(sourceIndex, kpidPrefix, &prefix);
    const bool directory = app_.GetFocusedPanel().IsItem_Folder(sourceIndex);
    const QString nativeName =
        name.vt == VT_BSTR ? QString::fromWCharArray(name.bstrVal) : file.fileName();
    const QString nativePrefix =
        prefix.vt == VT_BSTR ? QDir::fromNativeSeparators(QString::fromWCharArray(prefix.bstrVal))
                             : QString();
    const QVariant sizeValue =
        size.vt == VT_UI8 ? QVariant::fromValue(quint64(size.uhVal.QuadPart)) : QVariant();
    const int row = table_->rowCount();
    table_->insertRow(row);
    const QString displayName = ((flatDisk_ ? nativePrefix : QString()) + nativeName)
                                    .normalized(QString::NormalizationForm_C);
    QStringList values = {displayName,
                          file.absoluteFilePath(),
                          directory ? QString() : sizeValue.toString(),
                          QString(),
                          QString(),
                          GetExtensionPtr(QStringView(nativeName)).toString(),
                          QString::number(sourceIndex)};
    for (int col = 7; col < table_->columnCount(); ++col)
        values << QString();
    for (int col = 0; col < values.size(); ++col) {
        auto *item = new ArchiveTableItem(values[col]);
        item->setData(Qt::UserRole + 1, directory);
        item->setData(Qt::UserRole + 3, nativeName);
        item->setData(Qt::UserRole + 5, nativePrefix);
        item->setData(Qt::UserRole + 6, sourceIndex);
        if (col == 0) {
            item->setToolTip(file.absoluteFilePath());
            item->setIcon(icon);
        }
        if (col == 2 && !directory)
            item->setData(Qt::UserRole + 2, sizeValue);
        if (col == 6)
            item->setData(Qt::UserRole + 2, sourceIndex);
        if (col != 1 && col != 5 && col != 6)
            item->setTextAlignment(sourceColumnAlignment(col));
        table_->setItem(row, col, item);
    }
    return S_OK;
}

QStringList MainWindow::selectedPaths() const {
    QStringList paths;
    for (const QModelIndex &index : table_->selectionModel()->selectedRows())
        paths << table_->item(index.row(), 1)->text();
    return paths;
}

void MainWindow::selectPaths(const QStringList &paths) {
    if (scanningDisk_ || scanningArchive_) {
        scanSelectedPaths_ = paths;
        return;
    }
    table_->clearSelection();
    const auto &panel = app_.GetFocusedPanel();
    const bool streams = panel.GetFolder() && panel.IsAltStreamsFolder();
    for (int row = 0; row < table_->rowCount(); ++row) {
        const QString path = streams ? table_->item(row, 0)->data(Qt::UserRole).toString()
                                     : table_->item(row, 1)->text();
        if (paths.contains(path))
            table_->selectionModel()->select(table_->model()->index(row, 0),
                                             QItemSelectionModel::Select |
                                                 QItemSelectionModel::Rows);
    }
}

void MainWindow::createFolder() {
    if (state_ != State::Filesystem && state_ != State::Viewing)
        return;
    const bool archive = state_ == State::Viewing;
    if (archive && !createFolder_->isEnabled())
        return;
    QString name;
    if (!Dlg_CreateFolder(this, name))
        return;
    UString createdName;
    const HRESULT result = app_.GetFocusedPanel().CreateFolder(ToUString(name), createdName);
    if (result == E_ABORT)
        return;
    if (result != S_OK) {
        QMessageBox::warning(this, "Sevenzip Desktop", "Не удалось создать папку.");
        return;
    }
    if (archive) {
        renderBoundFolder();
        return;
    }
    showDirectory(directory_, false);
    selectPaths({QDir(directory_).filePath(QDir::fromNativeSeparators(name).section('/', 0, 0))});
}

void MainWindow::createFile() {
    const auto &panel = app_.GetFocusedPanel();
    const bool streams = panel.GetFolder() && panel.IsAltStreamsFolder();
    if (state_ != State::Filesystem && !streams)
        return;
    ComboDialog dialog("Создать файл", "Имя файла:", "Новый файл", this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString name = dialog.value();
    UString createdName;
    const HRESULT result = app_.GetFocusedPanel().CreateFile(ToUString(name), createdName);
    if (result == E_ABORT)
        return;
    if (result != S_OK) {
        QMessageBox::warning(this, "Создать файл", "Не удалось создать файл.");
        return;
    }
    if (streams) {
        const QString target =
            ToQString(app_.GetFocusedPanel().GetCurrentFolderPrefix()) + ToQString(createdName);
        renderBoundFolder();
        selectPaths({target});
        return;
    }
    const QString target = QDir(directory_).filePath(ToQString(createdName));
    showDirectory(directory_, false);
    selectPaths({target});
}

void MainWindow::openPath(const QString &path) { navigateTo(QFileInfo(path).absoluteFilePath()); }

HRESULT MainWindow::navigateTo(const QString &path, bool sourceAddress) {
    if (state_ == State::Loading || state_ == State::Operating)
        return E_ABORT;
    if (state_ == State::Viewing && path == archiveDisplayPath())
        return S_OK;
    // A typed relative address belongs to the displayed filesystem folder.
    // CPanel::BindToPath still resolves the resulting native path.
    const QString nativeAddress =
        !sourceAddress && state_ == State::Filesystem && QDir::isRelativePath(path)
            ? QDir(directory_).filePath(path)
            : path;
    const QFileInfo info(nativeAddress);
    if (!sourceAddress && info.isDir())
        showDirectory(nativeAddress, true);
    else if (!sourceAddress && info.isFile() && !info.isSymLink())
        openArchive(nativeAddress);
    else {
        QString nativePath = nativeAddress;
        if (!sourceAddress)
            nativePath.replace(QStringLiteral(" / "), QStringLiteral("/"));
        auto &panel = app_.GetFocusedPanel();
        HRESULT result;
        {
            QScopedValueRollback<bool> source(sourceArchiveProgress_, true);
            COpenResult opened;
            result = panel.BindToPath(ToUString(nativePath), UString(), opened);
        }
        renderBoundFolder();
        if (result != S_OK && result != E_ABORT)
            statusBar()->showMessage(QtLocalText("Path not found: ", "Путь не найден: ") + path);
        return result;
    }
    return S_OK;
}

void MainWindow::activateRow(int row) {
    if (renameActivationPending_)
        return;
    if (row < 0 || row >= table_->rowCount())
        return;
    if (table_->item(row, 0)->data(Qt::UserRole + 6).toUInt() == UInt32(-1)) {
        goParent();
        return;
    }
    if (!table_->selectionModel()->isRowSelected(row, QModelIndex()))
        table_->selectRow(row);
    table_->setCurrentCell(row, 0, QItemSelectionModel::NoUpdate);
    app_.ExecuteCommand(IDM_OPEN);
}
