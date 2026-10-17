#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QTimer>

namespace {
bool WriteStartupReport(bool timedOut) {
    const QString expected = qEnvironmentVariable("SEVENZIP_NATIVE_STARTUP_MEMBER");
    int windows = 0, archives = 0;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        auto *window = dynamic_cast<MainWindow *>(widget);
        if (!window || !window->isVisible())
            continue;
        ++windows;
        auto *table = window->findChild<QTableWidget *>("archiveTable");
        if (!expected.isEmpty() && table && table->rowCount() == 1 &&
            table->item(0, 0)->text() == expected)
            ++archives;
    }
    if (!timedOut && (windows == 0 || (!expected.isEmpty() && archives == 0)))
        return false;
    QFile report(qEnvironmentVariable("SEVENZIP_NATIVE_STARTUP_REPORT"));
    if (report.open(QIODevice::WriteOnly))
        report.write(
            QJsonDocument(QJsonObject{{"windows", windows},
                                      {"archives", archives},
                                      {"timedOut", timedOut},
                                      {"arguments", QJsonArray::fromStringList(qApp->arguments())},
                                      {"executable", qApp->applicationFilePath()}})
                .toJson());
    return true;
}

void ObserveDocumentStartup() {
    if (qEnvironmentVariableIsEmpty("SEVENZIP_NATIVE_STARTUP_REPORT"))
        return;
    auto *application = QCoreApplication::instance();
    auto *observer = new QTimer(application);
    QObject::connect(observer, &QTimer::timeout, application, [observer] {
        if (QThread::currentThread()->loopLevel() != 0 && WriteStartupReport(false)) {
            observer->stop();
            QCoreApplication::quit();
        }
    });
    observer->start(10);
    QTimer::singleShot(15000, application, [observer] {
        if (observer->isActive()) {
            WriteStartupReport(true);
            observer->stop();
            QCoreApplication::quit();
        }
    });
}
} // namespace

// Observe the real src/main.cpp entry, including its pre-desktop initialization.
Q_COREAPP_STARTUP_FUNCTION(ObserveDocumentStartup)
int SevenzipDesktopMain(int argc, char **argv);
int main(int argc, char **argv) { return SevenzipDesktopMain(argc, argv); }
