#include "app/desktopidentity.h"
#include "filemanager/qtfilemanagerstartup.h"
#include "filemanager/window/mainwindow.h"
#include "gui/qtguicommand.h"
#include "settings/RegistryUtils.h"

#include <QApplication>
#include <QFileInfo>
#include <QIcon>
#include <QStyleHints>
#include <QTimer>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(QStringLiteral(":/upstream/7zip/UI/FileManager/FM.ico")));
    app.setApplicationName("Sevenzip");
    app.setApplicationVersion(QStringLiteral(SEVENZIP_DESKTOP_DISPLAY_VERSION));
    app.setDesktopFileName(QString::fromLatin1(NDesktopIdentity::kFileName));
    CFmSettings settings;
    settings.Load(FmSettingsPath());
    app.styleHints()->setColorScheme(settings.ColorScheme);
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
