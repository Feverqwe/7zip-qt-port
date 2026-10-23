#include "qtoperationendpoint.h"
#include "7zip/UI/Common/ArchiveCommandLine.h"
#include "7zip/UI/Common/ExitCode.h"
#include "CPP/7zip/UI/Common/CompressCall.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "qtoperationprotocol.h"
#include <QApplication>
#include <QCoreApplication>
#include <QLocalSocket>
#include <QTimer>

namespace {
void TestOptions(const QtOperationMessage &job, CArcCmdLineOptions &options) {
    // Parse the source command defaults, without putting selection in argv or
    // recreating Parse1/Parse2's initialization/platform policy in Qt glue.
    CArcCmdLineParser parser;
    UStringVector defaults;
    defaults.Add(L"t");
    defaults.Add(L"-an");
    UString memorySwitch;
    AddLagePagesSwitch(memorySwitch, FmSettingsPath());
    memorySwitch.Trim();
    if (!memorySwitch.IsEmpty())
        defaults.Add(memorySwitch);
    if (job.YesToAll)
        defaults.Add(L"-y");
    parser.Parse1(defaults, options);
    parser.Parse2(options);
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
bool Send(QLocalSocket &socket, const QtOperationMessage &message) {
    QByteArray frame;
    QString error;
    if (!EncodeOperationMessage(message, frame, error))
        return false;
    if (socket.write(frame) != frame.size())
        return false;
    socket.flush();
    return true;
}
} // namespace

int RunGUIOperationEndpoint(QApplication &application, const QString &channel,
                            const QtGUIOperationRunner &run) {
    QLocalSocket socket;
    socket.setReadBufferSize(64 * 1024 * 1024 + 4);
    QtOperationMessageReader reader;
    QTimer startup;
    startup.setSingleShot(true);
    bool accepted = false;
    bool scheduled = false;
    QObject context; // Disconnect stack callbacks before socket/timer destruction.
    QObject::connect(&startup, &QTimer::timeout, &context,
                     [&] { application.exit(NExitCode::kUserError); });
    QObject::connect(&socket, &QLocalSocket::connected, &context, [&] {
        QtOperationMessage ready;
        Send(socket, ready);
    });
    QObject::connect(&socket, &QLocalSocket::disconnected, &context, [&] {
        if (!accepted)
            application.exit(NExitCode::kUserError);
        // An accepted job owns its request and source UI. Loss of the parent
        // is deliberately not an engine cancel or an application quit request.
    });
    QObject::connect(&socket, &QLocalSocket::errorOccurred, &context,
                     [&](QLocalSocket::LocalSocketError) {
                         if (!accepted)
                             application.exit(NExitCode::kUserError);
                     });
    QObject::connect(&socket, &QLocalSocket::readyRead, &context, [&] {
        for (;;) {
            QtOperationMessage job;
            QString error;
            const auto state = reader.read(socket, job, error);
            if (state == QtOperationMessageReader::State::Pending)
                return;
            if (state == QtOperationMessageReader::State::Error ||
                job.Type != QtOperationMessage::Kind::Job || scheduled) {
                if (!accepted) {
                    QtOperationMessage rejection;
                    rejection.Type = QtOperationMessage::Kind::Rejected;
                    rejection.Result.Error =
                        error.isEmpty() ? QCoreApplication::translate("QtOperation",
                                                                      "Expected one operation job.")
                                        : error;
                    Send(socket, rejection);
                    application.exit(NExitCode::kUserError);
                }
                socket.disconnectFromServer();
                return;
            }
            // Capture the complete request before acknowledgement. Test inputs
            // are ordinary filesystem sources; no parent-owned spool is used.
            scheduled = accepted = true;
            startup.stop();
            QtOperationMessage acknowledgement;
            acknowledgement.Type = QtOperationMessage::Kind::Accepted;
            Send(socket, acknowledgement);
            QTimer::singleShot(0, &context, [&, job] {
                const int code =
                    run([&](CArcCmdLineOptions &options) { TestOptions(job, options); });
                if (socket.state() == QLocalSocket::ConnectedState) {
                    QtOperationMessage result;
                    result.Type = QtOperationMessage::Kind::Result;
                    result.Result = Result(code);
                    Send(socket, result);
                    if (socket.bytesToWrite())
                        socket.waitForBytesWritten(1000);
                }
                application.exit(code);
            });
        }
    });
    QTimer::singleShot(0, &context, [&] {
        startup.start(10000);
        socket.connectToServer(channel, QIODevice::ReadWrite);
    });
    return application.exec();
}
