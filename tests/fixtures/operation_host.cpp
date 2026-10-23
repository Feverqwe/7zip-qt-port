#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "archivebackend.h"
#include "archiveenginelocation.h"
#include "qtguicommand.h"
#include "qtoperationendpoint.h"
#include <QApplication>
#include <QDialog>
#include <QEventLoop>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTimer>

class Diagnostics final : public QObject {
    QPointer<ArchiveBackend> backend_;

  public:
    QStringList Messages;
    bool eventFilter(QObject *object, QEvent *event) override {
        if (auto *backend = qobject_cast<ArchiveBackend *>(object);
            backend && backend != backend_) {
            backend_ = backend;
            connect(backend, &ArchiveBackend::codecCatalogReady, this,
                    [this](bool ready, const QString &error) {
                        if (!ready)
                            Messages.append(error);
                    });
        }
        if (event->type() == QEvent::Hide)
            if (auto *dialog = dynamic_cast<CProgressDialog *>(object)) {
                NWindows::NSynchronization::CCriticalSectionLock lock(dialog->Sync._cs);
                for (unsigned i = 0; i < dialog->Sync.Messages.Size(); ++i)
                    Messages.append(QString::fromWCharArray(dialog->Sync.Messages[i]));
                Messages.append(
                    QString::fromWCharArray(dialog->Sync.FinalMessage.ErrorMessage.Message));
            }
        return false;
    }
};

// Fixture-only synchronization around the production endpoint and native GUI
// dispatcher. No test environment switches enter the installed executable.
int main(int argc, char **argv) {
    QApplication application(argc, argv);
    application.setQuitOnLastWindowClosed(false);
    const auto arguments = application.arguments();
    if (arguments.size() != 3 || arguments[1] != "--operation-channel")
        return 100;
    const QString barrier = qEnvironmentVariable("SEVENZIP_OPERATION_BARRIER");
    const QString report = qEnvironmentVariable("SEVENZIP_OPERATION_REPORT");
    Diagnostics diagnostics;
    application.installEventFilter(&diagnostics);
    QTimer dialogs;
    if (qEnvironmentVariableIsSet("SEVENZIP_OPERATION_CANCEL_PASSWORD")) {
        QObject::connect(&dialogs, &QTimer::timeout, &application, [] {
            for (auto *widget : QApplication::topLevelWidgets())
                if (widget->isVisible() && widget->objectName() == "passwordDialog")
                    if (auto *dialog = qobject_cast<QDialog *>(widget))
                        dialog->reject();
        });
        dialogs.start(10);
    }
    const int result =
        RunGUIOperationEndpoint(application, arguments[2], [&](const QtGUICommandSource &command) {
            if (!barrier.isEmpty()) {
                QEventLoop wait;
                QTimer poll;
                QObject::connect(&poll, &QTimer::timeout, &wait, [&] {
                    if (QFileInfo::exists(barrier))
                        wait.quit();
                });
                poll.start(10);
                QTimer::singleShot(15000, &wait, &QEventLoop::quit);
                wait.exec();
                if (!QFileInfo::exists(barrier))
                    return 101;
            }
            return RunSevenzipGUICommand(application, command);
        });
    if (!report.isEmpty()) {
        QSaveFile detail(report + ".diagnostic");
        if (detail.open(QIODevice::WriteOnly)) {
            detail.write(
                QJsonDocument(
                    QJsonObject{{"engine", FindArchiveEngine()},
                                {"archive", qEnvironmentVariable("SEVENZIP_OPERATION_ARCHIVE")},
                                {"exists", QFileInfo::exists(
                                               qEnvironmentVariable("SEVENZIP_OPERATION_ARCHIVE"))},
                                {"messages", diagnostics.Messages.join('\n')}})
                    .toJson());
            detail.commit();
        }
        QSaveFile output(report);
        if (!output.open(QIODevice::WriteOnly) || output.write(QByteArray::number(result)) < 0 ||
            !output.commit())
            return 102;
    }
    return result;
}
