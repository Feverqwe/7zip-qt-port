#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "filesystemscanner.h"
#include "layout_inspection.h"
#include "window_state_support.h"

class FilesystemReloadTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static bool writeControl(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write("control") == 7;
    }
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void workerFailureRetainsSourceResult_data() {
        QTest::addColumn<unsigned>("sourceIndex");
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("flat");
        QTest::addColumn<bool>("reload");
        for (unsigned index : {0u, 1u})
            for (const QString language : {"-", "ru"})
                for (bool flat : {false, true})
                    for (bool reload : {false, true})
                        QTest::newRow(qPrintable(
                            QString("%1-%2-%3-%4").arg(index).arg(language).arg(flat).arg(reload)))
                            << index << language << flat << reload;
    }
    void workerFailureRetainsSourceResult() {
        QFETCH(unsigned, sourceIndex);
        QFETCH(QString, language);
        QFETCH(bool, flat);
        QFETCH(bool, reload);
        SaveRegLang(ToUString(language), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString small = fixture.filePath("small");
        const QString large = fixture.filePath("large");
        QVERIFY(QDir().mkpath(small) && QDir().mkpath(large));
        QVERIFY(writeControl(QDir(small).filePath(QStringLiteral("seed 文件.txt"))));
        for (int i = 0; i < 301; ++i)
            QVERIFY(
                writeControl(QDir(large).filePath(QStringLiteral("control 文件 %1.txt").arg(i))));
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *ui = window.findChild<QWidget *>(QString("filePanel%1").arg(sourceIndex));
        auto *address = ui->findChild<QLineEdit *>("archivePath");
        auto *view = static_cast<FileTable *>(ui->findChild<QTableWidget *>("archiveTable"));
        address->setFocus();
        address->setText(small);
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_COMPARE(view->rowCount(), 1);
        if (flat) {
            action(window, "flatViewAction")->trigger();
            QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        }
        if (reload) {
            address->setText(large);
            QTest::keyClick(address, Qt::Key_Return);
            QTRY_COMPARE(view->rowCount(), 301);
            QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        }
        const auto scanners = window.findChildren<FilesystemScanner *>();
        QCOMPARE(scanners.size(), 2);
        auto *scanner = scanners[sourceIndex];
        QSignalSpy failed(scanner, &FilesystemScanner::failed);
        QSignalSpy completed(scanner, &FilesystemScanner::completed);
        auto &panel = *view->sourcePanel;
        CMyComPtr<IFolderFolder> retained;
        bool restarted = false;
        const auto restart = connect(scanner, &FilesystemScanner::advanced, &window, [&](quint64) {
            if (!restarted) {
                restarted = true;
                retained = panel.GetFolder();
                scanner->start(fixture.filePath(QStringLiteral("missing 文件")), flat);
            }
        });
        QString shown;
        QTimer driver;
        connect(&driver, &QTimer::timeout, &window, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible()) {
                    shown = box->text();
                    for (const QSize size : {QSize(440, 250), QSize(360, 200)}) {
                        box->resize(size);
                        box->layout()->activate();
                        QVERIFY(
                            UiInspection::writeReport(*box, QString("filesystem-worker-error-%1-%2")
                                                                .arg(QTest::currentDataTag())
                                                                .arg(size.width())));
                    }
                    box->accept();
                }
        });
        driver.start(0);
        if (reload)
            action(window, "refreshAction")->trigger();
        else {
            address->setText(large);
            QTest::keyClick(address, Qt::Key_Return);
        }
        QTRY_COMPARE(failed.size(), 1);
        driver.stop();
        QVERIFY(restarted);
        const HRESULT result = HRESULT(failed.first().first().toInt());
        QVERIFY(result != S_OK);
        QCOMPARE(shown, ToQString(HResultToMessage(result)));
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        QCOMPARE(panel.GetFolder(), (IFolderFolder *)retained);
        QVERIFY(panel.IsFSFolder());
        QCOMPARE(view->rowCount(), 0);
        CSelectedState selection;
        QCOMPARE(panel.RefreshListCtrl(selection), result);
        QVERIFY(panel.ShouldProcessNotifications());
        QVERIFY(completed.isEmpty());
        QVERIFY(!scanner->isRunning());
        disconnect(restart);
        panel.OnReload();
        QTRY_COMPARE(view->rowCount(), 301);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        QCOMPARE(completed.size(), 1);
        QCOMPARE(failed.size(), 1);
        QFile control(QDir(large).filePath(QStringLiteral("control 文件 0.txt")));
        QVERIFY(control.open(QIODevice::ReadOnly));
        QCOMPARE(control.readAll(), QByteArray("control"));
        QVERIFY(panel.ShouldProcessNotifications());
    }
};

QTEST_MAIN(FilesystemReloadTests)
#include "filesystem_reload_tests.moc"
