#include "archivebackend.h"
#include "qtoperationlauncher.h"

QtOperationLauncher *ArchiveBackend::startGuiTest(const QStringList &archives, QString &error) {
    QtOperationMessage job;
    job.Type = QtOperationMessage::Kind::Job;
    job.Paths = archives;
    return QtOperationLauncher::Launch(QtOperationLauncher::GuiProgram(), job, error);
}

QtOperationLauncher *ArchiveBackend::startGuiExtract(const QStringList &archives,
                                                     const QString &destination, bool showDialog,
                                                     bool elimDup, quint32 writeZone,
                                                     QString &error) {
    QtOperationMessage job;
    job.Type = QtOperationMessage::Kind::Job;
    job.Operation = QtOperationMessage::Command::Extract;
    job.Paths = archives;
    job.Destination = destination;
    job.ShowDialog = showDialog;
    job.ElimDup = elimDup;
    job.WriteZone = writeZone;
    return QtOperationLauncher::Launch(QtOperationLauncher::GuiProgram(), job, error);
}
