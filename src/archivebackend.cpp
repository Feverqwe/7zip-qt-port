#include "archivebackend.h"
#include "CPP/7zip/UI/Common/IFileExtractCallback.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "archivecodeccatalog.h"
#include "archivecodecprobe.h"
#include "archiveenginelocation.h"
#include "archivesfxmodule.h"
#include "processpriority.h"
#include "qtarchivecreationlifecycle.h"
#include "qtarchivehashoperation.h"
#include "qtarchivevolumeoutput.h"
#include "qtlanguageservices.h"

#include "qtownedtempdir.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

ArchiveBackend::ArchiveBackend(QObject *parent, QString executable) : QObject(parent) {
    if (executable.isEmpty())
        executable = FindArchiveEngine();
    process_.setProgram(executable);
    configuredEngine_ = executable;
    process_.setProcessChannelMode(QProcess::MergedChannels);
    connect(&process_, &QProcess::started, this, [this] {
        processPause_.Attach(UInt32(process_.processId()));
        if (!cancelled_)
            emit engineStarted();
    });
    connect(&process_, &QProcess::readyRead, this, [this] { consumeOutput(process_.readAll()); });
    connect(&process_, &QProcess::readyReadStandardError, this, [this] {
        if (job_ == Job::SourceExtract)
            consumeOutput(process_.readAllStandardError());
    });
    connect(&process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            &ArchiveBackend::complete);
    connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart && job_ != Job::None) {
            processPause_.ResetAfterExit();
            if (job_ == Job::Create || job_ == Job::Add || job_ == Job::Delete ||
                job_ == Job::Rename || job_ == Job::ReplaceDelete || job_ == Job::ReplaceAdd ||
                job_ == Job::SourceUpdate || job_ == Job::SourcePlanDelete ||
                job_ == Job::SourcePlanFiles || job_ == Job::SourcePlanDirectories ||
                job_ == Job::SourcePlanRename || job_ == Job::SourceMutation ||
                job_ == Job::SourceExtract)
                QFile::remove(createTemporary_);
            job_ = Job::None;
            sourceExtractSelection_.reset();
            extractQueue_.clear();
            extractArguments_.clear();
            pendingPassword_.clear();
            createStages_.clear();
            createPassword_.clear();
            extractPassword_.clear();
            sourcePlanPassword_.clear();
            emit finished(
                false, QtLocalText("Could not start 7zz. Install official 7zz and add it to PATH.",
                                   QStringLiteral("Не удалось запустить 7zz. Установите "
                                                  "официальный 7zz и добавьте его в PATH.")));
        }
    });
}

ArchiveBackend::~ArchiveBackend() {
    codecProbe_.reset();
    // QProcess can emit readyRead/finished during destruction. Stop callbacks
    // while their buffers and operation state are still alive.
    disconnect(&process_, nullptr, this, nullptr);
    cancelAndWait();
    if (process_.state() == QProcess::NotRunning &&
        (job_ == Job::Create || job_ == Job::Add || job_ == Job::Delete || job_ == Job::Rename ||
         job_ == Job::ReplaceDelete || job_ == Job::ReplaceAdd || job_ == Job::SourceUpdate ||
         job_ == Job::SourcePlanDelete || job_ == Job::SourcePlanFiles ||
         job_ == Job::SourcePlanDirectories || job_ == Job::SourceMutation ||
         job_ == Job::SourcePlanRename || job_ == Job::SourceExtract))
        QFile::remove(createTemporary_);
}

bool ArchiveBackend::busy() const {
    return job_ != Job::None || overwritePromptActive_ || codecDiscoveryActive_;
}

HRESULT ArchiveBackend::lastResult() const { return lastResult_; }

HRESULT ArchiveBackend::setPaused(bool paused) {
    if (hashOperation_) {
        hashOperation_->setPaused(paused);
        return S_OK;
    }
    if (creationLifecycle_ && (job_ == Job::CreatePrepare || job_ == Job::CreateDeleteInputs)) {
        creationLifecycle_->setPaused(paused);
        return S_OK;
    }
    if (volumeOutput_) {
        volumeOutput_->setPaused(paused);
        return S_OK;
    }
    if (process_.state() != QProcess::Running)
        return E_FAIL;
    return processPause_.SetPaused(paused);
}

HRESULT ArchiveBackend::setBackground(bool background) {
    // The source progress dialog already sets this application's priority.
    // Native workers run in that process rather than a separate CLI child.
    if (hashOperation_ || volumeOutput_ || job_ == Job::CreatePrepare ||
        job_ == Job::CreateDeleteInputs)
        return S_OK;
    if (process_.state() != QProcess::Running)
        return E_FAIL;
    return NProcessPriority::SetBackground(UInt32(process_.processId()), background);
}

void ArchiveBackend::start(Job job, const QStringList &arguments, const QString &workingDirectory) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         QStringLiteral("Другая операция ещё выполняется.")));
        return;
    }
    job_ = job;
    lastResult_ = E_FAIL;
    output_.clear();
    listingOutput_.clear();
    pendingOutput_.clear();
    cancelled_ = false;
    passwordPrompted_ = false;
    process_.setWorkingDirectory(workingDirectory);
    process_.setProcessChannelMode(job == Job::SourceExtract ? QProcess::SeparateChannels
                                                             : QProcess::MergedChannels);
    process_.setStandardOutputFile(job == Job::SourceExtract ? createTemporary_ : QString());
    QStringList processArguments = arguments;
    processArguments.insert(1, "-sccUTF-8");
    if (IsLargePageSupported() && ReadLockMemoryEnable(FmSettingsPath()))
        processArguments.insert(1, "-slp");
    process_.setArguments(processArguments);
    process_.start();
    if (!pendingPassword_.isEmpty())
        providePassword(pendingPassword_);
}

void ArchiveBackend::providePassword(const QString &password) {
    if (!busy())
        return;
    pendingPassword_ = password;
    if (job_ == Job::ExtractCheck)
        extractPassword_ = password;
    process_.write(password.toUtf8() + '\n');
}

void ArchiveBackend::provideOverwriteAnswer(int answer) {
    if (overwritePromptActive_)
        overwriteAnswer_ = answer;
}

void ArchiveBackend::consumeLine(const QByteArray &line) {
    QString text = QString::fromUtf8(line);
    text.remove(QLatin1Char('\b'));
    text = text.trimmed();
    if (!pendingPassword_.isEmpty())
        text.replace(pendingPassword_, QtLocalText("[hidden]", QStringLiteral("[скрыто]")));
    if (text.isEmpty())
        return;
    output_ += text + '\n';
    emit outputReceived(text);
    static const QRegularExpression percent(R"((?:^|\s)(\d{1,3})%)");
    auto match = percent.globalMatch(text);
    int value = -1;
    QString detail;
    while (match.hasNext()) {
        const auto found = match.next();
        value = found.captured(1).toInt();
        detail = text.mid(found.capturedEnd()).trimmed();
    }
    if (value >= 0 && value <= 100) {
        if (detail.startsWith('+') || detail.startsWith('-'))
            detail = detail.mid(1).trimmed();
        emit progress(value, detail);
    } else if (text.startsWith("+ ") || text.startsWith("- ")) {
        emit progress(-1, text.mid(2).trimmed());
    }
}

void ArchiveBackend::consumeOutput(const QByteArray &chunk, bool flush) {
    // Metadata must retain spaces, blank/multiline values and fragmented UTF-8.
    // The existing trimmed/redacted progress journal is a separate presentation.
    if (job_ == Job::List)
        listingOutput_ += chunk;
    pendingOutput_ += chunk;
    if (!passwordPrompted_ && pendingPassword_.isEmpty() &&
        (pendingOutput_.contains("Enter password:") ||
         pendingOutput_.contains("Enter password (will not be echoed):"))) {
        passwordPrompted_ = true;
        emit passwordRequired();
    }
    qsizetype start = 0;
    while (true) {
        const qsizetype cr = pendingOutput_.indexOf('\r', start);
        const qsizetype lf = pendingOutput_.indexOf('\n', start);
        const qsizetype end = cr < 0 ? lf : (lf < 0 ? cr : qMin(cr, lf));
        if (end < 0)
            break;
        if (end > start)
            consumeLine(pendingOutput_.mid(start, end - start));
        start = end + 1;
    }
    if (start > 0)
        pendingOutput_.remove(0, start);
    if (flush && !pendingOutput_.isEmpty()) {
        consumeLine(pendingOutput_);
        pendingOutput_.clear();
    }
}

void ArchiveBackend::list(const QString &archive, const QString &password) {
    startListing(archive, password, {});
}

void ArchiveBackend::openFolder(const QString &archive, const QString &format,
                                const QString &password) {
    if (!busy() && !format.isEmpty()) {
        if (const auto catalog = codecCatalog(); catalog && catalog->result() == S_OK) {
            CObjectVector<COpenType> types;
            if (!catalog->parseOpenTypes(format, types)) {
                lastResult_ = E_INVALIDARG;
                emit finished(false, QtLocalText("Invalid archive open type: %1",
                                                 "Неверный режим открытия архива: %1")
                                         .arg(format));
                return;
            }
        }
    }
    if (format.isEmpty())
        list(archive, password);
    else
        startListing(archive, password, format);
}

void ArchiveBackend::startListing(const QString &archive, const QString &password,
                                  const QString &format) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         QStringLiteral("Другая операция ещё выполняется.")));
        return;
    }
    archive_ = QFileInfo(archive).absoluteFilePath();
    listedSourceArchives_.remove(archive_);
    entries_.clear();
    listed_ = false;
    encrypted_ = false;
    pendingPassword_ = password;
    QStringList arguments{"l", "-slt", "-sns"};
    if (!format.isEmpty())
        arguments << QStringLiteral("-t") + format;
    arguments << "--" << archive_;
    start(Job::List, arguments);
}

bool ArchiveBackend::prepareAskExtraction(const QString &outputDirectory,
                                          const QStringList &selected,
                                          ExtractionOptions::PathMode pathMode,
                                          QStringList *replaceEntries, QStringList *renameEntries) {
    enum class Mode { Ask, Skip, Overwrite, Rename };
    Mode mode = Mode::Ask;
    for (const ArchiveEntry &entry : entries_) {
        if (entry.directory || (!selected.isEmpty() && !selected.contains(entry.path)))
            continue;
        QString relative = entry.path;
        relative.replace('\\', '/');
        if (pathMode == ExtractionOptions::PathMode::NoPaths)
            relative = relative.section('/', -1);
        const QString target = QDir(outputDirectory).filePath(relative);
        if (!QFileInfo::exists(target))
            continue;
        if (mode == Mode::Ask) {
            overwritePromptActive_ = true;
            overwriteAnswer_ = -1;
            emit overwriteRequired(target, entry);
            overwritePromptActive_ = false;
            if (cancelled_ || overwriteAnswer_ == NOverwriteAnswer::kCancel) {
                cancelled_ = false;
                lastResult_ = E_ABORT;
                emit finished(false, QtLocalText("Operation canceled.",
                                                 QStringLiteral("Операция отменена.")));
                return false;
            }
            switch (overwriteAnswer_) {
            case NOverwriteAnswer::kNo:
                continue;
            case NOverwriteAnswer::kNoToAll:
                mode = Mode::Skip;
                continue;
            case NOverwriteAnswer::kYes:
                *replaceEntries << entry.path;
                continue;
            case NOverwriteAnswer::kYesToAll:
                mode = Mode::Overwrite;
                break;
            case NOverwriteAnswer::kAutoRename:
                mode = Mode::Rename;
                break;
            default:
                lastResult_ = E_FAIL;
                emit finished(false, QtLocalText("Overwrite decision is unavailable.",
                                                 QStringLiteral("Решение о замене недоступно.")));
                return false;
            }
        }
        if (mode == Mode::Overwrite)
            *replaceEntries << entry.path;
        else if (mode == Mode::Rename)
            *renameEntries << entry.path;
    }
    return true;
}

bool ArchiveBackend::hasListedSourceArchive(const QString &archive) const {
    if (busy())
        return false;
    const QFileInfo file(archive);
    const auto found = listedSourceArchives_.constFind(file.absoluteFilePath());
    return found != listedSourceArchives_.cend() && file.isFile() && !file.isSymLink() &&
           file.size() == found->Size && file.lastModified() == found->Modified &&
           file.metadataChangeTime() == found->MetadataChanged;
}

bool ArchiveBackend::restoreListedArchive(const QString &archive) {
    if (!hasListedSourceArchive(archive))
        return false;
    const QString path = QFileInfo(archive).absoluteFilePath();
    const ListedSourceSnapshot &snapshot = listedSourceArchives_.constFind(path).value();
    archive_ = path;
    entries_ = snapshot.Entries;
    encrypted_ = snapshot.Encrypted;
    listed_ = true;
    return true;
}

void ArchiveBackend::extract(const QString &archive, const QString &destination,
                             const QStringList &selected, const QString &password,
                             const ExtractionOptions &options) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         QStringLiteral("Другая операция ещё выполняется.")));
        return;
    }
    if (!restoreListedArchive(archive)) {
        emit finished(
            false,
            QtLocalText("Open the archive and inspect its contents first.",
                        QStringLiteral("Сначала откройте архив и проверьте его содержимое.")));
        return;
    }
    if (options.pathMode == ExtractionOptions::PathMode::AbsolutePaths || options.ElimDup.Val ||
        options.NtSecurity.Val) {
        emit finished(false, QtLocalText("These extraction options require the source workflow.",
                                         "Эти параметры извлечения требуют исходного сценария."));
        return;
    }
    QStringList selectedEntries;
    for (const QString &name : selected) {
        QString normalized = name;
        normalized.replace('\\', '/');
        bool found = false;
        for (const ArchiveEntry &entry : entries_) {
            QString entryPath = entry.path;
            entryPath.replace('\\', '/');
            if (entryPath == normalized) {
                selectedEntries << entry.path;
                found = true;
                break;
            }
        }
        if (!found) {
            emit finished(false,
                          QtLocalText("Selected file is missing from the archive: %1",
                                      QStringLiteral("Выбранный файл отсутствует в архиве: %1"))
                              .arg(name));
            return;
        }
    }
    QString validationError;
    if (!validateExtraction(destination, selectedEntries, options, &validationError)) {
        emit finished(false, validationError);
        return;
    }
    const QString outputDirectory = QFileInfo(destination).absoluteFilePath();
    if (!QDir().mkpath(outputDirectory) ||
        !validateExtraction(destination, selectedEntries, options, &validationError)) {
        emit finished(false,
                      validationError.isEmpty()
                          ? QtLocalText("Could not create destination folder: %1",
                                        QStringLiteral("Не удалось создать папку назначения: %1"))
                                .arg(destination)
                          : validationError);
        return;
    }
    extractQueue_.clear();
    QStringList replaceEntries;
    QStringList renameEntries;
    if (options.overwriteMode == ExtractionOptions::OverwriteMode::Ask) {
        if (!prepareAskExtraction(outputDirectory, selectedEntries, options.pathMode,
                                  &replaceEntries, &renameEntries))
            return;
        if (!validateExtraction(destination, selectedEntries, options, &validationError)) {
            emit finished(false, validationError);
            return;
        }
    }
    QStringList arguments = {options.pathMode == ExtractionOptions::PathMode::NoPaths ? "e" : "x",
                             "-y",
                             "-bsp1",
                             "-bb1",
                             "-spd",
                             "-o" + outputDirectory,
                             "--",
                             QFileInfo(archive).absoluteFilePath()};
    if (options.overwriteMode == ExtractionOptions::OverwriteMode::Ask ||
        options.overwriteMode == ExtractionOptions::OverwriteMode::SkipExisting)
        arguments.insert(2, "-aos");
    else if (options.overwriteMode == ExtractionOptions::OverwriteMode::OverwriteExisting)
        arguments.insert(2, "-aoa");
    else if (options.overwriteMode == ExtractionOptions::OverwriteMode::RenameExtracted)
        arguments.insert(2, "-aou");
    else if (options.overwriteMode == ExtractionOptions::OverwriteMode::RenameExisting)
        arguments.insert(2, "-aot");
    arguments += selectedEntries;
    if (options.overwriteMode == ExtractionOptions::OverwriteMode::Ask) {
        const auto addConflictPass = [&](const QString &mode, const QStringList &entries) {
            if (entries.isEmpty())
                return;
            QStringList pass = arguments;
            pass.replace(2, mode);
            pass.erase(pass.end() - selectedEntries.size(), pass.end());
            pass += entries;
            extractQueue_.append(pass);
        };
        addConflictPass("-aoa", replaceEntries);
        addConflictPass("-aou", renameEntries);
    }
    pendingPassword_ = password;
    if (encrypted_) {
        extractArguments_ = arguments;
        extractPassword_ = password;
        start(Job::ExtractCheck, {"t", "-bsp1", "-bb1", "--", archive_});
    } else
        start(Job::Extract, arguments);
}

void ArchiveBackend::test(const QString &archive, const QString &password) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         QStringLiteral("Другая операция ещё выполняется.")));
        return;
    }
    if (!restoreListedArchive(archive)) {
        emit finished(
            false,
            QtLocalText("Open the archive and inspect its contents first.",
                        QStringLiteral("Сначала откройте архив и проверьте его содержимое.")));
        return;
    }
    pendingPassword_ = password;
    start(Job::Test, {"t", "-bsp1", "-bb1", "--", archive_});
}

void ArchiveBackend::testSourceArchive(const QString &archive, const QString &password) {
    if (busy()) {
        emit finished(false, QtLocalText("Another operation is still running.",
                                         QStringLiteral("Другая операция ещё выполняется.")));
        return;
    }
    const QFileInfo file(archive);
    if (!file.isFile() || file.isSymLink()) {
        lastResult_ = E_INVALIDARG;
        emit finished(false, QtLocalText("Source archive is unavailable.",
                                         QStringLiteral("Исходный архив недоступен.")));
        return;
    }
    archive_ = file.absoluteFilePath();
    pendingPassword_ = password;
    start(Job::Test, {"t", "-bsp1", "-bb1", "--", archive_});
}

void ArchiveBackend::hash(const QStringList &paths, const QString &method) {
    hashMethods(paths, {method});
}

void ArchiveBackend::cancel() {
    if (!busy())
        return;
    cancelled_ = true;
    if (hashOperation_)
        hashOperation_->cancel();
    if (creationLifecycle_)
        creationLifecycle_->cancel();
    if (volumeOutput_)
        volumeOutput_->cancel();
    pendingPassword_.clear();
    if (overwritePromptActive_)
        emit dismissOverwriteRequested();
    extractQueue_.clear();
    if (process_.state() != QProcess::NotRunning)
        process_.kill();
}

void ArchiveBackend::cancelAndWait() {
    if (busy())
        cancel();
    if (hashOperation_) {
        hashOperation_->wait();
        finishHashOperation();
    }
    if (creationLifecycle_ && (job_ == Job::CreatePrepare || job_ == Job::CreateDeleteInputs)) {
        creationLifecycle_->wait();
        finishCreationLifecycle();
    }
    if (volumeOutput_) {
        if (!volumeOutput_->isRunning() && !volumeOutput_->isFinished()) {
            volumeOutput_->Result = E_ABORT;
            volumeOutput_->Error =
                QtLocalText("Operation canceled.", QStringLiteral("Операция отменена."));
        }
        volumeOutput_->wait();
        finishVolumeOutput();
    }
    if (process_.state() != QProcess::NotRunning) {
        process_.kill();
        process_.waitForFinished(-1);
    }
    processPause_.ResetAfterExit();
}

void ArchiveBackend::complete(int exitCode, QProcess::ExitStatus status) {
    processPause_.ResetAfterExit();
    if (job_ == Job::None)
        return;
    consumeOutput(process_.readAll(), true);
    if (job_ == Job::SourceExtract)
        consumeOutput(process_.readAllStandardError(), true);
    pendingPassword_.clear();
    const Job completed = job_;
    job_ = Job::None;
    if (completed == Job::SourceExtract)
        sourceExtractSelection_.reset();
    if (cancelled_) {
        lastResult_ = E_ABORT;
        extractPassword_.clear();
        createStages_.clear();
        createPassword_.clear();
        sourcePlanPassword_.clear();
        extractQueue_.clear();
        if (completed == Job::Create || completed == Job::Add || completed == Job::Delete ||
            completed == Job::Rename || completed == Job::ReplaceDelete ||
            completed == Job::ReplaceAdd || completed == Job::SourceUpdate ||
            completed == Job::SourcePlanDelete || completed == Job::SourcePlanFiles ||
            completed == Job::SourcePlanDirectories || completed == Job::SourceMutation ||
            completed == Job::SourcePlanRename || completed == Job::SourceExtract)
            QFile::remove(createTemporary_);
        emit finished(false,
                      QtLocalText("Operation canceled.", QStringLiteral("Операция отменена.")));
        return;
    }
    if (status != QProcess::NormalExit || exitCode != 0) {
        if (completed == Job::List)
            lastResult_ = listingFailureResult(exitCode, status);
        extractPassword_.clear();
        createStages_.clear();
        createPassword_.clear();
        sourcePlanPassword_.clear();
        extractQueue_.clear();
        if (completed == Job::Create || completed == Job::Add || completed == Job::Delete ||
            completed == Job::Rename || completed == Job::ReplaceDelete ||
            completed == Job::ReplaceAdd || completed == Job::SourceUpdate ||
            completed == Job::SourcePlanDelete || completed == Job::SourcePlanFiles ||
            completed == Job::SourcePlanDirectories || completed == Job::SourceMutation ||
            completed == Job::SourcePlanRename || completed == Job::SourceExtract)
            QFile::remove(createTemporary_);
        const bool wrongPassword =
            output_.contains(QStringLiteral("Wrong password"), Qt::CaseInsensitive);
        const QString detail = output_.trimmed().right(600);
        emit finished(
            false,
            wrongPassword
                ? QtLocalText("Wrong password or damaged encrypted archive.",
                              QStringLiteral("Неверный пароль или повреждённый защищённый архив."))
                : QtLocalText("7zz exited with code %1. %2",
                              QStringLiteral("7zz завершился с кодом %1. %2"))
                      .arg(exitCode)
                      .arg(detail));
        return;
    }
    if (completed == Job::ExtractCheck) {
        pendingPassword_ = extractPassword_;
        start(Job::Extract, extractArguments_);
        extractArguments_.clear();
        return;
    }
    if (completed == Job::Extract && !extractQueue_.isEmpty()) {
        pendingPassword_ = extractPassword_;
        const QStringList next = extractQueue_.takeFirst();
        start(Job::Extract, next);
        return;
    }
    if (completed == Job::Extract)
        extractPassword_.clear();
    if (completed == Job::ReplaceDelete) {
        start(Job::ReplaceAdd,
              {"a", "-y", "-bsp1", "-bb1", "-spd", "--", createTemporary_, replacementEntry_},
              replacementRoot_);
        return;
    }
    if (completed == Job::SourcePlanDelete) {
        if (!sourcePlan_.removed.isEmpty() || !sourcePlan_.removedDirectories.isEmpty()) {
            startSourcePlanStage(Job::SourcePlanDelete);
            return;
        }
        if (!sourcePlan_.files.isEmpty()) {
            startSourcePlanStage(Job::SourcePlanFiles);
            return;
        }
        if (!sourcePlan_.directories.isEmpty()) {
            startSourcePlanStage(Job::SourcePlanDirectories);
            return;
        }
        if (!sourcePlan_.renames.isEmpty()) {
            startSourcePlanStage(Job::SourcePlanRename);
            return;
        }
    } else if (completed == Job::SourcePlanFiles && !sourcePlan_.directories.isEmpty()) {
        startSourcePlanStage(Job::SourcePlanDirectories);
        return;
    } else if ((completed == Job::SourcePlanFiles || completed == Job::SourcePlanDirectories) &&
               !sourcePlan_.renames.isEmpty()) {
        startSourcePlanStage(Job::SourcePlanRename);
        return;
    }
    if (completed == Job::SourcePlanDelete || completed == Job::SourcePlanFiles ||
        completed == Job::SourcePlanDirectories || completed == Job::SourcePlanRename)
        sourcePlanPassword_.clear();
    if (completed == Job::Create && !createStages_.isEmpty()) {
        const CreateStage next = createStages_.takeFirst();
        pendingPassword_ = createPassword_;
        start(Job::Create, next.arguments, next.directory);
        return;
    }
    if (completed == Job::Create)
        createPassword_.clear();
    if (completed == Job::Create && !createVolumeSizes_.IsEmpty()) {
        startVolumeOutput();
        return;
    }
#ifndef Q_OS_WIN
    if (completed == Job::Create && createNativeSfx_ &&
        !ArchiveSfxModule::MakeExecutable(createTemporary_)) {
        QFile::remove(createTemporary_);
        emit finished(false, QtLocalText("Could not make the SFX archive executable.",
                                         "Не удалось назначить SFX-архиву право исполнения."));
        return;
    }
#endif
    if (completed == Job::Create && !QFile::rename(createTemporary_, createTarget_)) {
        QFile::remove(createTemporary_);
        emit finished(false, QtLocalText("Could not save new archive: %1",
                                         QStringLiteral("Не удалось сохранить новый архив: %1"))
                                 .arg(createTarget_));
        return;
    }
    if (completed == Job::Create && startCreationDeletion())
        return;
    if (completed == Job::Add || completed == Job::Delete || completed == Job::Rename ||
        completed == Job::ReplaceAdd) {
        if (!QFile::rename(createTarget_, addBackup_)) {
            QFile::remove(createTemporary_);
            emit finished(false, QtLocalText("Could not replace archive: %1",
                                             QStringLiteral("Не удалось заменить архив: %1"))
                                     .arg(createTarget_));
            return;
        }
        if (!QFile::rename(createTemporary_, createTarget_)) {
            const bool restored = QFile::rename(addBackup_, createTarget_);
            QFile::remove(createTemporary_);
            emit finished(
                false,
                restored ? QtLocalText("Could not save updated archive: %1",
                                       QStringLiteral("Не удалось сохранить обновлённый архив: %1"))
                               .arg(createTarget_)
                         : QtLocalText("Could not restore archive. Backup: %1",
                                       QStringLiteral("Не удалось восстановить архив. Копия: %1"))
                               .arg(addBackup_));
            return;
        }
        QFile::remove(addBackup_);
        listed_ = false;
        entries_.clear();
        listedSourceArchives_.remove(createTarget_);
    }
    if (completed == Job::List) {
        QString listing = QString::fromUtf8(listingOutput_);
        listing.replace("\r\n", "\n");
        listing.replace('\r', '\n');
        entries_ = parseListing(listing);
        listed_ = true;
        ArchiveInfo info;
        const QString header = listing.section(QStringLiteral("----------\n"), 0, 0);
        info.properties = parseArchiveHeaderProperties(header);
        for (const QString &line : header.split('\n')) {
            if (line.startsWith(QStringLiteral("Type = ")))
                info.format = line.mid(7);
            else if (line.startsWith(QStringLiteral("Physical Size = "))) {
                bool valid = false;
                const qint64 size = line.mid(16).toLongLong(&valid);
                if (valid)
                    info.physicalSize = size;
            } else if (line.startsWith(QStringLiteral("Total Physical Size = "))) {
                bool valid = false;
                const qint64 size = line.mid(22).toLongLong(&valid);
                if (valid)
                    info.totalPhysicalSize = size;
            } else if (line.startsWith(QStringLiteral("Volumes = "))) {
                bool valid = false;
                const int count = line.mid(10).toInt(&valid);
                if (valid && count > 0)
                    info.volumeCount = count;
            } else if (line.startsWith(QStringLiteral("Offset = "))) {
                bool valid = false;
                const qint64 offset = line.mid(9).toLongLong(&valid);
                if (valid)
                    info.offset = offset;
            }
        }
        assignDefaultEntryNames(info.format);
        for (const ArchiveEntry &entry : entries_) {
            if (entry.directory)
                ++info.folders;
            else {
                ++info.files;
                if (entry.size >= 0)
                    info.unpackedSize += entry.size;
            }
        }
        info.encrypted = output_.contains(QStringLiteral("Encrypted = +")) ||
                         header.contains(QStringLiteral("7zAES"));
        encrypted_ = info.encrypted;
        const QFileInfo listedFile(archive_);
        listedSourceArchives_.insert(archive_,
                                     {listedFile.size(), listedFile.lastModified(),
                                      listedFile.metadataChangeTime(), entries_, encrypted_});
        emit infoReady(info);
        emit entriesReady(entries_);
    }
    lastResult_ = S_OK;
    emit progress(100, QString());
    emit finished(
        true,
        completed == Job::List ? QtLocalText("Archive opened.", QStringLiteral("Архив открыт."))
        : completed == Job::Extract
            ? QtLocalText("Extraction completed.", QStringLiteral("Извлечение завершено."))
        : completed == Job::Test
            ? QtLocalText("No errors found.", QStringLiteral("Ошибок при проверке не найдено."))
        : completed == Job::Hash
            ? QtLocalText("Checksums calculated.", QStringLiteral("Контрольные суммы рассчитаны."))
        : completed == Job::Create
            ? QtLocalText("Archive created.", QStringLiteral("Архив создан."))
        : completed == Job::Delete ? QtLocalText("Items deleted from archive.",
                                                 QStringLiteral("Объекты удалены из архива."))
        : completed == Job::Rename
            ? QtLocalText("Item renamed.", QStringLiteral("Объект переименован."))
        : completed == Job::ReplaceAdd
            ? QtLocalText("File updated in archive.", QStringLiteral("Файл обновлён в архиве."))
            : QtLocalText("Files added.", QStringLiteral("Файлы добавлены.")));
}
