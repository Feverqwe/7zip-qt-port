#include "7zip/MyVersion.h"
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <cstdio>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (args.size() != 3 || args[1] != "i" || args[2] != "-sccUTF-8")
        return 64;
    const QString mode = QFileInfo(args[0]).baseName();
    if (mode.contains("bad-banner"))
        std::puts("7-Zip (z) " MY_VERSION "invalid");
    else if (mode.contains("wrong-version"))
        std::puts("7-Zip (z) 1.00 : fixture");
    else
        std::puts("7-Zip (z) " MY_VERSION " : fixture");
    std::fflush(stdout);
    if (mode.contains("delayed")) {
        QTimer::singleShot(100, &app, [&app] {
            QFile finished(QCoreApplication::applicationFilePath() + ".finished");
            if (!finished.open(QIODevice::WriteOnly) || finished.write("finished") != 8) {
                app.exit(74);
                return;
            }
            finished.close();
            app.quit();
        });
        return app.exec();
    }
    if (mode.contains("hang")) {
        QTimer::singleShot(10000, &app, [&app] { app.exit(1); });
        return app.exec();
    }
    return mode.contains("failed-exit") ? 2 : 0;
}
