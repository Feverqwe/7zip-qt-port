#include "qtnativemountjob.h"
#include <QCoreApplication>
#include <QRegularExpression>
#include <QRemoteObjectNode>
#include <QRemoteObjectPendingCallWatcher>
#include <QTimer>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.size() != 3 || args[1] != "--source")
        return 2;
    const QUrl endpoint(args[2]);
    if (endpoint.scheme() != "local" || !endpoint.authority().isEmpty())
        return 2;
    QRemoteObjectNode node;
    if (!node.connectToNode(endpoint))
        return 2;
    std::unique_ptr<NativeClientReplica> replica(node.acquire<NativeClientReplica>());
    QtNativeMountJob job;
    bool abandoned = false, connected = false;
    QTimer deadline;
    deadline.setSingleShot(true);
    QObject::connect(&deadline, &QTimer::timeout, &app, [&] { app.exit(3); });
    deadline.start(10000);
    QObject::connect(replica.get(), &QRemoteObjectReplica::initialized, &app, [&] {
        if (connected)
            return;
        connected = true;
        deadline.stop();
        QtNativeMountParameters parameters{replica->port(),  replica->share(),
                                           replica->user(),  replica->password(),
                                           replica->owner(), replica->directory()};
        const QRegularExpression component(QStringLiteral("^[A-Za-z0-9_-]+$"));
        if (parameters.Port < 1024 || parameters.Share.size() > 80 ||
            parameters.User.size() > 128 || parameters.Password.size() > 128 ||
            parameters.Owner.size() > 256 || parameters.Directory.size() > 32767 ||
            parameters.Directory.contains(QChar::Null) ||
            !component.match(parameters.Share).hasMatch() ||
            !component.match(parameters.User).hasMatch() ||
            !component.match(parameters.Password).hasMatch()) {
            app.exit(4);
            return;
        }
        // Initialization runs inside QtRO's receive dispatch. Let it return
        // before awaiting the acknowledgement on that same channel.
        deadline.start(10000);
        auto *ready = new QRemoteObjectPendingCallWatcher(replica->ready(), &app);
        QObject::connect(
            ready, &QRemoteObjectPendingCallWatcher::finished, &app, [&, ready, parameters] {
                deadline.stop();
                QRemoteObjectPendingReply<bool> reply(*ready);
                ready->deleteLater();
                if (reply.error() != QRemoteObjectPendingCall::NoError || !reply.returnValue()) {
                    app.exit(5);
                    return;
                }
                if (replica->recovery())
                    job.recover(parameters, endpoint, replica->force());
                else
                    job.mount(parameters, endpoint);
            });
    });
    QObject::connect(replica.get(), &NativeClientReplica::requestUnmount, &job,
                     &QtNativeMountJob::unmount);
    QObject::connect(replica.get(), &QRemoteObjectReplica::stateChanged, &app,
                     [&](QRemoteObjectReplica::State state, QRemoteObjectReplica::State) {
                         if (!connected || state == QRemoteObjectReplica::Valid)
                             return;
                         abandoned = true;
                         if (job.mounted() && !job.busy())
                             job.unmount(true);
                         else if (!job.busy())
                             app.quit();
                     });
    QObject::connect(&job, &QtNativeMountJob::mountFinished, &app,
                     [&](const QString &path, const QString &error) {
                         if (abandoned) {
                             if (job.mounted())
                                 job.unmount(true);
                             else
                                 app.quit();
                             return;
                         }
                         auto reply = replica->mounted(path, error);
                         reply.waitForFinished(5000);
                         if (!error.isEmpty())
                             app.quit();
                     });
    QObject::connect(&job, &QtNativeMountJob::unmountFinished, &app, [&](const QString &error) {
        if (!abandoned) {
            auto reply = replica->unmounted(error);
            reply.waitForFinished(5000);
        }
        if (error.isEmpty())
            app.quit();
        else if (abandoned)
            QTimer::singleShot(10000, &job, [&] { job.unmount(true); });
    });
    return app.exec();
}
