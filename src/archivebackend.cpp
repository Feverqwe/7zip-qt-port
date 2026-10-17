#include "archivebackend.h"
#include "CPP/7zip/UI/Common/CompressCall.h"
#include "CPP/7zip/UI/Common/IFileExtractCallback.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "archivecodeccatalog.h"
#include "archivecodecprobe.h"
#include "archiveenginelocation.h"
#include "archivesfxmodule.h"
#include "processpriority.h"
#include "qtarchivecopypreparation.h"
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
#ifdef Q_OS_WIN
#include "Windows/ProcessUtils.h"
#else
#include <cerrno>
#endif

ArchiveBackend::ArchiveBackend(QObject *parent, QString executable) : QObject(parent) {
    copyProgressTimer_.setInterval(33);
    connect(&copyProgressTimer_, &QTimer::timeout, this, &ArchiveBackend::reportCopyPreparation);
    workerProgressTimer_.setInterval(33);
    connect(&workerProgressTimer_, &QTimer::timeout, this, &ArchiveBackend::reportWorkerProgress);
    if (executable.isEmpty())
        executable = FindArchiveEngine();
    process_.setProgram(executable);
    configuredEngine_ = executable;
    process_.setProcessChannelMode(QProcess::MergedChannels);
    connect(this, &ArchiveBackend::engineStarted, this, &ArchiveBackend::phaseStarted);
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
    if (copyPreparation_) {
        copyPreparation_->setPaused(paused);
        return S_OK;
    }
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
    return processControlResult(processPause_.SetPaused(paused));
}

HRESULT ArchiveBackend::processControlResult(HRESULT result) const {
    if (result == S_OK)
        return S_OK;
    // A short-lived child can exit before QProcess delivers finished(). There
    // is no remaining child to control; keep its actual exit result.
#ifdef Q_OS_WIN
    const UInt32 pid = UInt32(process_.processId());
    if (pid != 0) {
        NWindows::CProcess child;
        if (child.Open(SYNCHRONIZE, false, pid)) {
            if (::WaitForSingleObject(child, 0) == WAIT_OBJECT_0)
                return S_OK;
        } else if (::GetLastError() == ERROR_INVALID_PARAMETER)
            return S_OK;
    }
#else
    if (result == HRESULT_FROM_WIN32(ESRCH))
        return S_OK;
#endif
    return result;
}

HRESULT ArchiveBackend::setBackground(bool background) {
    // The source progress dialog already sets this application's priority.
    // Native workers run in that process rather than a separate CLI child.
    if (copyPreparation_ || hashOperation_ || volumeOutput_ || job_ == Job::CreatePrepare ||
        job_ == Job::CreateDeleteInputs)
        return S_OK;
    if (process_.state() != QProcess::Running)
        return E_FAIL;
    return processControlResult(
        NProcessPriority::SetBackground(UInt32(process_.processId()), background));
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
    if (IsLargePageSupported()) {
        UString switches;
        AddLagePagesSwitch(switches, FmSettingsPath());
        if (!switches.IsEmpty())
            processArguments.insert(1, QString::fromWCharArray(switches).trimmed());
    }
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
    if (copyPreparation_)
        copyPreparation_->cancel();
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
    if (copyPreparation_) {
        copyPreparation_->wait();
        finishCopyPreparation();
    }
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
