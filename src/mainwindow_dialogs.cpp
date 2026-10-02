#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/BrowseDialog.h"
#include "CPP/7zip/UI/FileManager/FormatUtils.h"
#include "CPP/7zip/UI/FileManager/MessagesDialog.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "Common/StringConvert.h"
#include "archiveprogresstransport.h"
#include "mainwindow.h"
#include "qtcallbackprogresstransport.h"
#include "qtlangcontrols.h"

void MainWindow::appendJournal(const QString &line) {
    const UString message = ToUString(line);
    journalMessages_.Add(message);
    if (journalDialog_)
        journalDialog_->AppendMessage(message);
}

void MainWindow::showJournal() {
    if (!journalDialog_) {
        journalDialog_ = new CMessagesDialog(this);
        journalDialog_->Messages = &journalMessages_;
    }
    journalDialog_->Create(this);
}

void MainWindow::showInfo() {
    if (state_ != State::Viewing)
        return;
    if (infoDialog_)
        infoDialog_->close();
    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    infoDialog_ = dialog;
    dialog->setObjectName("archiveInfoDialog");
    QtBindLocalText(dialog, "Archive information", QStringLiteral("Информация об архиве"));
    auto *layout = new QVBoxLayout(dialog);
    auto *label = new QLabel(dialog);
    label->setObjectName("archiveInfoText");
    auto summary = [this](bool russian) {
        const QString unavailable =
            russian ? QStringLiteral("не указан 7zz") : QString("not reported by 7zz");
        const QString bytes = russian ? QStringLiteral(" байт") : QString(" bytes");
        return (russian ? QStringLiteral("Формат: %1\nРазмер архива: %2\nФайлов: %3\nПапок: "
                                         "%4\nРазмер файлов: %5 байт")
                        : QString("Format: %1\nArchive size: %2\nFiles: %3\nFolders: %4\nUnpacked "
                                  "size: %5 bytes"))
            .arg(info_.format.isEmpty() ? unavailable : info_.format)
            .arg(info_.physicalSize < 0 ? unavailable : QString::number(info_.physicalSize) + bytes)
            .arg(info_.files)
            .arg(info_.folders)
            .arg(info_.unpackedSize);
    };
    QtBindLocalText(label, summary(false), summary(true));
    layout->addWidget(label);
    dialog->show();
}

void MainWindow::showSelectedProperties() {
    if (!properties_->isEnabled())
        return;
    CPanel &panel = app_.GetFocusedPanel();
    CSelectedState selection;
    // Synchronize the Qt selection until the full source OnNotify port is connected.
    panel.SaveSelectedState(selection);
    CRecordVector<UInt32> indices;
    panel.Get_ItemIndices_Operated(indices);
    if (indices.IsEmpty())
        return;
    panel.LoadFullPath();
    PropertiesDialog dialog(panel, indices, this);
    dialog.exec();
}

void MainWindow::begin(const QString &label, const QString &detail, bool preserveJournal) {
    operationName_ = label;
    currentDetail_ = detail;
    if (!preserveJournal) {
        journalMessages_.Clear();
        if (journalDialog_)
            journalDialog_->ResetMessages();
    }
    appendJournal(label + ": " + detail);
    journalAction_->setEnabled(true);
    startProgressDialog(label, detail);
    progressTransport_ = std::make_unique<ArchiveProgressTransport>(*progress_, *backend_);
    progress_->OnInit();
    progress_->show();
}

void MainWindow::beginWorkerProgress(const QString &label, const QString &detail,
                                     std::function<void()> cancel) {
    startProgressDialog(label, detail);
    progress_->CompressingMode = false;
    progress_->ShowCompressionInfo = false;
    progressTransport_ = std::make_unique<QtCallbackProgressTransport>(std::move(cancel));
    progress_->AttachTransport(progressTransport_.get());
    progress_->OnInit();
    progress_->show();
}

void MainWindow::startProgressDialog(const QString &label, const QString &detail) {
    closeProgressDialog();
    auto *dialog = new CProgressDialog(this, ToUString(label));
    dialog->MainWindow = this;
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->Sync.Set_Status(ToUString(label));
    dialog->Sync.Set_FilePath(detail.toStdWString().c_str());
    progress_ = dialog;
}

void MainWindow::openDialog() {
    CBrowseInfo browse;
    browse.Owner = this;
    browse.Title = QtLocalText("Open archive", QStringLiteral("Открыть архив"));
    browse.Filters = backend_->openDialogFilter();
    if (browse.BrowseForFile())
        openArchive(browse.FilePath);
}

void MainWindow::openFileViewer(const QString &path, bool editable) {
    FileViewerDialog dialog(path, editable, this);
    dialog.exec();
    if (editable && state_ == State::Viewing && dialog.wasSaved() && viewTemporary_ &&
        !pendingEditEntry_.isEmpty()) {
        const UString message =
            MyFormatNew(IDS_WANT_UPDATE_MODIFIED_FILE, ToUString(pendingEditEntry_));
        if (QMessageBox::question(this, QStringLiteral("7-Zip"),
                                  QString::fromWCharArray(message)) == QMessageBox::Yes) {
            previousState_ = state_;
            updatingArchive_ = true;
            replacingArchiveEntry_ = true;
            setState(State::Operating);
            begin(QtLocalText("Updating file in archive",
                              QStringLiteral("Обновление файла в архиве")),
                  pendingEditEntry_);
            backend_->replaceEntry(archive_, pendingEditEntry_, viewTemporary_->path());
            return;
        }
    }
    pendingEditEntry_.clear();
    viewTemporary_.reset();
    if (editable && state_ == State::Filesystem)
        refreshCurrent();
}

bool MainWindow::openLocalFile(const QString &path) {
    if (fileOpener_(QUrl::fromLocalFile(path)))
        return true;
    QMessageBox::warning(
        this, QtLocalText("Open file", QStringLiteral("Открыть файл")),
        QtLocalText("Could not open file: %1", QStringLiteral("Не удалось открыть файл: %1"))
            .arg(path));
    return false;
}
