#include "window_state_support.h"

class PathHashBackend : public ControlledBackend {
  public:
    void hash(const QStringList &paths, const QString &method) override {
        hashPaths = paths;
        hashMethod = method;
        ++hashCalls;
        hashRunning = true;
    }
    bool busy() const override { return hashRunning || ControlledBackend::busy(); }
    void cancel() override {
        hashRunning = false;
        ControlledBackend::cancel();
    }
    void completeHash() {
        hashRunning = false;
        emit finished(true, QStringLiteral("Fixture hash complete."));
    }
    QStringList hashPaths;
    QString hashMethod;
    int hashCalls = 0;
    bool hashRunning = false;
};

class ArchiveSelectionRuntimeTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

    static void selectOnly(MainWindow &window, int row) {
        QVERIFY(row >= 0);
        auto *view = table(window);
        view->clearSelection();
        view->selectionModel()->select(view->model()->index(row, 0),
                                       QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }
    static void corruptPresentationPath(MainWindow &window, int row) {
        auto *item = table(window)->item(row, 0);
        item->setData(Qt::UserRole, QString("unrelated/display-only.txt"));
        item->setData(Qt::UserRole + 1, false);
        item->setData(Qt::UserRole + 4, 9999u);
        // The source item parameter (+6) and source folder remain untouched.
    }
    static void closeNextDialog(MainWindow &window, const QString &destination, bool accepted) {
        QTimer::singleShot(0, &window, [destination, accepted] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            if (auto *combo = dialog->findChild<QComboBox *>("extractDestination")) {
                dialog->findChild<QCheckBox *>("extractNameEnable")->setChecked(false);
                combo->setCurrentText(destination);
            }
            if (auto *combo = dialog->findChild<QComboBox *>("copyDestination"))
                combo->setCurrentText(destination);
            if (accepted)
                dialog->accept();
            else
                dialog->reject();
        });
    }

  private slots:
    void successfulHashUsesSourceLogicalSelection_data() {
        sourcePathsAcrossViewsAndConsumers_data();
    }
    void successfulHashUsesSourceLogicalSelection() {
        QFETCH(bool, flat);
        QFETCH(int, mode);
        PathHashBackend backend;
        MainWindow window(&backend);
        action(window, "flatViewAction")->setChecked(false);
        const QString directory = QStringLiteral("папка й с пробелом");
        openListedArchive(window, backend, "source-hash.zip",
                          {{"root.bin", 3, 1}, {directory + "/inside.txt", 7, 3}});
        action(window, "flatViewAction")->setChecked(flat);
        action(window, qPrintable(QString("viewMode%1Action").arg(mode)))->trigger();
        const int directoryRow = rowWithPath(window, directory);
        const int rootRow = rowWithPath(window, "root.bin");
        QVERIFY(directoryRow >= 0 && rootRow >= 0);
        selectOnly(window, directoryRow);
        table(window)->selectionModel()->select(table(window)->model()->index(rootRow, 0),
                                                QItemSelectionModel::Select |
                                                    QItemSelectionModel::Rows);
        corruptPresentationPath(window, directoryRow);
        corruptPresentationPath(window, rootRow);
        action(window, "hashSHA256Action")->trigger();
        QCOMPARE(backend.lastSelected, (QStringList{"root.bin", directory + "/inside.txt"}));
        const QDir extracted(backend.lastExtractDestination);
        QVERIFY(extracted.exists());
        backend.completeExtraction();
        QCOMPARE(backend.hashCalls, 1);
        QCOMPARE(backend.hashMethod, QString("SHA256"));
        QStringList expected = {extracted.filePath(directory), extracted.filePath("root.bin")};
        QStringList actual = backend.hashPaths;
        expected.sort();
        actual.sort();
        QCOMPARE(actual, expected);
        backend.completeHash();
        QVERIFY(!QFileInfo::exists(extracted.path()));
        QVERIFY(action(window, "hashSHA256Action")->isEnabled());
        action(window, "flatViewAction")->setChecked(false);
    }

    void renameUsesSourcePathAndOriginalNameHint() {
        ControlledBackend backend;
        MainWindow window(&backend);
        action(window, "flatViewAction")->setChecked(false);
        openListedArchive(window, backend, "source-rename.zip",
                          {{"a/same.txt", 3, 1}, {"b/same.txt", 7, 3}});
        action(window, "flatViewAction")->setChecked(true);
        const int row = rowWithPath(window, "b/same.txt");
        QVERIFY(row >= 0);
        selectOnly(window, row);
        corruptPresentationPath(window, row);
        QString hint;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = qobject_cast<QInputDialog *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            hint = dialog->textValue();
            dialog->setTextValue(QStringLiteral("новое й.txt"));
            dialog->accept();
        });
        action(window, "renameAction")->trigger();
        QCOMPARE(hint, QString("same.txt"));
        QCOMPARE(backend.renameCalls, 1);
        QCOMPARE(backend.lastRenameOld, QString("b/same.txt"));
        QCOMPARE(backend.lastRenameNew, QStringLiteral("b/новое й.txt"));
        backend.cancel();
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = qobject_cast<QInputDialog *>(QApplication::activeModalWidget());
            if (dialog)
                dialog->reject();
        });
        action(window, "renameAction")->trigger();
        QCOMPARE(backend.renameCalls, 1);
        action(window, "flatViewAction")->setChecked(false);
    }

    void sourcePathsAcrossViewsAndConsumers_data() {
        QTest::addColumn<bool>("flat");
        QTest::addColumn<int>("mode");
        for (bool flat : {false, true})
            for (int mode = 0; mode < 4; ++mode)
                QTest::newRow(qPrintable(QString("flat%1-mode%2").arg(flat).arg(mode)))
                    << flat << mode;
    }
    void sourcePathsAcrossViewsAndConsumers() {
        QFETCH(bool, flat);
        QFETCH(int, mode);
        QTemporaryDir destination;
        QVERIFY(destination.isValid());
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        action(window, "flatViewAction")->setChecked(false);
        const QString directory = QStringLiteral("папка й с пробелом");
        const QString first = directory + "/same.txt";
        const QString nested = directory + "/deep/same.txt";
        const QStringList expected = {"root.bin", first, nested};
        openListedArchive(
            window, backend, "owned-selection.zip",
            {{"root.bin", 11, 5}, {first, 13, 6}, {nested, 17, 8}, {"other/same.txt", 19, 9}});
        action(window, "flatViewAction")->setChecked(flat);
        action(window, qPrintable(QString("viewMode%1Action").arg(mode)))->trigger();
        const int directoryRow = rowWithPath(window, directory);
        const int rootRow = rowWithPath(window, "root.bin");
        QVERIFY(directoryRow >= 0 && rootRow >= 0);
        selectOnly(window, directoryRow);
        table(window)->selectionModel()->select(table(window)->model()->index(rootRow, 0),
                                                QItemSelectionModel::Select |
                                                    QItemSelectionModel::Rows);
        if (flat) {
            const int childRow = rowWithPath(window, nested);
            QVERIFY(childRow >= 0);
            table(window)->selectionModel()->select(table(window)->model()->index(childRow, 0),
                                                    QItemSelectionModel::Select |
                                                        QItemSelectionModel::Rows);
            corruptPresentationPath(window, childRow);
        }
        corruptPresentationPath(window, directoryRow);
        corruptPresentationPath(window, rootRow);
        window.extractTo(destination.path(), true);
        QCOMPARE(backend.extractCalls, 1);
        QCOMPARE(backend.lastSelected, expected); // Source archive order, overlap deduplicated.
        backend.cancel();
        QVERIFY(action(window, "copyAction")->isEnabled());
        closeNextDialog(window, destination.path(), false);
        action(window, "copyAction")->trigger();
        QCOMPARE(backend.extractCalls, 1);
        closeNextDialog(window, destination.path(), true);
        action(window, "copyAction")->trigger();
        QCOMPARE(backend.extractCalls, 2);
        QCOMPARE(backend.lastSelected, expected);
        QTimer::singleShot(0, &window, [] { dismissWarnings(); });
        backend.fail("fixture extraction failure");
        QVERIFY(action(window, "copyAction")->isEnabled());
        closeNextDialog(window, destination.path(), false);
        action(window, "moveAction")->trigger();
        QCOMPARE(backend.extractCalls, 2);
        closeNextDialog(window, destination.path(), true);
        action(window, "moveAction")->trigger();
        QCOMPARE(backend.extractCalls, 3);
        QCOMPARE(backend.lastSelected, expected);
        backend.cancel();
        QCOMPARE(backend.removeCalls, 0); // Cancelled move must not remove archive entries.
        action(window, "hashSHA256Action")->trigger();
        QCOMPARE(backend.extractCalls, 4);
        QCOMPARE(backend.lastSelected, expected);
        backend.cancel();
        QVERIFY(action(window, "hashSHA256Action")->isEnabled());
        action(window, "flatViewAction")->setChecked(false);
    }

    void flatDuplicateBasenamesUseTheSelectedSourceIndex() {
        ControlledBackend backend;
        MainWindow window(&backend);
        action(window, "flatViewAction")->setChecked(false);
        openListedArchive(window, backend, "duplicate-paths.zip",
                          {{"a/same.txt", 1, 1}, {"b/same.txt", 2, 2}});
        action(window, "flatViewAction")->setChecked(true);
        const int row = rowWithPath(window, "b/same.txt");
        QVERIFY(row >= 0);
        selectOnly(window, row);
        corruptPresentationPath(window, row);
        window.extractTo("/temporary/fixture destination", true);
        QCOMPARE(backend.lastSelected, QStringList{"b/same.txt"});
        backend.cancel();
        action(window, "flatViewAction")->setChecked(false);
    }
};

QTEST_MAIN(ArchiveSelectionRuntimeTests)
#include "archive_selection_runtime_tests.moc"
