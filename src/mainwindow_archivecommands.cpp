#include "7zip/PropID.h"
#include "7zip/UI/FileManager/resource.h"
#include "7zip/UI/FileManager/resourceGui.h"
#include "7zip/UI/GUI/CompressDialogRes.h"
#include "7zip/UI/GUI/ExtractRes.h"
#include "CPP/7zip/UI/Common/HashCalc.h"
#include "CPP/7zip/UI/FileManager/MessagesDialog.h"
#include "CPP/7zip/UI/GUI/UpdateGUI.h"
#include "CPP/7zip/UI/GUI/resource2.h"
#include "Windows/PropVariant.h"
#include "archiveprogressoperation.h"
#include "mainwindow.h"
#include "qtarchivebatchextract.h"
#include "qtarchivefileresource.h"
#include "qtextractpathpolicy.h"
#include "qtlangcontrols.h"
#include <QScopeGuard>
#include <QScopedValueRollback>

void MainWindow::calculateHash(const QString &method) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if ((state_ != State::Filesystem && state_ != State::Viewing) ||
        sourceOperatedIndices().IsEmpty())
        return;
    previousState_ = state_;
    setState(State::Operating);
    HRESULT result;
    {
        // The immutable catalog must outlive its borrowed hash-provider interfaces.
        const auto catalog = backend_->codecCatalog();
        CExternalCodecs providers;
        QScopedValueRollback<bool> progressOwner(sourceArchiveProgress_, true);
        result = backend_->loadHashProviders(providers);
        if (result == S_OK)
            result = app_.CalculateCrc2(ToUString(method), &providers);
    }
    if (!app_.QtOperations().closed()) {
        setState(previousState_);
        if (result != S_OK && result != E_ABORT)
            app_.GetFocusedPanel().MessageBox_Error_HRESULT(result);
    }
}

void MainWindow::openArchiveFile(const QString &entry) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    viewTemporary_ = std::make_unique<QtOwnedTempDir>();
    if (!viewTemporary_->isValid()) {
        viewTemporary_.reset();
        QMessageBox::warning(this, QtLocalText("Open file", QStringLiteral("Открыть файл")),
                             QtLocalText("Could not create temporary folder.",
                                         QStringLiteral("Не удалось создать временную папку.")));
        return;
    }
    pendingViewPath_ = QDir(viewTemporary_->path()).filePath(entry);
    pendingView_ = true;
    pendingExternalOpen_ = true;
    previousState_ = state_;
    setState(State::Operating);
    begin(QtLangText(IDS_OPENNING), entry);
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
        QMessageBox::information(
            this, QtLocalText("Viewer", QStringLiteral("Просмотр")),
            QtLocalText("File is larger than 16 MiB. Text preview is unavailable.",
                        QStringLiteral("Файл больше 16 МиБ. Просмотр текста недоступен.")));
        return;
    }
    viewTemporary_ = std::make_unique<QtOwnedTempDir>();
    if (!viewTemporary_->isValid()) {
        viewTemporary_.reset();
        QMessageBox::warning(this, QtLocalText("Viewer", QStringLiteral("Просмотр")),
                             QtLocalText("Could not create temporary folder.",
                                         QStringLiteral("Не удалось создать временную папку.")));
        return;
    }
    pendingViewPath_ = QDir(viewTemporary_->path()).filePath(entry);
    pendingView_ = true;
    pendingExternalOpen_ = false;
    pendingViewEditable_ = editable;
    pendingEditEntry_ = editable ? entry : QString();
    previousState_ = state_;
    setState(State::Operating);
    begin(editable ? QtLocalText("Editing file", QStringLiteral("Редактирование файла"))
                   : QtLocalText("Viewing file", QStringLiteral("Просмотр файла")),
          entry);
    backend_->extract(archive_, viewTemporary_->path(), {entry}, archivePassword_);
}

void MainWindow::extract(bool onlySelected) {
    if (state_ != State::Viewing || entries_.isEmpty())
        return;
    if (onlySelected && sourceOperatedIndices().IsEmpty())
        return;
    ExtractDialog dialog(copyDefaultDestination(), {}, onlySelected || !archiveFolder_.isEmpty(),
                         this);
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
    CreateArchiveDialog dialog(directory_, selectedPaths, this, {}, backend_->codecCatalog());
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
    begin(QtLangText(IDS_PROGRESS_COMPRESSING), target);
    backend_->create(target, sources, password, encryptHeaders, options);
}

void MainWindow::addDialog() {
    if (state_ != State::Viewing)
        return;
    CreateArchiveDialog dialog(returnDirectory_, {}, this, archive_, backend_->codecCatalog(),
                               info_.format);
    if (dialog.exec() != QDialog::Accepted)
        return;
    if (state_ != State::Viewing || dialog.sources().isEmpty())
        return;
    updateArchiveFromDialog(dialog.sources(), dialog.compressionOptions());
}

void MainWindow::updateArchiveFromDialog(const QStringList &sources,
                                         const CompressionOptions &options) {
    if (app_.QtOperations().closed() || state_ != State::Viewing || sources.isEmpty() ||
        app_.GetFocusedPanel().IsThereReadOnlyFolder())
        return;
    CPanel &panel = app_.GetFocusedPanel();
    auto *folder = dynamic_cast<CAgentFolder *>(panel.GetFolder());
    auto *resource =
        folder ? dynamic_cast<QtArchiveFileResource *>(folder->GetEngineResource()) : nullptr;
    const auto *actionSet = NCompressDialog::FindUpdateActionSet(options.UpdateMode);
    if (!resource || !resource->HasUpdateService() || !actionSet) {
        QMessageBox::warning(
            this, QtLangText(IDD_COMPRESS),
            QtLocalText("Source update is unavailable for this archive.",
                        QStringLiteral("Исходное обновление этого архива недоступно.")));
        return;
    }
    QString root, error;
    QStringList relativePaths;
    if (!ArchiveBackend::prepareSourceSelection(archive_, sources, &root, &relativePaths, &error,
                                                options.SymLinks.Val)) {
        QMessageBox::warning(this, QtLangText(IDD_COMPRESS), error);
        return;
    }
    UStringVector names;
    for (const QString &path : sources)
        names.Add(ToUString(QFileInfo(path).absoluteFilePath()));
    CSelectedState selection;
    panel.SaveSelectedState(selection);
    CMyComPtr<IUnknown> keepResourceAlive = resource;
    resource->SetCompressionOptions(options);
    const auto resetOptions = qScopeGuard([resource] { resource->ClearCompressionOptions(); });
    UStringVector messages;
    HRESULT result;
    {
        QScopedValueRollback<bool> progressOwner(sourceArchiveProgress_, true);
        result =
            panel.CopyFrom(false, UString(), names, true, &messages, actionSet, options.PathMode);
    }
    if (result == S_OK && messages.IsEmpty()) {
        refreshCurrent(&selection);
        statusBar()->showMessage(
            QtLocalText("Archive updated.", QStringLiteral("Архив обновлён.")));
    } else if (result != E_ABORT) {
        QMessageBox::warning(this, QtLangText(IDD_COMPRESS),
                             messages.IsEmpty()
                                 ? QString::fromWCharArray(HResultToMessage(result).Ptr())
                                 : ToQString(messages[0]));
    }
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
    begin(QtLangText(IDS_PROGRESS_EXTRACTING), destination);
    backend_->extract(archive_, destination, names,
                      password.isEmpty() ? archivePassword_ : password, options);
}

bool MainWindow::extractWithSource(const QString &destination, bool onlySelected,
                                   const QString &password, const ExtractionOptions &options,
                                   const QStringList &names,
                                   const CRecordVector<UInt32> *explicitIndices) {
    CPanel &panel = app_.GetFocusedPanel();
    auto *folder =
        state_ == State::Viewing ? dynamic_cast<CAgentFolder *>(panel.GetFolder()) : nullptr;
    auto *resource =
        folder ? dynamic_cast<QtArchiveFileResource *>(folder->GetEngineResource()) : nullptr;
    if (options.pathMode == ExtractionOptions::PathMode::CurrentPaths &&
        (!resource || !resource->HasExtractService() ||
         !backend_->hasListedSourceArchive(archive_))) {
        if (options.currentFolder.isEmpty())
            return false; // At archive root, the existing full-path transport is equivalent.
        QMessageBox::warning(
            this, QtLangText(IDS_COPYING),
            QtLocalText("Source extraction is unavailable for this archive.",
                        QStringLiteral("Исходное извлечение для этого архива недоступно.")));
        return true;
    }
    const bool nativeRar = resource && resource->HasNativeRarReader();
    if (nativeRar && !resource->HasNativeExtractService()) {
        QMessageBox::warning(
            this, QtLangText(IDS_PROGRESS_EXTRACTING),
            QtLocalText("Source extraction is unavailable for this archive.",
                        QStringLiteral("Исходное извлечение для этого архива недоступно.")));
        return true;
    }
    if (!resource || !resource->HasExtractService() || !backend_->hasListedSourceArchive(archive_))
        return false;
    const bool scoped = onlySelected || !archiveFolder_.isEmpty();
    CRecordVector<UInt32> indices;
    QStringList validationNames = names;
    if (scoped) {
        if (onlySelected)
            indices = explicitIndices ? *explicitIndices : sourceOperatedIndices();
        else {
            UInt32 count = 0;
            if (folder->GetNumberOfItems(&count) != S_OK)
                return true;
            for (UInt32 index = 0; index < count; ++index)
                indices.Add(index);
        }
        if (indices.IsEmpty())
            return true;
        validationNames = archivePathsForIndices(indices);
        if (validationNames.isEmpty())
            return true;
    }
    {
        QString validationError;
        const bool sourceLinks = resource->HasNativeExtractService();
        if (!backend_->validateExtraction(destination, validationNames, options, &validationError,
                                          sourceLinks)) {
            QMessageBox::warning(this, QtLangText(IDS_PROGRESS_EXTRACTING), validationError);
            return true;
        }
        const QString output = QFileInfo(destination).absoluteFilePath();
        if (!QDir().mkpath(output) ||
            !backend_->validateExtraction(output, validationNames, options, &validationError,
                                          sourceLinks)) {
            QMessageBox::warning(
                this, QtLangText(IDS_PROGRESS_EXTRACTING),
                validationError.isEmpty()
                    ? QtLocalText("Could not create destination folder: %1",
                                  QStringLiteral("Не удалось создать папку назначения: %1"))
                          .arg(destination)
                    : validationError);
            return true;
        }
        if (!onlySelected && archiveFolder_.isEmpty()) {
            previousState_ = state_;
            setState(State::Operating);
            operationName_ = QtLangText(IDS_PROGRESS_EXTRACTING);
            appendJournal(operationName_ + QStringLiteral(": ") + output);
            journalAction_->setEnabled(true);
            QtArchiveBatchResult result;
            {
                QScopedValueRollback<bool> progressOwner(sourceArchiveProgress_, true);
                ExtractionOptions sourceOptions = options;
                // Validation above implements RejectExisting. If a file appears
                // afterwards, source Skip also preserves it without overwriting.
                if (sourceOptions.overwriteMode == ExtractionOptions::OverwriteMode::RejectExisting)
                    sourceOptions.overwriteMode = ExtractionOptions::OverwriteMode::SkipExisting;
                result = RunQtArchiveBatchExtract(*backend_, {archive_}, output,
                                                  password.isEmpty() ? archivePassword_ : password,
                                                  sourceOptions, this);
            }
            const bool ok = result.Result == S_OK && result.CallbackOK;
            const QString message =
                ok ? QtLocalText("Extraction completed.", QStringLiteral("Извлечение завершено."))
                : result.Result == E_ABORT
                    ? QtLocalText("Operation canceled.", QStringLiteral("Операция отменена."))
                : result.Result == S_OK
                    ? QtLocalText("Extraction completed with errors.",
                                  QStringLiteral("Извлечение завершено с ошибками."))
                    : QString::fromWCharArray(HResultToMessage(result.Result));
            backendFinished(ok, message, result.Result, result.MessageWasDisplayed);
            return true;
        }
        previousState_ = state_;
        setState(State::Operating);
        operationName_ = QtLangText(IDS_COPYING); // PanelCopy::CopyTo title owner
        appendJournal(operationName_ + QStringLiteral(": ") + output);
        journalAction_->setEnabled(true);
        CCopyToOptions sourceOptions;
        CQtExtractPathPolicy pathPolicy(output);
        CExtractNtOptions ntOptions;
        ntOptions.NtSecurity = options.NtSecurity;
        ntOptions.QtPathPolicy = &pathPolicy;
        QScopedValueRollback<const CExtractNtOptions *> extractOptions(folder->QtExtractNtOptions,
                                                                       &ntOptions);
        sourceOptions.extractMode = options.pathMode != ExtractionOptions::PathMode::CurrentPaths;
        sourceOptions.folder = ToUString(QDir::toNativeSeparators(output));
        sourceOptions.pathMode = options.pathMode == ExtractionOptions::PathMode::NoPaths
                                     ? NExtract::NPathMode::kNoPaths
                                 : options.pathMode == ExtractionOptions::PathMode::AbsolutePaths
                                     ? NExtract::NPathMode::kAbsPaths
                                     : NExtract::NPathMode::kFullPaths;
        sourceOptions.NeedRegistryZone = false;
        sourceOptions.showErrorMessages = true;
        {
            switch (options.overwriteMode) {
            case ExtractionOptions::OverwriteMode::Ask:
                break;
            case ExtractionOptions::OverwriteMode::OverwriteExisting:
                sourceOptions.overwriteMode = NExtract::NOverwriteMode::kOverwrite;
                break;
            case ExtractionOptions::OverwriteMode::SkipExisting:
            case ExtractionOptions::OverwriteMode::RejectExisting:
                sourceOptions.overwriteMode = NExtract::NOverwriteMode::kSkip;
                break;
            case ExtractionOptions::OverwriteMode::RenameExtracted:
                sourceOptions.overwriteMode = NExtract::NOverwriteMode::kRename;
                break;
            case ExtractionOptions::OverwriteMode::RenameExisting:
                sourceOptions.overwriteMode = NExtract::NOverwriteMode::kRenameExisting;
                break;
            }
        }
        bool usePassword = !password.isEmpty() || !archivePassword_.isEmpty();
        UString sourcePassword = ToUString(password.isEmpty() ? archivePassword_ : password);
        UStringVector messages;
        HRESULT result;
        {
            QScopedValueRollback<bool> progressOwner(sourceArchiveProgress_, true);
            result = panel.CopyTo(sourceOptions, indices, &messages, usePassword, sourcePassword);
        }
        const bool ok = result == S_OK && messages.IsEmpty();
        QString message =
            ok ? QtLocalText("Extraction completed.", QStringLiteral("Извлечение завершено."))
            : result == E_ABORT
                ? QtLocalText("Operation canceled.", QStringLiteral("Операция отменена."))
                : QString::fromWCharArray(HResultToMessage(result));
        if (!messages.IsEmpty())
            message = ToQString(messages[0]);
        backendFinished(ok, message, result, !messages.IsEmpty());
        return true;
    }
}

void MainWindow::testArchive() {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (state_ != State::Viewing)
        return;
    auto &panel = app_.GetFocusedPanel();
    auto *folder = dynamic_cast<CAgentFolder *>(panel.GetFolder());
    auto *resource =
        folder ? dynamic_cast<QtArchiveFileResource *>(folder->GetEngineResource()) : nullptr;
    if (resource && resource->HasNativeRarReader() && !resource->HasNativeExtractService()) {
        QMessageBox::warning(
            this, QtLangText(IDS_PROGRESS_TESTING),
            QtLocalText("Source extraction is unavailable for this archive.",
                        QStringLiteral("Исходное извлечение для этого архива недоступно.")));
        return;
    }
    if (resource && resource->HasExtractService() && backend_->hasListedSourceArchive(archive_)) {
        previousState_ = state_;
        setState(State::Operating);
        operationName_ = QtLangText(IDS_PROGRESS_TESTING);
        {
            QScopedValueRollback<bool> progressOwner(sourceArchiveProgress_, true);
            panel.TestArchives();
        }
        setState(State::Viewing);
        return;
    }
    previousState_ = state_;
    setState(State::Operating);
    begin(QtLangText(IDS_PROGRESS_TESTING), archive_);
    backend_->test(archive_, archivePassword_);
}

void MainWindow::addSources(const QStringList &sources) {
    if (app_.QtOperations().closed())
        return;
    QtAppOperationState::CallScope call(app_.QtOperations());
    if (state_ != State::Viewing || app_.GetFocusedPanel().IsThereReadOnlyFolder() ||
        info_.encrypted || sources.isEmpty())
        return;
    previousState_ = state_;
    updatingArchive_ = true;
    setState(State::Operating);
    operationName_ = QtLangText(IDS_PROGRESS_COMPRESSING);
    journalMessages_.Clear();
    if (journalDialog_)
        journalDialog_->ResetMessages();
    appendJournal(operationName_ + ": " + archive_);
    journalAction_->setEnabled(true);
    bool success = false;
    bool messageDisplayed = false;
    QString message;
    HRESULT result;
    {
        QScopedValueRollback<bool> progressOwner(sourceArchiveProgress_, true);
        ArchiveProgressOperation operation(*backend_,
                                           [this, sources] { backend_->add(archive_, sources); });
        operation.MainWindow = this;
        result = operation.Create(ToUString(operationName_), this);
        success = result == S_OK && operation.succeeded();
        message = result == S_OK ? operation.resultMessage()
                                 : QString::fromWCharArray(HResultToMessage(result).Ptr());
        messageDisplayed = operation.MessagesDisplayed;
        if (result == S_OK)
            result = operation.Result;
    }
    // This update's messages/errors were displayed by the source dialog.
    // Existing state completion still owns refresh/selection restoration.
    backendFinished(success, message, result, messageDisplayed);
}
