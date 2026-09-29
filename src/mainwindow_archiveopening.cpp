#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include "mainwindow.h"
#include "qtarchivefileresource.h"
#include <QScopedValueRollback>

void MainWindow::openArchive(const QString &path) {
    if (state_ == State::Loading || state_ == State::Operating)
        return;
    const QFileInfo file(path);
    if (!file.isFile() || file.isSymLink()) {
        statusBar()->showMessage(QStringLiteral("Файл не найден: ") + path);
        return;
    }
    filesystemScanner_->cancel();
    scanningDisk_ = false;
    scanSelectedState_.reset();
    if (infoDialog_)
        infoDialog_->close();
    if (state_ == State::Filesystem)
        returnDirectory_ = directory_;
    auto &panel = app_.GetFocusedPanel();
    operationName_ = QStringLiteral("Открытие архива");
    journalText_.clear();
    if (journalView_)
        journalView_->clear();
    appendJournal(operationName_ + ": " + file.absoluteFilePath());
    journalAction_->setEnabled(true);
    QString message;
    const auto completion = connect(backend_, &ArchiveBackend::finished, this,
                                    [&message](bool, const QString &text) { message = text; });
    setState(State::Loading);
    HRESULT result;
    {
        QScopedValueRollback<bool> source(sourceArchiveProgress_, true);
        COpenResult opened;
        result = panel.BindToPath(ToUString(file.absoluteFilePath()), UString(), opened);
    }
    disconnect(completion);
    if (panel.IsArcFolder())
        renderBoundFolder();
    else
        showDirectory(returnDirectory_, false);
    if (result == E_ABORT)
        message = QStringLiteral("Операция отменена.");
    else if (message.isEmpty())
        message =
            result == S_OK ? QStringLiteral("Архив открыт.") : ToQString(HResultToMessage(result));
    appendJournal((result == S_OK ? QStringLiteral("Результат: ") : QStringLiteral("Ошибка: ")) +
                  message);
    statusBar()->showMessage(message);
    if (result != S_OK && result != E_ABORT)
        QMessageBox::warning(this, QStringLiteral("Sevenzip Desktop"), message);
}

bool MainWindow::isNestedArchive() const {
    return app_.Panels[app_.GetFocusedPanelIndex()].GetParentFolders().Size() > 1;
}

void MainWindow::renderBoundFolder() {
    auto &panel = app_.GetFocusedPanel();
    const auto selection = panel.TakeRefreshSelection();
    panel.LoadFullPath();
    if (!panel.IsArcFolder()) {
        const QString directory = panel.GetFolder()
                                      ? QDir::cleanPath(ToQString(GetFolderPath(panel.GetFolder())))
                                      : returnDirectory_;
        showDirectory(directory.isEmpty() ? returnDirectory_ : directory, false,
                      panel.GetFolder() && panel.IsFSFolder());
        if (selection && !scanningDisk_)
            panel.RefreshListCtrl(*selection);
        refreshActions();
        return;
    }
    auto *folder = dynamic_cast<CAgentFolder *>(panel.GetFolder());
    if (!folder)
        return;
    auto *resource = dynamic_cast<QtArchiveFileResource *>(folder->GetEngineResource());
    if (!resource)
        return;
    archive_ = resource->EnginePath;
    info_ = resource->Info;
    const auto &parents = panel.GetParentFolders();
    archivePassword_ = parents.IsEmpty() ? QString() : ToQString(parents.Back().Password);
    archiveFolder_ = QDir::fromNativeSeparators(ToQString(GetFolderPath(folder)));
    if (archiveFolder_.endsWith('/'))
        archiveFolder_.chop(1);
    displayArchive_ = QDir::fromNativeSeparators(ToQString(panel.GetCurrentFolderPrefix()));
    displayArchive_.chop(archiveFolder_.size() + (archiveFolder_.isEmpty() ? 1 : 2));
    entries_.clear();
    IInArchive *metadata = folder->GetArchiveProperties();
    UInt32 count = 0;
    if (metadata && metadata->GetNumberOfItems(&count) == S_OK) {
        for (UInt32 index = 0; index < count; ++index) {
            NWindows::NCOM::CPropVariant path, directory;
            metadata->GetProperty(index, kpidPath, &path);
            metadata->GetProperty(index, kpidIsDir, &directory);
            ArchiveEntry entry;
            if (path.vt == VT_BSTR)
                entry.path = QDir::fromNativeSeparators(QString::fromWCharArray(path.bstrVal));
            entry.directory = directory.vt == VT_BOOL && directory.boolVal != VARIANT_FALSE;
            entries_.append(entry);
        }
    }
    {
        QSignalBlocker block(flatView_);
        flatView_->setChecked(flatArchive_);
    }
    setState(State::Viewing);
    renderArchiveFolder();
    if (selection)
        panel.RefreshListCtrl(*selection);
    refreshActions();
}

void MainWindow::openSelectedInside() {
    if (!openInside_->isEnabled())
        return;
    const int row = table_->currentRow();
    if (state_ == State::Filesystem)
        navigateTo(table_->item(row, 1)->text());
    else if (table_->item(row, 0)->data(Qt::UserRole + 1).toBool())
        showArchiveFolder(table_->item(row, 0)->data(Qt::UserRole).toString());
    else
        openNestedArchive(table_->item(row, 0)->data(Qt::UserRole).toString());
}

void MainWindow::showArchiveFolder(const QString &folder) {
    auto &panel = app_.GetFocusedPanel();
    if (panel.GetParentFolders().IsEmpty())
        return;
    const UString path = panel.GetParentFolders().Back().VirtualPath + L"/" + ToUString(folder);
    {
        QScopedValueRollback<bool> source(sourceArchiveProgress_, true);
        COpenResult result;
        if (panel.BindToPath(path, UString(), result) != S_OK)
            return;
    }
    renderBoundFolder();
}

void MainWindow::openSelectedOutside() {
    if (!openOutside_->isEnabled())
        return;
    const int row = table_->currentRow();
    if (state_ == State::Filesystem)
        openLocalFile(table_->item(row, 1)->text());
    else
        openArchiveFile(table_->item(row, 0)->data(Qt::UserRole).toString());
}

void MainWindow::openNestedArchive(const QString &entry) {
    auto &panel = app_.GetFocusedPanel();
    UInt32 count = 0;
    if (!panel.GetFolder() || panel.GetFolder()->GetNumberOfItems(&count) != S_OK)
        return;
    for (UInt32 index = 0; index < count; ++index) {
        const QString path = QDir::fromNativeSeparators(
            ToQString(GetFolderPath(panel.GetFolder()) + panel.GetItemRelPath(index)));
        if (path != entry)
            continue;
        setState(State::Operating);
        {
            QScopedValueRollback<bool> source(sourceArchiveProgress_, true);
            panel.LoadFullPath();
            panel.OpenItemInArchive(index, true, false, false, false);
        }
        renderBoundFolder();
        return;
    }
}

void MainWindow::goBack() {
    if (state_ == State::Loading || state_ == State::Operating)
        return;
    if (state_ == State::Viewing)
        goParent();
    else if (!history_.isEmpty())
        showDirectory(history_.takeLast(), false);
}

void MainWindow::goUp() {
    if (state_ == State::Viewing && !archiveFolder_.isEmpty())
        goParent();
}

void MainWindow::goParent() {
    if (state_ != State::Viewing && state_ != State::Filesystem)
        return;
    auto &panel = app_.GetFocusedPanel();
    if (!panel.GetFolder())
        return;
    setState(State::Operating);
    {
        QScopedValueRollback<bool> source(sourceArchiveProgress_, true);
        panel.OpenParentFolder();
    }
    renderBoundFolder();
}

QString MainWindow::archiveDisplayPath() const {
    return displayArchive_ +
           (archiveFolder_.isEmpty() ? QString() : QStringLiteral(" / ") + archiveFolder_);
}
