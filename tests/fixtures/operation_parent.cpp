#include "operation_parent.h"
#include "qtoperationlauncher.h"
#include <QCoreApplication>
#include <QTimer>

int RunOperationParent(QCoreApplication &application, const QString &host) {
    QtOperationLauncher launcher;
    QtOperationMessage job;
    job.Type = QtOperationMessage::Kind::Job;
    job.Paths = {qEnvironmentVariable("SEVENZIP_OPERATION_ARCHIVE")};
    job.YesToAll = true;
    if (qEnvironmentVariableIsSet("SEVENZIP_OPERATION_PROBE_EXTRACT")) {
        job.Operation = QtOperationMessage::Command::Extract;
        job.Destination = qEnvironmentVariable("SEVENZIP_OPERATION_DESTINATION");
        job.ShowDialog = false;
    }
    QObject::connect(&launcher, &QtOperationLauncher::accepted, &application,
                     [&] { application.exit(0); });
    QObject::connect(&launcher, &QtOperationLauncher::finished, &application,
                     [&] { application.exit(103); });
    QTimer::singleShot(12000, &application, [&] { application.exit(104); });
    QString error;
    if (!launcher.start(host, job, error))
        return 105;
    return application.exec();
}
