#include "CPP/7zip/UI/FileManager/MessagesDialog.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "Common/StringConvert.h"
#include "archiveprogresstransport.h"
#include "mainwindow.h"
#include "qtcallbackprogresstransport.h"

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
    dialog->setWindowTitle("Информация об архиве");
    auto *layout = new QVBoxLayout(dialog);
    auto *label = new QLabel(dialog);
    label->setObjectName("archiveInfoText");
    label->setText(
        QStringLiteral(
            "Формат: %1\nРазмер архива: %2\nФайлов: %3\nПапок: %4\nРазмер файлов: %5 байт")
            .arg(info_.format.isEmpty() ? QStringLiteral("не указан 7zz") : info_.format)
            .arg(info_.physicalSize < 0
                     ? QStringLiteral("не указан 7zz")
                     : QString::number(info_.physicalSize) + QStringLiteral(" байт"))
            .arg(info_.files)
            .arg(info_.folders)
            .arg(info_.unpackedSize));
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
    const QString file =
        QFileDialog::getOpenFileName(this, "Открыть архив", {}, ArchiveFormats::openDialogFilter());
    if (!file.isEmpty())
        openArchive(file);
}

void MainWindow::openFileViewer(const QString &path, bool editable) {
    FileViewerDialog dialog(path, editable, this);
    dialog.exec();
    if (editable && state_ == State::Viewing && dialog.wasSaved() && viewTemporary_ &&
        !pendingEditEntry_.isEmpty()) {
        if (QMessageBox::question(this, QStringLiteral("Редактор"),
                                  QStringLiteral("Обновить изменённый файл в архиве?")) ==
            QMessageBox::Yes) {
            previousState_ = state_;
            updatingArchive_ = true;
            replacingArchiveEntry_ = true;
            setState(State::Operating);
            begin(QStringLiteral("Обновление файла в архиве"), pendingEditEntry_);
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
    QMessageBox::warning(this, QStringLiteral("Открыть файл"),
                         QStringLiteral("Не удалось открыть файл: %1").arg(path));
    return false;
}
