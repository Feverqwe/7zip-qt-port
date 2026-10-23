#include "archivebackend.h"
#include "CPP/7zip/UI/Common/IFileExtractCallback.h"
#include "archivecodeccatalog.h"
#include "archivecodecprobe.h"
#include "archiveenginelocation.h"
#include "archivesfxmodule.h"
#include "qtarchivecopypreparation.h"
#include "qtarchivecreationlifecycle.h"
#include "qtarchivedirectoryextract.h"
#include "qtarchivehashoperation.h"
#include "qtarchivereadoperation.h"
#include "qtarchivevolumeoutput.h"
#include "qtlanguageservices.h"

#include "qtownedtempdir.h"
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
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
    configuredEngine_ = executable;
    process_ = makeProcess(executable);
    connect(this, &ArchiveBackend::engineStarted, this, &ArchiveBackend::phaseStarted);
}

ArchiveBackend::~ArchiveBackend() {
    // Joining is cleanup here: finished receivers must not start a continuation
    // while the backend and its worker owners are being destroyed.
    blockSignals(true);
    codecProbe_.reset();
    // QProcess can emit readyRead/finished during destruction. Stop callbacks
    // while their buffers and operation state are still alive.
    disconnect(process_.get(), nullptr, this, nullptr);
    cancelAndWait();
    if (process_->state() == QProcess::NotRunning &&
        (job_ == Job::Create || job_ == Job::Add || job_ == Job::Delete || job_ == Job::Rename ||
         job_ == Job::ReplaceDelete || job_ == Job::ReplaceAdd || job_ == Job::SourceUpdate ||
         job_ == Job::SourcePlanDelete || job_ == Job::SourcePlanFiles ||
         job_ == Job::SourcePlanDirectories || job_ == Job::SourceMutation ||
         job_ == Job::SourcePlanRename))
        QFile::remove(createTemporary_);
}

bool ArchiveBackend::busy() const {
    return job_ != Job::None || overwritePromptActive_ || codecDiscoveryActive_;
}

HRESULT ArchiveBackend::lastResult() const { return lastResult_; }

HRESULT ArchiveBackend::setPaused(bool paused) {
    if (readOperation_) {
        readOperation_->setPaused(paused);
        return S_OK;
    }
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
    if (process_->state() != QProcess::Running)
        return E_FAIL;
    return processControlResult(processPause_.SetPaused(paused));
}

HRESULT ArchiveBackend::processControlResult(HRESULT result) const {
    if (result == S_OK)
        return S_OK;
    // A short-lived child can exit before QProcess delivers finished(). There
    // is no remaining child to control; keep its actual exit result.
#ifdef Q_OS_WIN
    const UInt32 pid = UInt32(process_->processId());
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

void ArchiveBackend::provideOverwriteAnswer(int answer) {
    if (overwritePromptActive_ && readOperation_) {
        overwritePromptActive_ = false;
        readOperation_->provideOverwriteAnswer(answer);
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
                emit finished(false, QCoreApplication::translate("archivebackend",
                                                                 "Invalid archive open type: %1")
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
        emit finished(false, QCoreApplication::translate("archivebackend",
                                                         "Another operation is still running."));
        return;
    }
    archive_ = QFileInfo(archive).absoluteFilePath();
    listedSourceArchives_.remove(archive_);
    entries_.clear();
    listed_ = false;
    encrypted_ = false;
    pendingPassword_ = password;
    startNativeRead(Job::List, archive_, password, format);
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

bool ArchiveBackend::restoreListedArchive(const QString &archive, bool allowLimited) {
    if (!hasListedSourceArchive(archive))
        return false;
    const QString path = QFileInfo(archive).absoluteFilePath();
    const ListedSourceSnapshot &snapshot = listedSourceArchives_.constFind(path).value();
    if (!allowLimited && (snapshot.LimitedRead || snapshot.ReadOnly))
        return false;
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
        emit finished(false, QCoreApplication::translate("archivebackend",
                                                         "Another operation is still running."));
        return;
    }
    lastResult_ = E_FAIL;
    cancelled_ = false;
    if (!restoreListedArchive(archive)) {
        emit finished(
            false, QCoreApplication::translate("archivebackend",
                                               "Open the archive and inspect its contents first."));
        return;
    }
    if (options.pathMode == ExtractionOptions::PathMode::AbsolutePaths || options.ElimDup.Val ||
        options.NtSecurity.Val) {
        emit finished(
            false, QCoreApplication::translate(
                       "archivebackend", "These extraction options require the source workflow."));
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
            emit finished(
                false, QCoreApplication::translate("archivebackend",
                                                   "Selected file is missing from the archive: %1")
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
        emit finished(false, validationError.isEmpty()
                                 ? QCoreApplication::translate(
                                       "archivebackend", "Could not create destination folder: %1")
                                       .arg(destination)
                                 : validationError);
        return;
    }
    auto request = std::make_shared<QtArchiveDirectoryExtractRequest>();
    request->Destination = outputDirectory;
    request->Selected = selectedEntries;
    request->Options = options;
    pendingPassword_ = password;
    startNativeRead(Job::Extract, archive_, password, {}, std::move(request));
}

void ArchiveBackend::test(const QString &archive, const QString &password) {
    if (busy()) {
        emit finished(false, QCoreApplication::translate("archivebackend",
                                                         "Another operation is still running."));
        return;
    }
    lastResult_ = E_FAIL;
    if (!restoreListedArchive(archive)) {
        emit finished(
            false, QCoreApplication::translate("archivebackend",
                                               "Open the archive and inspect its contents first."));
        return;
    }
    pendingPassword_ = password;
    startNativeRead(Job::Test, archive_, password);
}

void ArchiveBackend::testSourceArchive(const QString &archive, const QString &password) {
    if (busy()) {
        emit finished(false, QCoreApplication::translate("archivebackend",
                                                         "Another operation is still running."));
        return;
    }
    const QFileInfo file(archive);
    if (!file.isFile() || file.isSymLink()) {
        lastResult_ = E_INVALIDARG;
        emit finished(
            false, QCoreApplication::translate("archivebackend", "Source archive is unavailable."));
        return;
    }
    archive_ = file.absoluteFilePath();
    pendingPassword_ = password;
    startNativeRead(Job::Test, archive_, password);
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
    if (readOperation_)
        readOperation_->cancel();
    if (creationLifecycle_)
        creationLifecycle_->cancel();
    if (volumeOutput_)
        volumeOutput_->cancel();
    pendingPassword_.clear();
    if (overwritePromptActive_) {
        const QPointer<ArchiveBackend> owner(this);
        emit dismissOverwriteRequested();
        if (!owner)
            return;
    }
    if (process_->state() != QProcess::NotRunning)
        process_->kill();
}

void ArchiveBackend::cancelAndWait() {
    const QPointer<ArchiveBackend> owner(this);
    const auto transport = process_;
    if (busy())
        cancel();
    if (!owner)
        return;
    // Finish only the current native phase. Its terminal signal can release
    // this owner or start another operation, which this call must not join.
    if (copyPreparation_) {
        copyPreparation_->wait();
        finishCopyPreparation();
        return;
    }
    if (hashOperation_) {
        hashOperation_->wait();
        finishHashOperation();
        return;
    }
    if (readOperation_) {
        readOperation_->wait();
        finishReadOperation();
        return;
    }
    if (creationLifecycle_ && (job_ == Job::CreatePrepare || job_ == Job::CreateDeleteInputs)) {
        creationLifecycle_->wait();
        finishCreationLifecycle();
        return;
    }
    if (volumeOutput_) {
        if (!volumeOutput_->isRunning() && !volumeOutput_->isFinished()) {
            volumeOutput_->Result = E_ABORT;
            volumeOutput_->Error =
                QCoreApplication::translate("archivebackend", "Operation canceled.");
        }
        volumeOutput_->wait();
        finishVolumeOutput();
        return;
    }
    if (transport->state() != QProcess::NotRunning) {
        transport->kill();
        transport->waitForFinished(-1);
        if (!owner || process_ != transport)
            return;
    }
    processPause_.ResetAfterExit();
}
