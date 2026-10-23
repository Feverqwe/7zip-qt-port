#include "archivecodecprobe.h"
#include "Common/StringConvert.h"
#include "qtlanguageservices.h"
#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDir>
#include <QFileInfo>
#include <QScopeGuard>
#include <QStandardPaths>
#include <atomic>
#include <new>

namespace {
QString FindModule(const QString &program) {
#ifdef _WIN32
    const QString name = QStringLiteral("7z.dll");
#else
    const QString name = QStringLiteral("7z.so");
#endif
    QString executable =
        QFileInfo(program).isRelative() ? QStandardPaths::findExecutable(program) : program;
    // A selected deployment can contain only modules. Do not silently replace
    // an explicit location by an unrelated runtime when its console is absent.
    if (executable.isEmpty() && QDir::fromNativeSeparators(program).contains('/'))
        executable = QFileInfo(program).absoluteFilePath();
    if (executable.isEmpty())
        return QString::fromWCharArray(fs2us(GetBaseFolderPrefixFromRegistry())) + name;
    const QFileInfo engine(executable);
    const QString canonical = engine.canonicalFilePath();
    const QDir directory(canonical.isEmpty() ? engine.absolutePath()
                                             : QFileInfo(canonical).absolutePath());
    // Preserve source LoadCodecs' deployment boundary for plugin-only folders.
    // Loading and enumeration themselves remain in that original module.
    if (QDir(directory.filePath("Codecs")).exists() || QDir(directory.filePath("Formats")).exists())
        return directory.filePath(name);
    const QStringList candidates = {directory.filePath(name), directory.filePath("../lib/" + name),
                                    directory.filePath("../lib/7zip/" + name)};
    for (const QString &candidate : candidates)
        if (QFileInfo(candidate).isFile())
            return QDir::cleanPath(candidate);
    if (engine.isExecutable()) {
        FString prefix =
            us2fs(QDir::toNativeSeparators(directory.absolutePath() + '/').toStdWString().c_str());
        return QString::fromWCharArray(fs2us(GetBaseFolderPrefixFromRegistry(prefix))) + name;
    }
    return directory.filePath(name); // Native loader reports the missing module.
}

} // namespace

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

ArchiveCodecProbeResult ArchiveCodecProbe::Discover(const QString &engine,
                                                    const QString &requestedModule,
                                                    const std::shared_ptr<Work> &work) {
    ArchiveCodecProbeResult result;
    if (work->Stopped())
        return result;
    result.catalog = LoadCatalog(engine, requestedModule);
    result.result = result.catalog->result();
    result.error = result.catalog->error();
    return result;
}

std::shared_ptr<const ArchiveCodecCatalog>
ArchiveCodecProbe::LoadCatalog(const QString &engine, const QString &requestedModule) {
    const QString module = requestedModule.isEmpty() ? FindModule(engine) : requestedModule;
    // CCodecs::LoadDll obtains ABI/version metadata through GetModuleProp;
    // original directory loading and SetLibCodecs bind the retained providers.
    return std::make_shared<ArchiveCodecCatalog>(module, 0, ArchiveCodecCatalog::Scope::Deployment,
                                                 engine);
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

bool ArchiveCodecProbe::Start(const QString &engine, const QString &module,
                              std::function<void(ArchiveCodecProbeResult)> completion,
                              int timeoutMs) {
    if (active_)
        return false;
    try {
        const auto work = std::make_shared<Work>(timeoutMs);
        std::unique_ptr<QThread> loader(QThread::create([work, engine, module] {
            try {
                work->result = Discover(engine, module, work);
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
