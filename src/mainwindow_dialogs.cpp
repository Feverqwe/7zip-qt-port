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
    QVector<PropertyItem> items;
    for (const QModelIndex &index : table_->selectionModel()->selectedRows()) {
        const int row = index.row();
        const QTableWidgetItem *name = table_->item(row, 0);
        PropertyItem item;
        item.name = name->text();
        item.directory = name->data(Qt::UserRole + 1).toBool();
        item.path = state_ == State::Filesystem ? table_->item(row, 1)->text()
                                                : name->data(Qt::UserRole).toString();
        item.modified = table_->item(row, 4)->text();
        if (state_ == State::Filesystem) {
            const QFileInfo file(item.path);
            item.size = item.directory ? 0 : file.size();
        } else if (!item.directory) {
            item.size = table_->item(row, 2)->text().toLongLong();
            item.packedSize = table_->item(row, 3)->text().toLongLong();
        }
        items.append(item);
    }
    PropertiesDialog dialog(items, state_ == State::Viewing,
                            state_ == State::Filesystem ? directory_ : archiveDisplayPath(), this);
    dialog.exec();
}

void MainWindow::begin(const QString &label, const QString &detail) {
    operationName_ = label;
    currentDetail_ = detail;
    journalText_.clear();
    if (journalView_)
        journalView_->clear();
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
        QFileDialog::getOpenFileName(this, "Открыть архив", {}, "Архивы (*.7z *.zip)");
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
        showDirectory(directory_, false);
}

bool MainWindow::openLocalFile(const QString &path) {
    if (fileOpener_(QUrl::fromLocalFile(path)))
        return true;
    QMessageBox::warning(this, QStringLiteral("Открыть файл"),
                         QStringLiteral("Не удалось открыть файл: %1").arg(path));
    return false;
}
