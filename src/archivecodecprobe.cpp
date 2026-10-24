#include "archivecodecprobe.h"
#include "Common/StringConvert.h"
#include "archivecodelocation.h"
#include "qtlanguageservices.h"
#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDir>
#include <QFileInfo>
#include <QScopeGuard>
#include <atomic>
#include <new>

struct ArchiveCodecProbe::Work {
    std::atomic<bool> cancelled{false};
    std::atomic<bool> timedOut{false};
    const QDeadlineTimer deadline;
    ArchiveCodecProbeResult result;

    explicit Work(int timeoutMs) : deadline(qMax(1, timeoutMs), Qt::PreciseTimer) {}

    bool Stopped() {
        if (deadline.hasExpired())
            timedOut = true;
        return cancelled || timedOut;
    }
};

ArchiveCodecProbeResult ArchiveCodecProbe::Discover(const QString &module,
                                                    const std::shared_ptr<Work> &work) {
    ArchiveCodecProbeResult result;
    if (work->Stopped())
        return result;
    result.catalog = LoadCatalog(module);
    result.result = result.catalog->result();
    result.error = result.catalog->error();
    return result;
}

std::shared_ptr<const ArchiveCodecCatalog> ArchiveCodecProbe::LoadCatalog(const QString &module) {
    // CCodecs::LoadDll obtains ABI/version metadata through GetModuleProp;
    // original directory loading and SetLibCodecs bind the retained providers.
    return std::make_shared<ArchiveCodecCatalog>(module.isEmpty() ? FindArchiveModule() : module, 0,
                                                 ArchiveCodecCatalog::Scope::Deployment);
}

ArchiveCodecProbe::ArchiveCodecProbe(QObject *parent) : QObject(parent) {}

ArchiveCodecProbe::~ArchiveCodecProbe() {
    Cancel();
    if (loader_) {
        disconnect(loader_.get(), nullptr, this, nullptr);
        // Native DLL entrypoints cannot be interrupted safely. Join retains
        // immutable inputs/results and module leases through that work.
        loader_->wait();
    }
}

bool ArchiveCodecProbe::Start(const QString &module,
                              std::function<void(ArchiveCodecProbeResult)> completion,
                              int timeoutMs) {
    if (active_)
        return false;
    try {
        const auto work = std::make_shared<Work>(timeoutMs);
        std::unique_ptr<QThread> loader(QThread::create([work, module] {
            try {
                work->result = Discover(module, work);
            } catch (const std::bad_alloc &) {
                work->result.result = E_OUTOFMEMORY;
            } catch (...) {
                work->result.result = E_FAIL;
            }
            // Native DLL calls cannot be interrupted safely. Account for their
            // elapsed time before queueing the joined result back to the GUI.
            work->Stopped();
        }));
        const auto stopUnpublished = qScopeGuard([&] {
            if (loader) {
                work->cancelled = true;
                loader->wait();
            }
        });
        auto *thread = loader.get();
        connect(
            thread, &QThread::finished, this,
            [this, work, thread] {
                if (loader_.get() != thread || work_ != work)
                    return;
                loader_->wait();
                loader_.reset();
                Complete(std::move(work->result));
            },
            Qt::QueuedConnection);
        loader->start();
        if (!loader->isRunning() && !loader->isFinished())
            return false;
        // Publish only fully prepared owners. Queued delivery cannot consume
        // this state until Start has returned to the owning event loop.
        completion_ = std::move(completion);
        work_ = work;
        loader_ = std::move(loader);
        active_ = true;
        return true;
    } catch (...) {
        return false;
    }
}

void ArchiveCodecProbe::Cancel() {
    if (active_)
        work_->cancelled = true;
}

void ArchiveCodecProbe::Complete(ArchiveCodecProbeResult result) {
    if (work_->cancelled || work_->timedOut) {
        result.catalog.reset();
        result.result = E_ABORT;
        result.error =
            work_->timedOut
                ? QCoreApplication::translate("archivecodecprobe", "Codec discovery timed out.")
                : QCoreApplication::translate("archivecodecprobe", "Codec discovery cancelled.");
    }
    active_ = false;
    const auto completion = std::move(completion_);
    if (completion)
        completion(std::move(result));
}
