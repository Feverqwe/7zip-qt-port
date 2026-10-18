#include "CPP/7zip/UI/FileManager/ListViewDialog.h"
#include "window_state_support.h"
#include <QElapsedTimer>
#include <QScopeGuard>
#include <QTextStream>

class LifecycleBackend : public ControlledBackend {
  public:
    void hash(const QStringList &paths, const QString &) override {
        hashPaths = paths;
        ++hashCalls;
    }
    QStringList hashPaths;
    int hashCalls = 0;
    void cancel() override {
        temporaryExistedOnCancel = QFileInfo::exists(observedTemporary);
        ControlledBackend::cancel();
    }
    QString observedTemporary;
    bool temporaryExistedOnCancel = false;
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
    void closingWindowWaitsForSourceHashBeforeReleasingOwner() {
        LifecycleBackend backend;
        MainWindow window(&backend);
        window.show();
        openListedArchive(window, backend, "close hash lifetime.zip",
                          {{QStringLiteral("имя й с пробелом.txt"), 6, 3}});
        table(window)->selectRow(0);
        bool closeWasDeferred = false;
        backend.queueSourceExtractAction([&] {
            backend.observedTemporary = QFileInfo(backend.lastExtractDestination).absolutePath();
            closeWasDeferred = !window.close();
            backend.cancel();
        });
        action(window, "hashSHA256Action")->trigger();
        QVERIFY(closeWasDeferred);
        QCOMPARE(backend.cancelCalls, 1);
        QVERIFY(backend.temporaryExistedOnCancel);
        QVERIFY(!backend.busy());
        QVERIFY(!QFileInfo::exists(backend.observedTemporary));
        QVERIFY(window.close());
        QCOMPARE(table(window)->rowCount(), 0);
        backend.completeList({{"late event.txt", 9, 5}});
        QCoreApplication::processEvents();
        QCOMPARE(backend.hashCalls, 0);
        QCOMPARE(backend.extractCalls, 1);
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(!window.isEnabled());
    }

    void transportCanStopImmediatelyAfterStart() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list(temporary.filePath("starting-process.zip"));
        QElapsedTimer elapsed;
        elapsed.start();
        backend.cancelAndWait();
        QVERIFY(elapsed.elapsed() < 5000);
        QVERIFY(!backend.busy());
        QCOMPARE(finished.size(), 1);
        QVERIFY(!finished.first().at(0).toBool());
    }

    void transportStopsBeforeCancelAndWaitReturns() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        ArchiveBackend backend(nullptr, QCoreApplication::applicationFilePath());
        QSignalSpy output(&backend, &ArchiveBackend::outputReceived);
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        for (int iteration = 0; iteration < 2; ++iteration) {
            const int previousOutput = output.size();
            backend.list(temporary.filePath("controlled-process.zip"));
            QTRY_VERIFY(output.size() > previousOutput);
            QVERIFY(backend.busy());
            QCOMPARE(output.last().at(0).toString(), QString("Fixture process ready."));
            QElapsedTimer elapsed;
            elapsed.start();
            backend.cancelAndWait();
            QVERIFY(elapsed.elapsed() < 5000);
            QVERIFY(!backend.busy());
            QCOMPARE(finished.size(), iteration + 1);
            QVERIFY(!finished.last().at(0).toBool());
            QVERIFY(!finished.last().at(1).toString().isEmpty());
            QCOMPARE(backend.lastResult(), E_ABORT);
            backend.cancelAndWait();
            QCOMPARE(finished.size(), iteration + 1);
        }
    }

    void destroyingWindowAfterSourceHashJoinsReleasesItsSpool() {
        LifecycleBackend backend;
        auto window = std::make_unique<MainWindow>(&backend);
        action(*window, "flatViewAction")->setChecked(false);
        openListedArchive(*window, backend, "owned-lifetime.zip",
                          {{QStringLiteral("имя й с пробелом.txt"), 6, 3}});
        table(*window)->selectRow(0);
        backend.queueSourceExtractAction([&] {
            backend.observedTemporary = QFileInfo(backend.lastExtractDestination).absolutePath();
            backend.cancel();
        });
        action(*window, "hashSHA256Action")->trigger();
        QVERIFY(!backend.busy());
        QVERIFY(!QFileInfo::exists(backend.observedTemporary));
        const QPointer<MainWindow> destroyed = window.get();
        window.reset();
        QVERIFY(destroyed.isNull());
        QCOMPARE(backend.cancelCalls, 1);
        QVERIFY(backend.temporaryExistedOnCancel);
        QVERIFY(!backend.busy());
        QVERIFY(!QFileInfo::exists(backend.observedTemporary));
        backend.completeList({{"late event.txt", 9, 5}});
        QCoreApplication::processEvents();
        QCOMPARE(backend.extractCalls, 1);
    }

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
        QVERIFY(backend.prepareArchiveFixture("first-owner.zip"));
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] { backend.completeList({{oldPath, 6, 3}}); });
            verifyLoadingCommands(window);
        });
        window.openArchive("first-owner.zip");
        action(window, "flatViewAction")->setChecked(flat);
        selectAndForge(window);
        action(window, "hashSHA256Action")->trigger();
        QCOMPARE(backend.lastSelected, QStringList{oldPath});
        const QString oldTemporary = backend.lastExtractDestination;
        QVERIFY(!QFileInfo::exists(oldTemporary));
        QVERIFY(action(window, "hashSHA256Action")->isEnabled());
        QVERIFY(action(window, "viewFileAction")->isEnabled());
        window.extractTo(destination.path(), true);
        QCOMPARE(backend.lastSelected, QStringList{oldPath});
        backend.cancel();

        const QString newPath = QStringLiteral("new й с пробелом.txt");
        QVERIFY(backend.prepareArchiveFixture("second-owner.zip"));
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] { backend.completeList({{newPath, 19, 7}}); });
            verifyLoadingCommands(window);
        });
        window.openArchive("second-owner.zip");
        QCOMPARE(table(window)->item(0, 2)->text(), QString("19"));
        selectAndForge(window);
        window.extractTo(destination.path(), true);
        QCOMPARE(backend.lastSelected, QStringList{newPath});
        backend.cancel();
        action(window, "hashSHA256Action")->trigger();
        QCOMPARE(backend.lastSelected, QStringList{newPath});
        const QString newTemporary = backend.lastExtractDestination;
        QVERIFY(!QFileInfo::exists(newTemporary));

        QVERIFY(backend.prepareArchiveFixture("failed-owner.zip"));
        QTimer warningResponder;
        connect(&warningResponder, &QTimer::timeout, &window, [] { dismissWarnings(); });
        warningResponder.start(1);
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] {
                if (cancel)
                    backend.cancel();
                else
                    backend.fail(QStringLiteral("Fixture listing failed."));
            });
            verifyLoadingCommands(window);
        });
        window.openArchive("failed-owner.zip");
        warningResponder.stop();
        QCOMPARE(window.findChild<QLineEdit *>("archivePath")->text(), root.path());
        action(window, "flatViewAction")->setChecked(flat);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        QTRY_VERIFY(table(window)->isEnabled());
        selectAndForge(window);
        const int requests = backend.extractCalls;
        bool resultsShown = false;
        QTimer closeResults;
        connect(&closeResults, &QTimer::timeout, &window, [&] {
            if (auto *dialog = qobject_cast<CListViewDialog *>(QApplication::activeModalWidget())) {
                resultsShown = true;
                dialog->accept();
            }
        });
        closeResults.start(1);
        action(window, "hashSHA256Action")->trigger();
        closeResults.stop();
        QVERIFY(resultsShown);
        QCOMPARE(backend.hashCalls, 0); // Filesystem CalculateCrc2 reads the bound source path.
        QCOMPARE(backend.extractCalls, requests);
        backend.cancel();
        action(window, "flatViewAction")->setChecked(false);
    }
};

int main(int argc, char **argv) {
    // Controlled process fixture: ArchiveBackend's list command invokes this
    // executable instead of 7zz. No shell, real archive or service is involved.
    if (argc > 1 && QByteArray(argv[1]) == "l") {
        QCoreApplication app(argc, argv);
        QTextStream output(stdout);
        output << "Fixture process ready.\n";
        output.flush();
        QTimer::singleShot(30000, &app, &QCoreApplication::quit);
        return app.exec();
    }
    QApplication app(argc, argv);
    BindingLifecycleTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "binding_lifecycle_tests.moc"
