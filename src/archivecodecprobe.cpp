#include "archivecodecprobe.h"
#include "qtlanguageservices.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

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

ArchiveCodecProbe::ArchiveCodecProbe(QObject *parent) : QObject(parent) {
    process_.setProcessChannelMode(QProcess::MergedChannels);
    deadline_.setSingleShot(true);
    connect(&process_, &QProcess::started, this, [this] {
        if (cancelled_ || timedOut_)
            process_.kill();
    });
    connect(&process_, &QProcess::readyRead, this, [this] {
        const QByteArray bytes = process_.readAll();
        output_.append(bytes.left(qMax(0, 8192 - int(output_.size()))));
    });
    connect(&process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            &ArchiveCodecProbe::Finish);
    connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            Finish(-1, QProcess::CrashExit);
    });
    connect(&deadline_, &QTimer::timeout, this, [this] {
        timedOut_ = true;
        process_.kill();
    });
}

ArchiveCodecProbe::~ArchiveCodecProbe() {
    disconnect(&process_, nullptr, this, nullptr);
    deadline_.stop();
    if (process_.state() != QProcess::NotRunning) {
        process_.kill();
        process_.waitForFinished(3000);
    }
}

bool ArchiveCodecProbe::Start(const QString &engine, const QString &module,
                              std::function<void(ArchiveCodecProbeResult)> completion,
                              int timeoutMs) {
    if (active_)
        return false;
    output_.clear();
    cancelled_ = false;
    timedOut_ = false;
    active_ = true;
    completion_ = std::move(completion);
    explicitModule_ = !module.isEmpty();
    module_ = module.isEmpty() ? FindModule(engine) : module;
    process_.setProgram(engine);
    process_.setArguments({QStringLiteral("i"), QStringLiteral("-sccUTF-8")});
    deadline_.start(qMax(1, timeoutMs));
    process_.start();
    return true;
}

void ArchiveCodecProbe::Cancel() {
    if (!active_)
        return;
    cancelled_ = true;
    process_.kill();
}

void ArchiveCodecProbe::Finish(int exitCode, QProcess::ExitStatus status) {
    if (!active_)
        return;
    deadline_.stop();
    const QByteArray remaining = process_.readAll();
    output_.append(remaining.left(qMax(0, 8192 - int(output_.size()))));
    ArchiveCodecProbeResult result;
    if (cancelled_ || timedOut_) {
        result.result = E_ABORT;
        result.error =
            timedOut_ ? QtLocalText("7zz version detection timed out.",
                                    "Истекло время определения версии 7zz.")
                      : QtLocalText("Codec discovery cancelled.", "Поиск модуля форматов отменён.");
    } else if (status != QProcess::NormalExit || exitCode != 0) {
        result.error = QtLocalText("Could not determine the version of official 7zz.",
                                   "Не удалось определить версию официального 7zz.");
    } else if (const UInt32 version = EngineVersion(output_)) {
        try {
            const QStringList libraries = EngineLibraries(output_);
            const QString module = explicitModule_ ? module_ : DeploymentModule(libraries, module_);
            result.catalog = std::make_shared<ArchiveCodecCatalog>(module, version, libraries,
                                                                   process_.program());
            result.result = result.catalog->result();
            result.error = result.catalog->error();
        } catch (...) {
            result.error = QtLocalText("Could not load codec metadata.",
                                       "Не удалось загрузить сведения о форматах.");
        }
    } else
        result.error = QtLocalText("The 7zz version banner is missing or invalid.",
                                   "Строка версии 7zz отсутствует или некорректна.");
    active_ = false;
    const auto completion = std::move(completion_);
    if (completion)
        completion(std::move(result));
}
