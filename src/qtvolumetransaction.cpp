#include "qtvolumetransaction.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "qtlangcontrols.h"
#include <QFile>
#include <QFileInfo>
#include <QScopeGuard>
#include <QUuid>

QtVolumeTransaction::~QtVolumeTransaction() {
    if (committed_)
        return;
    for (const QString &path : staged_)
        QFile::remove(path);
    for (qsizetype i = 0; i < publishedCount_; ++i)
        QFile::remove(targets_[i]);
}

bool QtVolumeTransaction::create(NWindows::NFile::NIO::COutFile &file, const FString &target) {
    const QString path = ToQString(fs2us(target));
    if (QFileInfo::exists(path) || QFileInfo(path).isSymLink()) {
        failedPath_ = path;
        return false;
    }
    const QString stage = path + ".sevenzip-" + QUuid::createUuid().toString(QUuid::Id128);
    const FString nativeStage = us2fs(ToUString(stage));
    // Register both names before creating a file that rollback must own.
    targets_.append(path);
    auto removeTarget = qScopeGuard([&] { targets_.removeLast(); });
    staged_.append(stage);
    auto removeStage = qScopeGuard([&] { staged_.removeLast(); });
    if (!file.Create_NEW(nativeStage)) {
        failedPath_ = path;
        return false;
    }
    removeStage.dismiss();
    removeTarget.dismiss();
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
        ++publishedCount_;
    }
    committed_ = true;
    return true;
}
