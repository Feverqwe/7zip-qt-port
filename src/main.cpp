#include "mainwindow.h"

#include <QApplication>
#include <QTimer>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Sevenzip Desktop");
    MainWindow window;
    if (app.arguments().contains("--smoke-test")) {
        QTimer::singleShot(0, &app, &QCoreApplication::quit);
    } else {
        window.show();
        if (app.arguments().size() > 1)
            QTimer::singleShot(0, &window,
                               [&window, &app] { window.openArchive(app.arguments().at(1)); });
    }
    return app.exec();
}
