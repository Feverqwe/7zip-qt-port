#include "desktopidentity.h"
#include "mainwindow.h"
#include "qtassociationfileopen.h"

#include <QApplication>
#include <QFileInfo>
#include <QTimer>
#ifdef Q_OS_WIN
#include "qtfilemanagerstartup.h"
#include "qtguicommand.h"
#endif

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Sevenzip Desktop");
    app.setDesktopFileName(QString::fromLatin1(NDesktopIdentity::kFileName));
#ifdef Q_OS_WIN
    if (QFileInfo(app.applicationFilePath())
            .fileName()
            .compare(QStringLiteral("7zG.exe"), Qt::CaseInsensitive) == 0)
        return RunSevenzipGUI(app);
    if (!app.arguments().contains("--smoke-test"))
        return RunSevenzipFileManager(app);
#endif
    QtAssociationFileOpen associatedFiles(app);
    MainWindow window;
    QObject::connect(&associatedFiles, &QtAssociationFileOpen::OpenRequested, &app,
                     [](const QString &path) {
                         // LaunchServices can target an already busy application.
                         // Match Windows' independent association launch by giving
                         // the request its own window/backend/operation lifetime.
                         auto *receiver = new MainWindow;
                         receiver->setAttribute(Qt::WA_DeleteOnClose);
                         receiver->openPath(path);
                         receiver->show();
                     });
    associatedFiles.Activate();
    if (app.arguments().contains("--smoke-test")) {
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
    } else {
        window.show();
        if (app.arguments().size() > 1)
            QTimer::singleShot(0, &window,
                               [&window, &app] { window.openPath(app.arguments().at(1)); });
    }
    return app.exec();
}
