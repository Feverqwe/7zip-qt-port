#include "qtvolumetransaction.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "qtlangcontrols.h"
#include <QFile>
#include <QFileInfo>
#include <QUuid>

QtVolumeTransaction::~QtVolumeTransaction() {
    if (committed_)
        return;
    for (const QString &path : staged_)
        QFile::remove(path);
    for (const QString &path : published_)
        QFile::remove(path);
}

bool QtVolumeTransaction::create(NWindows::NFile::NIO::COutFile &file, const FString &target) {
    const QString path = ToQString(fs2us(target));
    if (QFileInfo::exists(path) || QFileInfo(path).isSymLink()) {
        failedPath_ = path;
        return false;
    }
    const QString stage = path + ".sevenzip-" + QUuid::createUuid().toString(QUuid::Id128);
    if (!file.Create_NEW(us2fs(ToUString(stage)))) {
        failedPath_ = path;
        return false;
    }
    targets_ << path;
    staged_ << stage;
    return true;
}

bool QtVolumeTransaction::publish() {
    for (qsizetype i = 0; i < targets_.size(); ++i) {
        const QString &target = targets_[i];
        if (QFileInfo::exists(target) || QFileInfo(target).isSymLink() ||
            !QFile::rename(staged_[i], target)) {
            failedPath_ = target;
            return false;
        }
        published_ << target;
    }
    committed_ = true;
    return true;
}
