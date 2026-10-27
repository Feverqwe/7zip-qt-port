#pragma once

#include "archivebackend.h"
#include "qtoperationlauncher.h"
#include "scoped_environment.h"

class DropBackend final : public ArchiveBackend {
  public:
    QList<QtOperationResult> Results;
    QString Barrier;
    QString Program = QStringLiteral(SEVENZIP_OPERATION_HOST);
    quint64 ProcessId = 0;
    bool Cancel = false;
    bool Fail = false;
    unsigned Accepted = 0;
    QtOperationLauncher *startGuiCompress(const QString &archive, const QString &type,
                                          bool addExtension, const QStringList &paths, bool email,
                                          bool showDialog, QString &error) override {
        QtOperationJob job;
        job.Operation = QtOperationJob::Command::Compress;
        job.ArchivePath = archive;
        job.ArchiveType = Fail ? QStringLiteral("unsupported-drag-format") : type;
        job.AddExtension = addExtension;
        job.Paths = paths;
        job.EMail = email;
        job.ShowDialog = showDialog;
        // Verify source failure without waiting for a native Windows question.
        job.YesToAll = Fail;
        ScopedTestEnvironment decision("SEVENZIP_OPERATION_CREATE_DECISION",
                                       Cancel ? "cancel" : "accept");
        ScopedTestEnvironment barrier("SEVENZIP_OPERATION_BARRIER", Barrier);
        auto *operation = QtOperationLauncher::Launch(Program, job, error);
        if (operation) {
            ProcessId = operation->processId();
            connect(operation, &QtOperationLauncher::accepted, this, [this] { ++Accepted; });
            connect(operation, &QtOperationLauncher::finished, this,
                    [this](const QtOperationResult &result) { Results.append(result); });
        }
        return operation;
    }
};
