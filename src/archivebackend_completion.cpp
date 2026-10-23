#include "archivebackend.h"
#include "archivesfxmodule.h"
#include "qtlanguageservices.h"
#include "qtownedtempdir.h"

#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <utility>

bool ArchiveBackend::advanceProcessJob(Job completed) {
    if (completed == Job::Extract && !extractQueue_.isEmpty()) {
        pendingPassword_ = extractPassword_;
        const QStringList next = extractQueue_.takeFirst();
        start(Job::Extract, next);
        return true;
    }
    if (completed == Job::Extract)
        extractPassword_.clear();
    if (completed == Job::ReplaceDelete) {
        start(Job::ReplaceAdd,
              {"a", "-y", "-bsp1", "-bb1", "-spd", "--", createTemporary_, replacementEntry_},
              replacementRoot_);
        return true;
    }
    if (completed == Job::SourcePlanDelete) {
        if (!sourcePlan_.removed.isEmpty() || !sourcePlan_.removedDirectories.isEmpty()) {
            startSourcePlanStage(Job::SourcePlanDelete);
            return true;
        }
        if (!sourcePlan_.files.isEmpty()) {
            startSourcePlanStage(Job::SourcePlanFiles);
            return true;
        }
        if (!sourcePlan_.directories.isEmpty()) {
            startSourcePlanStage(Job::SourcePlanDirectories);
            return true;
        }
        if (!sourcePlan_.renames.isEmpty()) {
            startSourcePlanStage(Job::SourcePlanRename);
            return true;
        }
    } else if (completed == Job::SourcePlanFiles && !sourcePlan_.directories.isEmpty()) {
        startSourcePlanStage(Job::SourcePlanDirectories);
        return true;
    } else if ((completed == Job::SourcePlanFiles || completed == Job::SourcePlanDirectories) &&
               !sourcePlan_.renames.isEmpty()) {
        startSourcePlanStage(Job::SourcePlanRename);
        return true;
    }
    if (completed == Job::SourcePlanDelete || completed == Job::SourcePlanFiles ||
        completed == Job::SourcePlanDirectories || completed == Job::SourcePlanRename)
        sourcePlanPassword_.clear();
    if (completed == Job::Create && !createStages_.isEmpty()) {
        const CreateStage next = createStages_.takeFirst();
        pendingPassword_ = createPassword_;
        start(Job::Create, next.arguments, next.directory);
        return true;
    }
    if (completed == Job::Create)
        createPassword_.clear();
    if (completed == Job::Create && !createVolumeSizes_.IsEmpty()) {
        startVolumeOutput();
        return true;
    }
    return false;
}

bool ArchiveBackend::failProcessJob(Job completed, int exitCode, QProcess::ExitStatus status) {
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
        emit finished(
            false, QCoreApplication::translate("archivebackend_completion", "Operation canceled."));
        return true;
    }
    if (status != QProcess::NormalExit || exitCode != 0) {
        if (completed == Job::List)
            lastResult_ = listingFailureResult(exitCode, status);
        const bool partialStream = completed == Job::SourceExtract && sourceExtractRecovery_ &&
                                   status == QProcess::NormalExit &&
                                   (exitCode == 1 || exitCode == 2);
        if (partialStream) {
            lastResult_ = S_FALSE;
            sourceExtractOperation_ = sourceExtractErrorOperation();
        }
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
            completed == Job::SourcePlanRename ||
            (completed == Job::SourceExtract &&
             (!partialStream || QFileInfo(createTemporary_).size() <= 0)))
            QFile::remove(createTemporary_);
        const bool wrongPassword =
            output_.contains(QStringLiteral("Wrong password"), Qt::CaseInsensitive);
        const QString detail = output_.trimmed().right(600);
        emit finished(
            false, wrongPassword
                       ? QCoreApplication::translate("archivebackend_completion",
                                                     "Wrong password or damaged encrypted archive.")
                       : QCoreApplication::translate("archivebackend_completion",
                                                     "7zz exited with code %1. %2")
                             .arg(exitCode)
                             .arg(detail));
        return true;
    }
    return false;
}

void ArchiveBackend::complete(int exitCode, QProcess::ExitStatus status) {
    processPause_.ResetAfterExit();
    if (job_ == Job::None)
        return;
    const QPointer<ArchiveBackend> owner(this);
    const auto transport = process_;
    const Job completed = job_;
    consumeOutput(transport->readAll(), true);
    if (!owner || process_ != transport || job_ != completed)
        return;
    if (completed == Job::SourceExtract) {
        consumeOutput(transport->readAllStandardError(), true);
        if (!owner || process_ != transport || job_ != completed)
            return;
    }
    pendingPassword_.clear();
    job_ = Job::None;
    if (completed == Job::SourceExtract)
        sourceExtractSelection_.reset();
    const QString limitations = !cancelled_ && completed == Job::List &&
                                        status == QProcess::NormalExit &&
                                        (exitCode == 0 || exitCode == 1 || exitCode == 2)
                                    ? processListingLimitations()
                                    : QString();
    if (limitations.isEmpty() && failProcessJob(completed, exitCode, status))
        return;
    if (advanceProcessJob(completed))
        return;
#ifndef Q_OS_WIN
    if (completed == Job::Create && createNativeSfx_ &&
        !ArchiveSfxModule::MakeExecutable(createTemporary_)) {
        QFile::remove(createTemporary_);
        emit finished(false,
                      QCoreApplication::translate("archivebackend_completion",
                                                  "Could not make the SFX archive executable."));
        return;
    }
#endif
    if (completed == Job::Create && !QFile::rename(createTemporary_, createTarget_)) {
        QFile::remove(createTemporary_);
        emit finished(false, QCoreApplication::translate("archivebackend_completion",
                                                         "Could not save new archive: %1")
                                 .arg(createTarget_));
        return;
    }
    if (completed == Job::Create && startCreationDeletion())
        return;
    if ((completed == Job::Add || completed == Job::Delete || completed == Job::Rename ||
         completed == Job::ReplaceAdd) &&
        !publishProcessMutation())
        return;
    lastResult_ = S_OK;
    if (completed == Job::SourceExtract)
        sourceExtractOperation_ = NArchive::NExtract::NOperationResult::kOK;
    if (completed == Job::List) {
        finishProcessListing(limitations);
        if (!owner)
            return;
    }
    emit progress(100, QString());
    if (!owner)
        return;
    emit finished(
        true,
        completed == Job::List
            ? QCoreApplication::translate("archivebackend_completion", "Archive opened.")
        : completed == Job::Extract
            ? QCoreApplication::translate("archivebackend_completion", "Extraction completed.")
        : completed == Job::Test
            ? QCoreApplication::translate("archivebackend_completion", "No errors found.")
        : completed == Job::Hash
            ? QCoreApplication::translate("archivebackend_completion", "Checksums calculated.")
        : completed == Job::Create
            ? QCoreApplication::translate("archivebackend_completion", "Archive created.")
        : completed == Job::Delete ? QCoreApplication::translate("archivebackend_completion",
                                                                 "Items deleted from archive.")
        : completed == Job::Rename
            ? QCoreApplication::translate("archivebackend_completion", "Item renamed.")
        : completed == Job::ReplaceAdd
            ? QCoreApplication::translate("archivebackend_completion", "File updated in archive.")
            : QCoreApplication::translate("archivebackend_completion", "Files added."));
}
