#include "archivebackend.h"
#include "qtarchivevolumeoutput.h"
#include "qtlanguageservices.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

bool ArchiveBackend::checkVolumeTargets(const QString &archive, QString *error) {
    const QFileInfo target(archive);
    const QRegularExpression pattern("^" + QRegularExpression::escape(target.fileName()) +
                                     R"(\.\d{3,}$)");
    for (const QString &name : target.dir().entryList(QDir::AllEntries | QDir::Hidden |
                                                      QDir::System | QDir::NoDotAndDotDot))
        if (pattern.match(name).hasMatch()) {
            *error =
                QtLocalText("Volume already exists: %1", QStringLiteral("Том уже существует: %1"))
                    .arg(target.dir().filePath(name));
            return false;
        }
    return true;
}

void ArchiveBackend::startVolumeOutput() {
    job_ = Job::CreateVolumes;
    volumeOutput_ = std::make_unique<QtArchiveVolumeOutput>(
        createTemporary_, createVolumeTarget_, createVolumeSizes_, createSetArcMTime_,
        createNativeSfx_ ? createSfxModule_ : QString(), createTarget_);
    const quint64 generation = ++volumeGeneration_;
    const quint64 progressGeneration = beginWorkerProgress();
    const QString caption =
        QtLocalText("Creating archive volumes", QStringLiteral("Создание архивных томов"));
    volumeOutput_->Progress = [this, progressGeneration, caption](quint64 done, quint64 total) {
        const int percent = total ? int(100.0 * double(done) / double(total)) : 0;
        postWorkerProgress(progressGeneration, percent, caption);
    };
    connect(volumeOutput_.get(), &QThread::finished, this, [this, generation] {
        if (volumeOutput_ && volumeGeneration_ == generation)
            finishVolumeOutput();
    });
    emit progress(0, caption);
    if (volumeOutput_ && volumeGeneration_ == generation)
        startWorkerPhase(*volumeOutput_);
}

void ArchiveBackend::finishVolumeOutput() {
    if (!volumeOutput_)
        return;
    volumeOutput_->wait();
    finishWorkerProgress();
    lastResult_ = volumeOutput_->Result;
    if (cancelled_)
        lastResult_ = E_ABORT;
    const QString error =
        cancelled_ ? QtLocalText("Operation canceled.", QStringLiteral("Операция отменена."))
                   : volumeOutput_->Error;
    volumeOutput_.reset();
    QFile::remove(createTemporary_);
    createVolumeSizes_.Clear();
    job_ = Job::None;
    if (lastResult_ == S_OK && startCreationDeletion())
        return;
    emit finished(lastResult_ == S_OK, lastResult_ == S_OK
                                           ? QtLocalText("Archive volumes created.",
                                                         QStringLiteral("Архивные тома созданы."))
                                           : error);
}
