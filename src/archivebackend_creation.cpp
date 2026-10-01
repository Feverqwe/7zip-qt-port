#include "archivebackend.h"
#include "qtarchivecreationlifecycle.h"

void ArchiveBackend::finishCreationLifecycle() {
    if (!creationLifecycle_ || (job_ != Job::CreatePrepare && job_ != Job::CreateDeleteInputs))
        return;
    creationLifecycle_->wait();
    const bool preparing = job_ == Job::CreatePrepare;
    lastResult_ = creationLifecycle_->Result;
    if (cancelled_)
        lastResult_ = E_ABORT;
    const QString error =
        cancelled_ ? QStringLiteral("Операция отменена.") : creationLifecycle_->Error;
    job_ = Job::None;
    if (lastResult_ != S_OK) {
        creationLifecycle_.reset();
        createStages_.clear();
        createPassword_.clear();
        emit finished(false, error);
        return;
    }
    if (preparing) {
        const CreateStage first = createStages_.takeFirst();
        pendingPassword_ = createPassword_;
        start(Job::Create, first.arguments, first.directory);
    } else {
        creationLifecycle_.reset();
        emit finished(true, QStringLiteral("Архив создан."));
    }
}

bool ArchiveBackend::startCreationDeletion() {
    if (!creationLifecycle_)
        return false;
    job_ = Job::CreateDeleteInputs;
    creationLifecycle_->Progress = [this](const QString &path, bool) {
        QMetaObject::invokeMethod(
            this, [this, path] { emit progress(100, QStringLiteral("Удаление: %1").arg(path)); },
            Qt::QueuedConnection);
    };
    creationLifecycle_->beginDeletion();
    return true;
}
