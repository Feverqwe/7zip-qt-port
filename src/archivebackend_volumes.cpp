#include "archivebackend.h"
#include "qtarchivevolumeoutput.h"
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
            *error = QStringLiteral("Том уже существует: %1").arg(target.dir().filePath(name));
            return false;
        }
    return true;
}

void ArchiveBackend::startVolumeOutput() {
    job_ = Job::CreateVolumes;
    volumeOutput_ = std::make_unique<QtArchiveVolumeOutput>(createTemporary_, createTarget_,
                                                            createVolumeSizes_, createSetArcMTime_);
    volumeOutput_->Progress = [this](quint64 done, quint64 total) {
        const int percent = total ? int(100.0 * double(done) / double(total)) : 0;
        QMetaObject::invokeMethod(
            this,
            [this, percent] {
                if (job_ == Job::CreateVolumes)
                    emit progress(percent, QStringLiteral("Создание архивных томов"));
            },
            Qt::QueuedConnection);
    };
    connect(volumeOutput_.get(), &QThread::finished, this, &ArchiveBackend::finishVolumeOutput);
    emit progress(0, QStringLiteral("Создание архивных томов"));
    if (volumeOutput_)
        volumeOutput_->start();
}

void ArchiveBackend::finishVolumeOutput() {
    if (!volumeOutput_)
        return;
    volumeOutput_->wait();
    lastResult_ = volumeOutput_->Result;
    if (cancelled_)
        lastResult_ = E_ABORT;
    const QString error = cancelled_ ? QStringLiteral("Операция отменена.") : volumeOutput_->Error;
    volumeOutput_.reset();
    QFile::remove(createTemporary_);
    createVolumeSizes_.Clear();
    job_ = Job::None;
    if (lastResult_ == S_OK && startCreationDeletion())
        return;
    emit finished(lastResult_ == S_OK,
                  lastResult_ == S_OK ? QStringLiteral("Архивные тома созданы.") : error);
}
