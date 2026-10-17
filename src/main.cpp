#include "desktopidentity.h"
#include "mainwindow.h"
#include "qtfilemanagerstartup.h"
#include "qtguicommand.h"

#include <QApplication>
#include <QFileInfo>
#include <QIcon>
#include <QTimer>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/upstream/7zip/UI/FileManager/FM.ico")));
    app.setApplicationName("Sevenzip Desktop");
    app.setDesktopFileName(QString::fromLatin1(NDesktopIdentity::kFileName));
#ifdef Q_OS_WIN
    const QString guiName = QStringLiteral("7zG.exe");
    const QString entryName = QFileInfo(app.applicationFilePath()).fileName();
#else
    const QString guiName = QStringLiteral("7zG");
    // macOS resolves applicationFilePath through CFBundleExecutable, including
    // when the copied GUI entry is launched from the same bundle directory.
    const QString entryName = QFileInfo(app.arguments().value(0)).fileName();
#endif
    if (entryName.compare(guiName, Qt::CaseInsensitive) == 0)
        return RunSevenzipGUI(app);
    if (!app.arguments().contains("--smoke-test"))
        return RunSevenzipDesktop(app);
    MainWindow window;
    QTimer::singleShot(0, &app, &QCoreApplication::quit);
    return app.exec();
}
