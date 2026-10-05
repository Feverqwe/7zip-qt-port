#include "CPP/7zip/UI/FileManager/PasswordDialog.h"
#include "mainwindow.h"
#include "qtfilemanagerstartup.h"
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QTimer>
#include <cstdio>

int main(int argc, char **argv) {
    QApplication application(argc, argv);
    application.setApplicationName("Sevenzip File Manager entry probe");
    bool bound = false;
    bool refusal = false;
    bool password = false;
    bool timedOut = false;
    const QString expected = qEnvironmentVariable("SEVENZIP_PROBE_MEMBER");
    QTimer observer;
    QObject::connect(&observer, &QTimer::timeout, &application, [&] {
        if (auto *dialog = dynamic_cast<CPasswordDialog *>(QApplication::activeModalWidget())) {
            password = true;
            dialog->reject();
            return;
        }
        if (auto *dialog = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
            refusal = true;
            dialog->accept();
            return;
        }
        for (QWidget *widget : QApplication::topLevelWidgets()) {
            auto *window = dynamic_cast<MainWindow *>(widget);
            if (!window || !window->isVisible())
                continue;
            auto *table = window->findChild<QTableWidget *>("archiveTable");
            if (!table || !table->isEnabled())
                continue;
            for (int row = 0; row < table->rowCount(); ++row) {
                if (table->item(row, 0)->text() != expected)
                    continue;
                bound = true;
                window->close();
                return;
            }
        }
    });
    observer.start(5);
    QTimer::singleShot(30000, &application, [&] {
        timedOut = true;
        application.quit();
    });
    const int result = RunSevenzipFileManager(application);
    observer.stop();
    const QJsonObject report{{"bound", bound},
                             {"refusal", refusal},
                             {"password", password},
                             {"timeout", timedOut},
                             {"exitCode", result}};
    const QByteArray bytes = QJsonDocument(report).toJson(QJsonDocument::Compact);
    std::fwrite(bytes.constData(), 1, static_cast<size_t>(bytes.size()), stdout);
    return timedOut ? 1 : result;
}
