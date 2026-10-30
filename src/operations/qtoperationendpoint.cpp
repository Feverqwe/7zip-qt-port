#include "operations/qtoperationendpoint.h"
#include "7zip/UI/Common/ArchiveCommandLine.h"
#include "7zip/UI/Common/ExitCode.h"
#include "Windows/FileName.h"
#include "operations/CompressCall.h"
#include "operations/qtoperationprotocol.h"
#include "rep_operation_merged.h"
#include "settings/RegistryUtils.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFileInfo>
#include <QRemoteObjectNode>
#include <QTimer>
#include <memory>

namespace {
void OperationOptions(const QtOperationJob &job, CArcCmdLineOptions &options) {
    // Parse the source command defaults, without putting selection in argv or
    // recreating Parse1/Parse2's initialization/platform policy in Qt glue.
    CArcCmdLineParser parser;
    UStringVector defaults;
    const bool extract = job.Operation == QtOperationJob::Command::Extract;
    const bool compress = job.Operation == QtOperationJob::Command::Compress;
    const bool hash = job.Operation == QtOperationJob::Command::Hash;
    const bool benchmark = job.Operation == QtOperationJob::Command::Benchmark;
    defaults.Add(benchmark ? L"b" : hash ? L"h" : compress ? L"a" : extract ? L"x" : L"t");
    if (!benchmark)
        defaults.Add(L"-an");
    else if (job.TotalMode)
        defaults.Add(L"-mm=*");
    if (job.HashMode)
        defaults.Add(L"-thash");
    if (hash && !job.HashMethod.isEmpty())
        defaults.Add(UString(L"-scrc") + UString(job.HashMethod.toStdWString().c_str()));
    if (compress) {
        defaults.Add(job.AddExtension ? L"-saa" : L"-sae");
        if (!job.ArchiveType.isEmpty())
            defaults.Add(UString(L"-t") + UString(job.ArchiveType.toStdWString().c_str()));
        if (job.EMail)
            defaults.Add(L"-seml.");
    }
    UString memorySwitch;
    AddLagePagesSwitch(memorySwitch, FmSettingsPath());
    memorySwitch.Trim();
    if (!memorySwitch.IsEmpty())
        defaults.Add(memorySwitch);
    if (job.YesToAll)
        defaults.Add(L"-y");
    parser.Parse1(defaults, options);
    parser.Parse2(options);
    if (benchmark)
        return;
    if (hash) {
        options.Censor.CensorPaths.Clear();
        options.Censor.Pairs.Clear();
        if (job.HashBasePrefix.isEmpty()) {
            for (const auto &path : job.Paths)
                options.Censor.AddPreItem_NoWildcard(UString(path.toStdWString().c_str()));
            options.Censor.AddPathsToCensor(NWildcard::k_RelatPath);
        }
        return;
    }
    if (compress) {
        options.ArchiveName = job.ArchivePath.toStdWString().c_str();
        options.ShowDialog = job.ShowDialog;
        options.Censor.CensorPaths.Clear();
        options.Censor.Pairs.Clear();
        for (const auto &path : job.Paths)
            options.Censor.AddPreItem_NoWildcard(UString(path.toStdWString().c_str()));
        // UpdateGUI needs the literal pre-items for ShowDialog. Common/Update
        // performs the source path conversion/enumeration in its worker.
        return;
    }
    if (extract) {
        options.ShowDialog = job.ShowDialog;
        options.ExtractOptions.OutputDir = us2fs(job.Destination.toStdWString().c_str());
#ifdef _WIN32
        NWindows::NFile::NName::NormalizeDirSeparators(options.ExtractOptions.OutputDir);
#endif
        NWindows::NFile::NName::NormalizeDirPathPrefix(options.ExtractOptions.OutputDir);
        if (job.ElimDup)
            options.ExtractOptions.ElimDup.Val = options.ExtractOptions.ElimDup.Def = true;
        if (job.WriteZone != quint32(-1))
            options.ExtractOptions.ZoneMode = NExtract::NZoneIdMode::EEnum(job.WriteZone);
    }
    // CompressCall2's literal archive selection. Main2 retains enumeration,
    // errors, callbacks, dialogs and the original test/exit policy.
    options.arcCensor.CensorPaths.Clear();
    options.arcCensor.Pairs.Clear();
    for (const auto &path : job.Paths)
        options.arcCensor.AddPreItem_NoWildcard(UString(path.toStdWString().c_str()));
    options.arcCensor.AddPathsToCensor(NWildcard::k_RelatPath);
    options.arcCensor.ExtendExclude();
}
QtOperationResult Result(int code) {
    using Outcome = QtOperationResult::Outcome;
    return {code == NExitCode::kSuccess     ? Outcome::Success
            : code == NExitCode::kWarning   ? Outcome::Warning
            : code == NExitCode::kUserBreak ? Outcome::Cancelled
                                            : Outcome::Failure,
            code,
            {}};
}
} // namespace

int RunGUIOperationEndpoint(QApplication &application, const QString &channel,
                            const QtGUIOperationRunner &run) {
    QRemoteObjectNode node;
    std::unique_ptr<OperationReplica> replica(node.acquire<OperationReplica>());
    QTimer startup;
    startup.setSingleShot(true);
    bool accepted = false;
    QObject context; // Disconnect stack callbacks before node/timer destruction.
    QObject::connect(&startup, &QTimer::timeout, &context,
                     [&] { application.exit(NExitCode::kUserError); });
    QObject::connect(replica.get(), &QRemoteObjectReplica::stateChanged, &context,
                     [&](QRemoteObjectReplica::State state, QRemoteObjectReplica::State) {
                         if (!accepted && (state == QRemoteObjectReplica::Suspect ||
                                           state == QRemoteObjectReplica::SignatureMismatch))
                             application.exit(NExitCode::kUserError);
                         // Losing a source after acceptance never cancels native work.
                     });
    QObject::connect(replica.get(), &QRemoteObjectReplica::initialized, &context, [&] {
        if (accepted)
            return;
        const auto request = OperationJobFromRequest(replica->request());
        const auto validated = ValidateOperationJob(request).transform([&] { return request; });
        if (!validated) {
            replica->reject(validated.error());
            application.exit(NExitCode::kUserError);
            return;
        }
        // Capture all request values before the explicit acceptance call. No
        // native operation depends on the parent's replica/property lifetime.
        accepted = true;
        startup.stop();
        replica->accept();
        QTimer::singleShot(0, &context, [&, job = *validated] {
            QStringList changedDirectories;
            QStringList changedArchives;
            QtGUICommandSource command;
            command.Configure = [&](CArcCmdLineOptions &options) {
                OperationOptions(job, options);
            };
            if (job.Operation == QtOperationJob::Command::Hash && !job.HashBasePrefix.isEmpty())
                command.FileHash =
                    QtGUIFileHashSelection{job.HashBasePrefix, job.Paths, job.HashEnterToDirs};
            const int code = run(command, [&](const QString &path, QtGUIOutputKind kind) {
                const QString directory =
                    kind == QtGUIOutputKind::Archive ? QFileInfo(path).absolutePath() : path;
                if (kind == QtGUIOutputKind::Archive && !changedArchives.contains(path))
                    changedArchives.append(path);
                if (!changedDirectories.contains(directory))
                    changedDirectories.append(directory);
                if (replica->isReplicaValid())
                    replica->outputs(changedDirectories, changedArchives);
            });
            if (replica->isReplicaValid()) {
                auto result = Result(code);
                result.ChangedDirectories = changedDirectories;
                result.ChangedArchives = changedArchives;
                auto completion = replica->complete(OperationReportFromResult(result));
                // Await delivery, not just a local socket write. A departed
                // manager cannot hold the independent child open indefinitely.
                completion.waitForFinished(1000);
            }
            application.exit(code);
        });
    });
    QTimer::singleShot(0, &context, [&] {
        startup.start(10000);
        if (!node.connectToNode(QUrl(QStringLiteral("local:") + channel)))
            application.exit(NExitCode::kUserError);
    });
    return application.exec();
}
