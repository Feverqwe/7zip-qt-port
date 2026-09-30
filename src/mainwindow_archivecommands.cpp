#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/MessagesDialog.h"
#include "Windows/PropVariant.h"
#include "archiveprogressoperation.h"
#include "mainwindow.h"
#include "qtarchivefileresource.h"
#include <QScopedValueRollback>

void MainWindow::calculateHash(const QString &method) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if ((state_ != State::Filesystem && state_ != State::Viewing) ||
        sourceOperatedIndices().IsEmpty())
        return;
    hashMethod_ = method;
    previousState_ = state_;
    if (state_ == State::Filesystem) {
        const QStringList paths = selectedFilesystemPaths();
        if (paths.isEmpty())
            return;
        hashing_ = true;
        setState(State::Operating);
        begin(QStringLiteral("Расчёт контрольных сумм"), method);
        backend_->hash(paths, method);
        return;
    }
    hashSelection_ = selectedArchiveItemsPaths();
    const QStringList entries = selectedArchivePaths();
    if (entries.isEmpty())
        return;
    hashTemporary_ = std::make_unique<QTemporaryDir>();
    if (!hashTemporary_->isValid()) {
        hashTemporary_.reset();
        QMessageBox::warning(this, QStringLiteral("Контрольные суммы"),
                             QStringLiteral("Не удалось создать временную папку."));
        return;
    }
    pendingHashExtraction_ = true;
    setState(State::Operating);
    begin(QStringLiteral("Извлечение для расчёта контрольных сумм"), archive_);
    backend_->extract(archive_, hashTemporary_->path(), entries, archivePassword_);
}

void MainWindow::openArchiveFile(const QString &entry) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    viewTemporary_ = std::make_unique<QTemporaryDir>();
    if (!viewTemporary_->isValid()) {
        viewTemporary_.reset();
        QMessageBox::warning(this, QStringLiteral("Открыть файл"),
                             QStringLiteral("Не удалось создать временную папку."));
        return;
    }
    pendingViewPath_ = QDir(viewTemporary_->path()).filePath(entry);
    pendingView_ = true;
    pendingExternalOpen_ = true;
    previousState_ = state_;
    setState(State::Operating);
    begin(QStringLiteral("Открытие файла"), entry);
    backend_->extract(archive_, viewTemporary_->path(), {entry}, archivePassword_);
}

void MainWindow::viewSelectedFile(bool editable) {
    if (state_ != State::Filesystem && state_ != State::Viewing)
        return;
    const auto indices = sourceOperatedIndices();
    if (indices.Size() != 1)
        return;
    if (app_.GetFocusedPanel().IsItem_Folder(indices[0]))
        return;
    if (state_ == State::Filesystem) {
        const QStringList paths = selectedFilesystemPaths();
        if (paths.size() != 1)
            return;
        const QString path = paths.first();
        if (QFileInfo(path).isFile() && !QFileInfo(path).isSymLink())
            openFileViewer(path, editable);
        return;
    }
    if (editable && (info_.encrypted || isNestedArchive()))
        return;
    const QStringList paths = selectedArchiveItemsPaths();
    if (paths.size() != 1)
        return;
    const QString entry = paths.first();
    NWindows::NCOM::CPropVariant size;
    app_.GetFocusedPanel().GetFolder()->GetProperty(indices[0], kpidSize, &size);
    if (size.vt == VT_UI8 && size.uhVal.QuadPart > 16 * 1024 * 1024) {
        QMessageBox::information(this, "Просмотр",
                                 "Файл больше 16 МиБ. Просмотр текста недоступен.");
        return;
    }
    viewTemporary_ = std::make_unique<QTemporaryDir>();
    if (!viewTemporary_->isValid()) {
        viewTemporary_.reset();
        QMessageBox::warning(this, "Просмотр", "Не удалось создать временную папку.");
        return;
    }
    pendingViewPath_ = QDir(viewTemporary_->path()).filePath(entry);
    pendingView_ = true;
    pendingExternalOpen_ = false;
    pendingViewEditable_ = editable;
    pendingEditEntry_ = editable ? entry : QString();
    previousState_ = state_;
    setState(State::Operating);
    begin(editable ? "Редактирование файла" : "Просмотр файла", entry);
    backend_->extract(archive_, viewTemporary_->path(), {entry}, archivePassword_);
}

void MainWindow::extract(bool onlySelected) {
    if (state_ != State::Viewing || entries_.isEmpty())
        return;
    if (onlySelected && sourceOperatedIndices().IsEmpty())
        return;
    ExtractDialog dialog(copyDefaultDestination(), {}, onlySelected, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const QString destination = dialog.destination();
    if (destination.isEmpty())
        return;
    extractTo(destination, onlySelected, dialog.password(), dialog.options());
}

void MainWindow::createDialog() {
    if (state_ != State::Filesystem)
        return;
    const QStringList selectedPaths = selectedFilesystemPaths();
    CreateArchiveDialog dialog(directory_, selectedPaths, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    createArchive(dialog.target(), dialog.sources(), dialog.password(), dialog.encryptHeaders(),
                  dialog.compressionOptions());
}

void MainWindow::createArchive(const QString &target, const QStringList &sources,
                               const QString &password, bool encryptHeaders,
                               const CompressionOptions &options) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (state_ != State::Filesystem || sources.isEmpty() || target.isEmpty())
        return;
    previousState_ = state_;
    creating_ = true;
    setState(State::Operating);
    begin("Создание архива", target);
    backend_->create(target, sources, password, encryptHeaders, options);
}

void MainWindow::addDialog() {
    if (state_ != State::Viewing)
        return;
    CreateArchiveDialog dialog(returnDirectory_, {}, this, archive_);
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (state_ != State::Viewing || dialog.sources().isEmpty())
        return;
    addSources(dialog.sources());
}

void MainWindow::extractTo(const QString &destination, bool onlySelected, const QString &password,
                           const ExtractionOptions &options) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (state_ != State::Viewing || entries_.isEmpty() || destination.isEmpty())
        return;
    QStringList names;
    if (onlySelected) {
        names = selectedArchivePaths();
        if (names.isEmpty())
            return;
    }
    if (extractWithSource(destination, onlySelected, password, options, names))
        return;
    previousState_ = state_;
    setState(State::Operating);
    begin("Извлечение", destination);
    backend_->extract(archive_, destination, names,
                      password.isEmpty() ? archivePassword_ : password, options);
}

bool MainWindow::extractWithSource(const QString &destination, bool onlySelected,
                                   const QString &password, const ExtractionOptions &options,
                                   const QStringList &names) {
    CPanel &panel = app_.GetFocusedPanel();
    auto *folder =
        state_ == State::Viewing ? dynamic_cast<CAgentFolder *>(panel.GetFolder()) : nullptr;
    auto *resource =
        folder ? dynamic_cast<QtArchiveFileResource *>(folder->GetEngineResource()) : nullptr;
    if (options.overwriteMode != ExtractionOptions::OverwriteMode::Ask ||
        !archiveFolder_.isEmpty() || isNestedArchive() || info_.encrypted || !resource ||
        !resource->HasExtractService() || !backend_->hasListedSourceArchive(archive_))
        return false;
    {
        QString validationError;
        if (!backend_->validateExtraction(destination, names, options, &validationError)) {
            QMessageBox::warning(this, QStringLiteral("Извлечение"), validationError);
            return true;
        }
        const QString output = QFileInfo(destination).absoluteFilePath();
        if (!QDir().mkpath(output) ||
            !backend_->validateExtraction(output, names, options, &validationError)) {
            QMessageBox::warning(
                this, QStringLiteral("Извлечение"),
                validationError.isEmpty()
                    ? QStringLiteral("Не удалось создать папку назначения: %1").arg(destination)
                    : validationError);
            return true;
        }
        CRecordVector<UInt32> indices;
        if (onlySelected)
            indices = sourceOperatedIndices();
        else {
            UInt32 count = 0;
            if (folder->GetNumberOfItems(&count) != S_OK)
                return true;
            for (UInt32 index = 0; index < count; ++index)
                indices.Add(index);
        }
        if (indices.IsEmpty())
            return true;
        previousState_ = state_;
        setState(State::Operating);
        operationName_ = QStringLiteral("Извлечение");
        appendJournal(operationName_ + QStringLiteral(": ") + output);
        journalAction_->setEnabled(true);
        CCopyToOptions sourceOptions;
        sourceOptions.extractMode = true;
        sourceOptions.folder = ToUString(output);
        sourceOptions.pathMode = options.pathMode == ExtractionOptions::PathMode::NoPaths
                                     ? NExtract::NPathMode::kNoPaths
                                     : NExtract::NPathMode::kFullPaths;
        sourceOptions.NeedRegistryZone = false;
        sourceOptions.showErrorMessages = true;
        bool usePassword = !password.isEmpty() || !archivePassword_.isEmpty();
        UString sourcePassword = ToUString(password.isEmpty() ? archivePassword_ : password);
        UStringVector messages;
        HRESULT result;
        {
            QScopedValueRollback<bool> progressOwner(sourceArchiveProgress_, true);
            result = panel.CopyTo(sourceOptions, indices, &messages, usePassword, sourcePassword);
        }
        const bool ok = result == S_OK && messages.IsEmpty();
        QString message = ok                  ? QStringLiteral("Извлечение завершено.")
                          : result == E_ABORT ? QStringLiteral("Операция отменена.")
                                              : QString::fromWCharArray(HResultToMessage(result));
        if (!messages.IsEmpty())
            message = ToQString(messages[0]);
        backendFinished(ok, message, !messages.IsEmpty());
        return true;
    }
}

void MainWindow::testArchive() {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (state_ != State::Viewing)
        return;
    previousState_ = state_;
    setState(State::Operating);
    begin("Проверка архива", archive_);
    backend_->test(archive_, archivePassword_);
}

void MainWindow::addSources(const QStringList &sources) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (state_ != State::Viewing || !ArchiveFormats::canModify(archive_) || info_.encrypted ||
        sources.isEmpty())
        return;
    previousState_ = state_;
    updatingArchive_ = true;
    setState(State::Operating);
    operationName_ = QStringLiteral("Добавление в архив");
    journalMessages_.Clear();
    if (journalDialog_)
        journalDialog_->ResetMessages();
    appendJournal(operationName_ + ": " + archive_);
    journalAction_->setEnabled(true);
    bool success = false;
    bool messageDisplayed = false;
    QString message;
    {
        QScopedValueRollback<bool> progressOwner(sourceArchiveProgress_, true);
        ArchiveProgressOperation operation(*backend_,
                                           [this, sources] { backend_->add(archive_, sources); });
        operation.MainWindow = this;
        const HRESULT result = operation.Create(ToUString(operationName_), this);
        success = result == S_OK && operation.succeeded();
        message = result == S_OK ? operation.resultMessage()
                                 : QString::fromWCharArray(HResultToMessage(result).Ptr());
        messageDisplayed = operation.MessagesDisplayed;
    }
    // This update's messages/errors were displayed by the source dialog.
    // Existing state completion still owns refresh/selection restoration.
    backendFinished(success, message, messageDisplayed);
}
