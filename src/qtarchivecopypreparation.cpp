#include "qtarchivecopypreparation.h"
#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "Common/StringConvert.h"
#include "qtownedtempdir.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <new>

QtArchiveCopyPreparation::QtArchiveCopyPreparation(QString source, QString output)
    : source_(std::move(source)), output_(std::move(output)),
      total_(quint64(qMax<qint64>(0, QFileInfo(source_).size()))) {}

QtArchiveCopyPreparation::~QtArchiveCopyPreparation() {
    cancel();
    wait();
}

void QtArchiveCopyPreparation::cancel() { progress_.cancel(); }

void QtArchiveCopyPreparation::setPaused(bool paused) { progress_.setPaused(paused); }

HRESULT QtArchiveCopyPreparation::checkpoint() { return progress_.checkpoint(); }

int QtArchiveCopyPreparation::percent() const {
    if (!total_)
        return 0;
    const quint64 completed = qMin(completed_.load(), total_);
    return int(100.0 * double(completed) / double(total_));
}

void QtArchiveCopyPreparation::run() try {
    Result = checkpoint();
    if (Result != S_OK)
        return;
    // Keep incomplete bytes private; failure/cancel cannot remove a colliding
    // caller-owned output. The peer directory also keeps publication on one FS.
    staging_ = std::make_unique<QtOwnedTempDir>(QFileInfo(output_).absolutePath() +
                                                "/.sevenzip-preparation-XXXXXX");
    if (!staging_->isValid()) {
        Result = E_FAIL;
        return;
    }
    const FString source = us2fs(QDir::toNativeSeparators(source_).toStdWString().c_str());
    const FString stage =
        us2fs(QDir::toNativeSeparators(staging_->filePath("archive")).toStdWString().c_str());
    NWindows::NFile::NFind::CFileInfo metadata;
    if (!metadata.Find(source) || metadata.IsOsSymLink()) {
        Result = E_FAIL;
        return;
    }
    NFsFolder::CCopyStateIO copy;
    copy.QtProgress = [this](quint64 completed) {
        completed_.store(completed);
        return checkpoint() == S_OK;
    };
    Result = copy.MyCopyFile(source, stage, metadata.GetWinAttrib());
    if (Result == S_OK && (copy.ErrorFileIndex >= 0 || !copy.ErrorMessage.isEmpty()))
        Result = E_FAIL;
    if (Result == S_OK)
        Result = checkpoint();
} catch (const std::bad_alloc &) {
    Result = E_OUTOFMEMORY;
} catch (...) {
    Result = E_FAIL;
}

HRESULT QtArchiveCopyPreparation::publish() {
    return progress_.withPublicationGuard([this] {
        if (Result != S_OK || !staging_)
            return Result;
        return QFile::rename(staging_->filePath("archive"), output_) ? S_OK : E_FAIL;
    });
}
