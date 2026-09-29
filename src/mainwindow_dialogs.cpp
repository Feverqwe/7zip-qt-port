#include "mainwindow.h"

void MainWindow::appendJournal(const QString &line) {
    journalText_ += line + '\n';
    if (journalView_)
        journalView_->appendPlainText(line);
}

void MainWindow::showJournal() {
    if (!journalDialog_) {
        journalDialog_ = new QDialog(this);
        journalDialog_->setObjectName("operationJournalDialog");
        journalDialog_->setWindowTitle("Журнал последней операции");
        journalDialog_->resize(700, 440);
        auto *layout = new QVBoxLayout(journalDialog_);
        journalView_ = new QPlainTextEdit(journalDialog_);
        journalView_->setObjectName("operationJournalText");
        journalView_->setReadOnly(true);
        journalView_->setPlainText(journalText_);
        layout->addWidget(journalView_);
    }
    journalDialog_->show();
    journalDialog_->raise();
    journalDialog_->activateWindow();
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
    panel.SetCurrentFolderPrefix(state_ == State::Filesystem ? directory_ : archiveDisplayPath());
    PropertiesDialog dialog(panel, indices, this);
    dialog.exec();
}

void MainWindow::begin(const QString &label, const QString &detail, bool preserveJournal) {
    operationName_ = label;
    currentDetail_ = detail;
    if (!preserveJournal) {
        journalText_.clear();
        if (journalView_)
            journalView_->clear();
    }
    appendJournal(label + ": " + detail);
    journalAction_->setEnabled(true);
    progress_ = new QProgressDialog(label + "\n" + detail, "Отмена", 0, 100, this);
    progress_->setWindowModality(Qt::NonModal);
    progress_->setMinimumDuration(0);
    progress_->setValue(0);
    connect(progress_, &QProgressDialog::canceled, backend_, &ArchiveBackend::cancel);
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
            app_.GetFocusedPanel().SaveSelectedState(refreshSelectedState_);
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
