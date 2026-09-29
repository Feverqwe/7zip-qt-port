#include "window_state_support.h"

class LifecycleBackend : public ControlledBackend {
  public:
    void hash(const QStringList &paths, const QString &) override {
        hashPaths = paths;
        ++hashCalls;
    }
    QStringList hashPaths;
    int hashCalls = 0;
};

class BindingLifecycleTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

    static void navigate(MainWindow &window, const QString &path) {
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(path);
        emit address->returnPressed();
    }
    static void selectAndForge(MainWindow &window) {
        auto *view = table(window);
        QCOMPARE(view->rowCount(), 1);
        view->selectRow(0);
        view->item(0, 0)->setData(Qt::UserRole, QString("forged/previous-owner.txt"));
        view->item(0, 0)->setData(Qt::UserRole + 1, true);
        view->item(0, 1)->setText("forged/previous-owner.txt");
    }
    static void verifyLoadingCommands(MainWindow &window) {
        QCOMPARE(table(window)->rowCount(), 0);
        for (const char *name : {"viewFileAction", "editFileAction", "selectedPropertiesAction",
                                 "hashSHA256Action", "renameAction", "copyAction"})
            QVERIFY(!action(window, name)->isEnabled());
    }

  private slots:
    void consumersRebindAfterReplacementAndFailedListing_data() {
        QTest::addColumn<bool>("flat");
        QTest::addColumn<int>("mode");
        QTest::addColumn<bool>("cancel");
        for (bool flat : {false, true})
            for (int mode = 0; mode < 4; ++mode)
                for (bool cancel : {false, true})
                    QTest::newRow(qPrintable(QString("%1-%2-%3")
                                                 .arg(flat ? "flat" : "normal")
                                                 .arg(mode)
                                                 .arg(cancel ? "cancel" : "failure")))
                        << flat << mode << cancel;
    }
    void consumersRebindAfterReplacementAndFailedListing() {
        QFETCH(bool, flat);
        QFETCH(int, mode);
        QFETCH(bool, cancel);
        QTemporaryDir disk;
        QTemporaryDir destination;
        QVERIFY(disk.isValid() && destination.isValid());
        const QDir root(QFileInfo(disk.path()).canonicalFilePath());
        const QString diskPath = root.filePath(QStringLiteral("disk й с пробелом.txt"));
        QFile file(diskPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("original"), qint64(8));
        file.close();
        LifecycleBackend backend;
        MainWindow window(&backend);
        action(window, "flatViewAction")->setChecked(false);
        navigate(window, root.path());
        action(window, qPrintable(QString("viewMode%1Action").arg(mode)))->trigger();
        const QString oldPath = QStringLiteral("old й с пробелом.txt");
        window.openArchive("first-owner.zip");
        verifyLoadingCommands(window);
        backend.completeList({{oldPath, 6, 3}});
        action(window, "flatViewAction")->setChecked(flat);
        selectAndForge(window);
        action(window, "hashSHA256Action")->trigger();
        QCOMPARE(backend.lastSelected, QStringList{oldPath});
        const QString oldTemporary = backend.lastExtractDestination;
        QVERIFY(QFileInfo::exists(oldTemporary));
        backend.cancel();
        QVERIFY(!QFileInfo::exists(oldTemporary));
        QVERIFY(action(window, "hashSHA256Action")->isEnabled());
        QVERIFY(action(window, "viewFileAction")->isEnabled());
        window.extractTo(destination.path(), true);
        QCOMPARE(backend.lastSelected, QStringList{oldPath});
        backend.cancel();

        const QString newPath = QStringLiteral("new й с пробелом.txt");
        window.openArchive("second-owner.zip");
        verifyLoadingCommands(window);
        backend.completeList({{newPath, 19, 7}});
        QCOMPARE(table(window)->item(0, 2)->text(), QString("19"));
        selectAndForge(window);
        window.extractTo(destination.path(), true);
        QCOMPARE(backend.lastSelected, QStringList{newPath});
        backend.cancel();
        action(window, "hashSHA256Action")->trigger();
        QCOMPARE(backend.lastSelected, QStringList{newPath});
        const QString newTemporary = backend.lastExtractDestination;
        QVERIFY(QFileInfo::exists(newTemporary));
        backend.cancel();
        QVERIFY(!QFileInfo::exists(newTemporary));

        window.openArchive("failed-owner.zip");
        verifyLoadingCommands(window);
        if (cancel)
            backend.cancel();
        else {
            QTimer::singleShot(0, &window, [] {
                if (auto *dialog = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
                    dialog->accept();
            });
            backend.fail(QStringLiteral("Fixture listing failed."));
        }
        QCOMPARE(window.findChild<QLineEdit *>("archivePath")->text(), root.path());
        action(window, "flatViewAction")->setChecked(flat);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        QTRY_VERIFY(table(window)->isEnabled());
        selectAndForge(window);
        const int requests = backend.extractCalls;
        action(window, "hashSHA256Action")->trigger();
        QCOMPARE(backend.hashCalls, 1);
        QCOMPARE(backend.hashPaths, QStringList{diskPath});
        QCOMPARE(backend.extractCalls, requests);
        backend.cancel();
        action(window, "flatViewAction")->setChecked(false);
    }
};

QTEST_MAIN(BindingLifecycleTests)
#include "binding_lifecycle_tests.moc"
