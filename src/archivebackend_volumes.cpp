#include "archivebackend.h"
#include "qtarchivevolumeoutput.h"
#include "qtlanguageservices.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QRegularExpression>

bool ArchiveBackend::checkVolumeTargets(const QString &archive, QString *error) {
    const QFileInfo target(archive);
    const QRegularExpression pattern("^" + QRegularExpression::escape(target.fileName()) +
                                     R"(\.\d{3,}$)");
    for (const QString &name : target.dir().entryList(QDir::AllEntries | QDir::Hidden |
                                                      QDir::System | QDir::NoDotAndDotDot))
        if (pattern.match(name).hasMatch()) {
            *error =
                QCoreApplication::translate("archivebackend_volumes", "Volume already exists: %1")
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
        QCoreApplication::translate("archivebackend_volumes", "Creating archive volumes");
    volumeOutput_->Progress = [this, progressGeneration, caption](quint64 done, quint64 total) {
        const int percent = total ? int(100.0 * double(done) / double(total)) : 0;
        postWorkerProgress(progressGeneration, percent, caption);
    };
    connect(volumeOutput_.get(), &QThread::finished, this, [this, generation] {
        if (volumeOutput_ && volumeGeneration_ == generation)
            finishVolumeOutput();
    });
    const QPointer<ArchiveBackend> owner(this);
    emit progress(0, caption);
    if (owner && volumeOutput_ && volumeGeneration_ == generation)
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
        cancelled_ ? QCoreApplication::translate("archivebackend_volumes", "Operation canceled.")
                   : volumeOutput_->Error;
    volumeOutput_.reset();
    QFile::remove(createTemporary_);
    createVolumeSizes_.Clear();
    job_ = Job::None;
    if (lastResult_ == S_OK && startCreationDeletion())
        return;
    emit finished(lastResult_ == S_OK, lastResult_ == S_OK
                                           ? QCoreApplication::translate("archivebackend_volumes",
                                                                         "Archive volumes created.")
                                           : error);
}
