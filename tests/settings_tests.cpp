#include "mainwindow.h"

#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class SettingsTests : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }

    void optionsApplyCancelAndPersist() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString original = QDir::currentPath();
        QVERIFY(QDir::setCurrent(temporary.path()));
        QVERIFY(QDir().mkdir(QStringLiteral("папка с пробелом")));
        MainWindow window;
        window.show();
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *action = window.findChild<QAction *>("settingsAction");
        QVERIFY(table && action);
        QVERIFY(!table->showGrid());

        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("settingsDialog");
            QVERIFY(dialog);
            dialog->findChild<QCheckBox *>("gridOption")->setChecked(true);
            dialog->findChild<QCheckBox *>("singleClickOption")->setChecked(true);
            dialog->findChild<QCheckBox *>("realFileIconsOption")->setChecked(true);
            dialog->reject();
        });
        action->trigger();
        QVERIFY(!table->showGrid());

        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("settingsDialog");
            QVERIFY(dialog);
            dialog->findChild<QCheckBox *>("gridOption")->setChecked(true);
            dialog->findChild<QCheckBox *>("singleClickOption")->setChecked(true);
            dialog->findChild<QCheckBox *>("realFileIconsOption")->setChecked(true);
            dialog->accept();
        });
        action->trigger();
        QVERIFY(table->showGrid());
        QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"), QSettings::IniFormat);
        QVERIFY(settings.value("list/realFileIcons").toBool());
        QVERIFY(settings.value("list/grid").toBool());
        QVERIFY(settings.value("list/singleClick").toBool());

        int folderRow = -1;
        for (int row = 0; row < table->rowCount(); ++row)
            if (table->item(row, 0)->text() == QStringLiteral("папка с пробелом"))
                folderRow = row;
        QVERIFY(folderRow >= 0);
        QVERIFY(QMetaObject::invokeMethod(table, "cellClicked", Qt::DirectConnection,
                                          Q_ARG(int, folderRow), Q_ARG(int, 0)));
        QCOMPARE(
            window.findChild<QLineEdit *>("archivePath")->text(),
            QFileInfo(temporary.filePath(QStringLiteral("папка с пробелом"))).canonicalFilePath());

        MainWindow reopened;
        QVERIFY(reopened.findChild<QTableWidget *>("archiveTable")->showGrid());
        QVERIFY(QDir::setCurrent(original));
    }
};

QTEST_MAIN(SettingsTests)
#include "settings_tests.moc"
