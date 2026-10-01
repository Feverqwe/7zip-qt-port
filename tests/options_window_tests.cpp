#include "window_state_support.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QSettings>

class OptionsWindowTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
  private slots:
    void applyRefreshesBothPanelsAndRetainsSelection() {
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QTemporaryDir fixture;
        const QString left = fixture.filePath("левая папка");
        const QString right = fixture.filePath("правая папка");
        QVERIFY(QDir().mkpath(left) && QDir().mkpath(right));
        auto write = [](const QString &path) {
            QFile file(path);
            return file.open(QIODevice::WriteOnly) && file.write("data") == 4;
        };
        QVERIFY(write(left + "/left.txt") && write(right + "/right.txt"));
        MainWindow window;
        window.show();
        auto navigate = [&](QWidget *panel, const QString &path) {
            auto *address = panel->findChild<QLineEdit *>("archivePath");
            address->setText(path);
            QTest::keyClick(address, Qt::Key_Return);
        };
        auto *leftPanel = window.findChild<QWidget *>("filePanel0");
        QVERIFY(leftPanel);
        navigate(leftPanel, left);
        auto *leftTable = leftPanel->findChild<QTableWidget *>("archiveTable");
        QTRY_COMPARE(leftTable->rowCount(), 1);
        leftTable->selectRow(0);
        leftTable->setCurrentCell(0, 0);
        window.findChild<QAction *>("twoPanelsAction")->trigger();
        auto *rightPanel = window.findChild<QWidget *>("filePanel1");
        QVERIFY(rightPanel);
        navigate(rightPanel, right);
        auto *rightTable = rightPanel->findChild<QTableWidget *>("archiveTable");
        QTRY_COMPARE(rightTable->rowCount(), 1);
        rightTable->selectRow(0);
        rightTable->setCurrentCell(0, 0);
        rightTable->setFocus();
        QApplication::processEvents();
        bool applied = false;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<QDialog *>("settingsDialog");
            QVERIFY(dialog);
            QVERIFY(write(left + "/new-left.txt") && write(right + "/new-right.txt"));
            dialog->findChild<QCheckBox *>("showDotsOption")->setChecked(true);
            dialog->findChild<QCheckBox *>("gridOption")->setChecked(true);
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply)->click();
            applied = true;
            dialog->findChild<QCheckBox *>("gridOption")->setChecked(false);
            dialog->reject();
        });
        window.findChild<QAction *>("settingsAction")->trigger();
        QVERIFY(applied);
        for (auto *table : {leftTable, rightTable}) {
            QTRY_COMPARE(table->rowCount(), 3);
            QCOMPARE(table->item(0, 0)->text(), QString(".."));
            QVERIFY(table->showGrid());
            QCOMPARE(table->selectionModel()->selectedRows().size(), 1);
        }
        QCOMPARE(leftTable->item(leftTable->currentRow(), 0)->text(), QString("left.txt"));
        QCOMPARE(rightTable->item(rightTable->currentRow(), 0)->text(), QString("right.txt"));
        QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"), QSettings::IniFormat);
        QVERIFY(settings.value("FM/ShowDots").toBool());
        QVERIFY(settings.value("FM/ShowGrid").toBool());
    }
};
QTEST_MAIN(OptionsWindowTests)
#include "options_window_tests.moc"
