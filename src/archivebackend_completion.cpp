#include "archivebackend.h"
#include "archivesfxmodule.h"
#include "qtlanguageservices.h"
#include "qtownedtempdir.h"

#include <QFile>

bool ArchiveBackend::advanceProcessJob(Job completed) {
    if (completed == Job::ExtractCheck) {
        pendingPassword_ = extractPassword_;
        start(Job::Extract, extractArguments_);
        extractArguments_.clear();
        return true;
    }
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
        emit finished(false,
                      QtLocalText("Operation canceled.", QStringLiteral("Операция отменена.")));
        return true;
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
        return true;
    }
    return false;
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
    if (failProcessJob(completed, exitCode, status))
        return;
    if (advanceProcessJob(completed))
        return;
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
    if ((completed == Job::Add || completed == Job::Delete || completed == Job::Rename ||
         completed == Job::ReplaceAdd) &&
        !publishProcessMutation())
        return;
    if (completed == Job::List)
        finishProcessListing();
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
