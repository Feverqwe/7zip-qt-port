#include "archivecodecprobe.h"
#include "qtlanguageservices.h"
#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
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
    QString executable = program;
    if (QFileInfo(executable).isRelative())
        executable = QStandardPaths::findExecutable(program);
    const QFileInfo engine(executable);
    const QString canonical = engine.canonicalFilePath();
    const QDir directory(canonical.isEmpty() ? engine.absolutePath()
                                             : QFileInfo(canonical).absolutePath());
    // Explicit deployment locations, not arbitrary plugin directories. External
    // plugin discovery/registration remains in its separate source owner.
    const QStringList candidates = {QDir(QCoreApplication::applicationDirPath()).filePath(name),
                                    directory.filePath(name), directory.filePath("../lib/" + name),
                                    directory.filePath("../lib/7zip/" + name)};
    for (const QString &candidate : candidates)
        if (QFileInfo(candidate).isFile())
            return QDir::cleanPath(candidate);
    return directory.filePath(name); // Native loader reports the missing module.
}

UInt32 EngineVersion(const QByteArray &output) {
    // Console/Main.cpp's official banner. There is no upstream parser of this
    // output; only version transport is local, never format/extension inference.
    static const QRegularExpression banner(
        QStringLiteral("(?:^|\\n)7-Zip(?: \\([^\\r\\n]*\\))? ([0-9]+)\\.([0-9]+)(?=\\s|$)"));
    const auto match = banner.match(QString::fromUtf8(output));
    if (!match.hasMatch())
        return 0;
    bool majorOk = false, minorOk = false;
    const quint32 major = match.captured(1).toUInt(&majorOk);
    const quint32 minor = match.captured(2).toUInt(&minorOk);
    return majorOk && minorOk && major && major <= 0xffff && minor <= 0xffff ? (major << 16) | minor
                                                                             : 0;
}

QStringList EngineLibraries(const QByteArray &output) {
    // Console/Main.cpp kInfo prints "index : version : full path" under Libs.
    // This transport confirms shared deployment; native LoadCodecs owns metadata.
    const QString text = QString::fromUtf8(output).replace("\r\n", "\n");
    const int start = text.indexOf("\nLibs:\n");
    if (start < 0)
        return {};
    const int end = text.indexOf("\nFormats:", start);
    if (end < 0)
        return {};
    static const QRegularExpression line(QStringLiteral("^\\s*[0-9]+ : [0-9]+\\.[0-9]+ : (.+)$"));
    QStringList paths;
    for (const QString &row : text.mid(start + 7, end - start - 7).split('\n')) {
        const auto match = line.match(row.trimmed());
        if (match.hasMatch())
            paths.append(match.captured(1));
    }
    return paths;
}

QString DeploymentModule(const QStringList &libraries, const QString &fallback) {
#ifdef _WIN32
    const QString name = QStringLiteral("7z.dll");
#else
    const QString name = QStringLiteral("7z.so");
#endif
    // The original engine has already resolved its registry/deployment folder.
    // Transport that result rather than implement another registry policy.
    for (const QString &library : libraries) {
        const QFileInfo info(library);
        if (info.fileName() == name)
            return library;
    }
    for (const QString &library : libraries) {
        QDir folder(QFileInfo(library).absolutePath());
        if (folder.dirName() == "Codecs" || folder.dirName() == "Formats") {
            folder.cdUp();
            return folder.filePath(name); // May be missing: source load is nonfatal.
        }
    }
    return fallback;
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
    const QString module = requestedModule.isEmpty() ? FindModule(engine) : requestedModule;
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.setProgram(engine);
    process.setArguments({QStringLiteral("i"), QStringLiteral("-sccUTF-8")});
    // Windows creates its pipes/process synchronously inside start(). Keep that
    // platform phase and the process's QObject lifetime wholly in this worker.
    process.start();
    QByteArray output;
    const auto readOutput = [&] {
        const QByteArray bytes = process.readAll();
        output.append(bytes.left(qMax(0, 8192 - int(output.size()))));
    };
    while (process.state() != QProcess::NotRunning) {
        if (work->Stopped()) {
            process.kill();
            process.waitForFinished(3000);
            return result;
        }
        process.waitForFinished(20);
        readOutput();
    }
    readOutput();
    if (work->Stopped())
        return result;
    if (process.error() == QProcess::FailedToStart ||
        process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        result.error = QCoreApplication::translate(
            "archivecodecprobe", "Could not determine the version of official 7zz.");
    } else if (const UInt32 version = EngineVersion(output)) {
        const QStringList libraries = EngineLibraries(output);
        const QString deployment =
            requestedModule.isEmpty() ? DeploymentModule(libraries, module) : module;
        // CCodecs::LoadDll/LoadDllsFromFolder/SetLibCodecs and runtime copying
        // stay inside the existing catalog owner. Only their scheduling changes.
        try {
            result.catalog =
                std::make_shared<ArchiveCodecCatalog>(deployment, version, libraries, engine);
            result.result = result.catalog->result();
            result.error = result.catalog->error();
        } catch (...) {
            result.error =
                QCoreApplication::translate("archivecodecprobe", "Could not load codec metadata.");
        }
    } else
        result.error = QCoreApplication::translate("archivecodecprobe",
                                                   "The 7zz version banner is missing or invalid.");
    return result;
}

ArchiveCodecProbe::ArchiveCodecProbe(QObject *parent) : QObject(parent) {}

ArchiveCodecProbe::~ArchiveCodecProbe() {
    Cancel();
    if (loader_) {
        disconnect(loader_.get(), nullptr, this, nullptr);
        // Neither CreateProcess nor native DLL entrypoints can be interrupted
        // safely. Cancellation kills/reaps a child after start returns; join
        // retains immutable inputs/results and module leases through that work.
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
