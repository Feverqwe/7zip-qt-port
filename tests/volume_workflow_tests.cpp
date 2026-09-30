#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "CPP/7zip/UI/FileManager/SplitDialog.h"
#include "window_state_support.h"

class VolumeWorkflowTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static QByteArray read(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }
    static bool write(const QString &path, const QByteArray &bytes) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    }
    static void navigate(MainWindow &window, const QString &path) {
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(path);
        QTest::keyClick(address, Qt::Key_Return);
    }
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }

    void splitCreatesNewDestinationAndCombinesIntoAnother() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString name = QStringLiteral("исходник с пробелом.bin");
        const QByteArray bytes(75, 'x');
        QVERIFY(write(temp.filePath(name), bytes));
        const QString destination = temp.filePath(QStringLiteral("новая папка/части"));
        ControlledBackend backend;
        MainWindow window(&backend);
        navigate(window, temp.path());
        table(window)->selectRow(filesystemRow(window, name));
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<CSplitDialog *>("splitDialog");
            if (!dialog)
                return;
            dialog->findChild<QComboBox *>("volumeDestination")->setCurrentText(destination);
            dialog->findChild<QComboBox *>("volumeSize")->setCurrentText("10 20");
            dialog->accept();
        });
        action(window, "splitFileAction")->trigger();
        auto *splitProgress =
            dynamic_cast<CProgressDialog *>(window.findChild<QDialog *>("progressDialog"));
        QVERIFY(splitProgress);
        QVERIFY(!splitProgress->findChild<QPushButton *>("IDB_PAUSE")->isEnabled());
        QTRY_VERIFY(action(window, "createFolderAction")->isEnabled());
        QCOMPARE(QDir(destination).entryList(QDir::Files).size(), 5);
        QByteArray restored;
        for (int i = 1; i <= 5; ++i)
            restored +=
                read(QDir(destination)
                         .filePath(name + QStringLiteral(".%1").arg(i, 3, 10, QLatin1Char('0'))));
        QCOMPARE(restored, bytes);
        QCOMPARE(read(temp.filePath(name)), bytes);

        navigate(window, destination);
        table(window)->selectRow(filesystemRow(window, name + ".001"));
        const QString combined = temp.filePath(QStringLiteral("другая новая папка"));
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = window.findChild<QDialog *>("combineDialog");
            if (!dialog)
                return;
            dialog->findChild<QComboBox *>("copyDestination")->setCurrentText(combined);
            dialog->accept();
        });
        action(window, "combineFilesAction")->trigger();
        auto *combineProgress =
            dynamic_cast<CProgressDialog *>(window.findChild<QDialog *>("progressDialog"));
        QVERIFY(combineProgress);
        QVERIFY(!combineProgress->findChild<QPushButton *>("IDB_PAUSE")->isEnabled());
        QTRY_VERIFY(action(window, "createFolderAction")->isEnabled());
        QCOMPARE(read(QDir(combined).filePath(name)), bytes);
        QCOMPARE(QDir(destination).entryList(QDir::Files).size(), 5);
        QVERIFY(QDir(combined).entryList({"*.sevenzip-*"}, QDir::Files | QDir::Hidden).isEmpty());
    }

    void splitConfirmationAndCancel_data() {
        QTest::addColumn<int>("choice");
        QTest::newRow("cancel-dialog") << 0;
        QTest::newRow("decline-100-parts") << 1;
        QTest::newRow("confirm-100-parts") << 2;
        QTest::newRow("cancel-100-parts-confirmation") << 3;
    }
    void splitConfirmationAndCancel() {
        QFETCH(int, choice);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(write(temp.filePath("source.bin"), QByteArray(100, 'x')));
        const QString destination = temp.filePath(QStringLiteral("папка назначения"));
        ControlledBackend backend;
        MainWindow window(&backend);
        navigate(window, temp.path());
        table(window)->selectRow(filesystemRow(window, "source.bin"));
        bool dialogHandled = false;
        bool confirmationSeen = false;
        QTimer timer;
        connect(&timer, &QTimer::timeout, &window, [&] {
            if (!dialogHandled) {
                if (auto *dialog = window.findChild<CSplitDialog *>("splitDialog")) {
                    dialogHandled = true;
                    dialog->findChild<QComboBox *>("volumeDestination")
                        ->setCurrentText(destination);
                    dialog->findChild<QComboBox *>("volumeSize")->setCurrentText("1");
                    if (choice == 0)
                        dialog->reject();
                    else
                        dialog->accept();
                }
            }
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible()) {
                    confirmationSeen = box->text().contains("100");
                    box->button(choice == 2   ? QMessageBox::Yes
                                : choice == 3 ? QMessageBox::Cancel
                                              : QMessageBox::No)
                        ->click();
                }
        });
        timer.start(1);
        action(window, "splitFileAction")->trigger();
        timer.stop();
        QVERIFY(dialogHandled);
        QCOMPARE(confirmationSeen, choice != 0);
        QTRY_VERIFY(action(window, "createFolderAction")->isEnabled());
        QCOMPARE(QFileInfo::exists(destination), choice == 2);
        if (choice == 2)
            QCOMPARE(QDir(destination).entryList(QDir::Files).size(), 100);
        QCOMPARE(read(temp.filePath("source.bin")), QByteArray(100, 'x'));
    }

    void invalidDestinationReportsErrorWithoutChangingSource() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(write(temp.filePath("source.bin"), QByteArray(75, 'x')));
        const QString destination = temp.filePath("occupied");
        QVERIFY(write(destination, "keep"));
        ControlledBackend backend;
        MainWindow window(&backend);
        navigate(window, temp.path());
        table(window)->selectRow(filesystemRow(window, "source.bin"));
        bool submitted = false;
        bool errorSeen = false;
        QTimer timer;
        connect(&timer, &QTimer::timeout, &window, [&] {
            if (!submitted)
                if (auto *dialog = window.findChild<CSplitDialog *>("splitDialog")) {
                    submitted = true;
                    dialog->findChild<QComboBox *>("volumeDestination")
                        ->setCurrentText(destination);
                    dialog->findChild<QComboBox *>("volumeSize")->setCurrentText("10");
                    dialog->accept();
                }
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible()) {
                    errorSeen = true;
                    box->accept();
                }
        });
        timer.start(1);
        action(window, "splitFileAction")->trigger();
        timer.stop();
        QVERIFY(submitted && errorSeen);
        QCOMPARE(read(destination), QByteArray("keep"));
        QCOMPARE(read(temp.filePath("source.bin")), QByteArray(75, 'x'));
        QVERIFY(action(window, "createFolderAction")->isEnabled());
        QVERIFY(QDir(temp.path()).entryList({"*.001"}, QDir::Files).isEmpty());
    }
};
QTEST_MAIN(VolumeWorkflowTests)
#include "volume_workflow_tests.moc"
