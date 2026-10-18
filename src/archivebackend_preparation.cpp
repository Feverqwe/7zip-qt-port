#include "7zip/UI/FileManager/resource.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "archivebackend.h"
#include "qtarchivecopypreparation.h"
#include "qtlanguageservices.h"

void ArchiveBackend::prepareArchiveCopy(const QString &source, const QString &output,
                                        std::function<void()> continuation) {
    if (busy()) {
        emit finished(false, QCoreApplication::translate("archivebackend_preparation",
                                                         "Another operation is still running."));
        return;
    }
    copyPreparation_ = std::make_unique<QtArchiveCopyPreparation>(source, output);
    copyContinuation_ = std::move(continuation);
    copyProgressDetail_ = QString::fromWCharArray(LangString(IDS_COPYING).Ptr()) + ' ' + source;
    const quint64 generation = ++copyGeneration_;
    connect(copyPreparation_.get(), &QThread::finished, this, [this, generation] {
        if (generation == copyGeneration_)
            finishCopyPreparation();
    });
    lastResult_ = E_FAIL;
    cancelled_ = false;
    job_ = Job::CopyPrepare;
    copyProgressTimer_.start();
    if (!startWorkerPhase(*copyPreparation_))
        return;
    // Last operation: a progress consumer may synchronously cancel/join/delete.
    const QString detail = copyProgressDetail_;
    emit progress(0, detail);
}

void ArchiveBackend::reportCopyPreparation() {
    if (job_ == Job::CopyPrepare && copyPreparation_) {
        const QString detail = copyProgressDetail_;
        emit progress(copyPreparation_->percent(), detail);
    }
}

void ArchiveBackend::finishCopyPreparation() {
    if (job_ != Job::CopyPrepare || !copyPreparation_)
        return;
    copyPreparation_->wait();
    copyProgressTimer_.stop();
    lastResult_ = cancelled_ ? E_ABORT : copyPreparation_->Result;
    if (lastResult_ == S_OK)
        lastResult_ = copyPreparation_->publish();
    auto continuation = std::move(copyContinuation_);
    copyPreparation_.reset();
    copyProgressDetail_.clear();
    job_ = Job::None;
    if (lastResult_ != S_OK) {
        pendingPassword_.clear();
        sourcePlanPassword_.clear();
        sourcePlan_ = {};
        sourcePlanSwitches_.clear();
        // A refused publication can leave a caller-owned collision at this path.
        createTemporary_.clear();
        const QString error =
            lastResult_ == E_ABORT
                ? QCoreApplication::translate("archivebackend_preparation", "Operation canceled.")
                : QCoreApplication::translate("archivebackend_preparation",
                                              "Could not prepare a copy of the archive.");
        emit finished(false, error);
        return;
    }
    if (continuation)
        continuation();
}
