#include "7zip/UI/FileManager/PropertyNameRes.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/ListViewDialog.h"
#include "archivecodeccatalog.h"
#include "window_state_support.h"
#ifdef Q_OS_WIN
#include "native_properties_fixture.h"
#endif

class FilesystemPathBackend : public ControlledBackend {
  public:
    void hash(const QStringList &paths, const QString &method) override {
        hashPaths = paths;
        hashMethod = method;
        ++hashCalls;
    }
    QStringList hashPaths;
    QString hashMethod;
    int hashCalls = 0;
};

class FilesystemSelectionRuntimeTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    quint32 moduleVersion_ = 0;

    static int diskRow(MainWindow &window, const QString &path) {
        for (int row = 0; row < table(window)->rowCount(); ++row)
            if (table(window)->item(row, 1)->text() == path)
                return row;
        return -1;
    }
    static void createFile(const QString &path) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("source fixture"), qint64(14));
    }

  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Source Add requires the official native module");
#else
        ArchiveCodecCatalog probe(QString::fromUtf8(SEVENZIP_CODEC_MODULE), 0);
        QCOMPARE(probe.result(), S_OK);
        moduleVersion_ = probe.version();
#endif
    }
    void createUnselectedFocusDoesNotBecomeSource_data() {
        sourcePathsAcrossViewsAndConsumers_data();
    }
    void createUnselectedFocusDoesNotBecomeSource() {
        QFETCH(bool, flat);
        QFETCH(int, mode);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QDir root(QFileInfo(temporary.path()).canonicalFilePath());
        const QString name = QStringLiteral("focused й с пробелом.txt");
        createFile(root.filePath(name));
        QTemporaryDir empty;
        QVERIFY(empty.isValid());
        ControlledBackend backend;
#ifdef SEVENZIP_CODEC_MODULE
        QCOMPARE(backend.loadCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE), moduleVersion_),
                 S_OK);
#endif
        MainWindow window(&backend);
        action(window, "flatViewAction")->setChecked(false);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(root.path());
        emit address->returnPressed();
        action(window, "flatViewAction")->setChecked(flat);
        action(window, qPrintable(QString("viewMode%1Action").arg(mode)))->trigger();
        QTRY_COMPARE(table(window)->rowCount(), 1);
        QTRY_VERIFY(table(window)->isEnabled());
        table(window)->clearSelection();
        table(window)->setCurrentCell(0, 0, QItemSelectionModel::NoUpdate);
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 0);
        table(window)->item(0, 1)->setText(root.filePath("forged-focused.txt"));
        table(window)->item(0, 0)->setData(Qt::UserRole + 1, true);
        QStringList sources;
        int errors = 0;
        auto cancelCreate = [&] {
            QTimer answer;
            connect(&answer, &QTimer::timeout, &window, [&] {
                auto *dialog =
                    qobject_cast<CreateArchiveDialog *>(QApplication::activeModalWidget());
                if (dialog) {
                    sources = dialog->sources();
                    for (QString &path : sources)
                        path = path.normalized(QString::NormalizationForm_C);
                    dialog->reject();
                } else if (auto *box =
                               qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                    ++errors;
                    box->accept();
                }
            });
            answer.start(1);
            action(window, "toolbarAddAction")->trigger();
        };
        cancelCreate();
        QVERIFY(sources.isEmpty());
        QCOMPARE(errors, 1);
        QCOMPARE(backend.createCalls, 0);
        table(window)->selectionModel()->select(table(window)->model()->index(0, 0),
                                                QItemSelectionModel::Select |
                                                    QItemSelectionModel::Rows);
        cancelCreate();
        QCOMPARE(sources, QStringList{root.filePath(name)});
        QCOMPARE(errors, 1);
        sources.clear();
        address->setText(empty.path());
        emit address->returnPressed();
        QTRY_COMPARE(table(window)->rowCount(), 0);
        QTRY_VERIFY(table(window)->isEnabled());
        cancelCreate();
        QVERIFY(sources.isEmpty());
        QCOMPARE(errors, 2);
        QCOMPARE(backend.createCalls, 0);
        action(window, "flatViewAction")->setChecked(false);
    }

    void sourcePathsAcrossViewsAndConsumers_data() {
        QTest::addColumn<bool>("flat");
        QTest::addColumn<int>("mode");
        for (bool flat : {false, true})
            for (int mode = 0; mode < 4; ++mode)
                QTest::newRow(qPrintable(QString("%1-%2").arg(flat ? "flat" : "normal").arg(mode)))
                    << flat << mode;
    }
    void sourcePathsAcrossViewsAndConsumers() {
        QFETCH(bool, flat);
        QFETCH(int, mode);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QDir root(QFileInfo(temporary.path()).canonicalFilePath());
        const QString directory = QStringLiteral("папка й с пробелом");
        const QString name =
            QFileInfo(temporary.path()).fileName() + QStringLiteral(" одинаковое й");
        QVERIFY(root.mkdir(directory));
        createFile(root.filePath(name));
        createFile(root.filePath(directory + '/' + name));

        FilesystemPathBackend backend;
#ifdef SEVENZIP_CODEC_MODULE
        QCOMPARE(backend.loadCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE), moduleVersion_),
                 S_OK);
#endif
        MainWindow window(&backend);
        window.show();
        action(window, "flatViewAction")->setChecked(false);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(root.path());
        emit address->returnPressed();
        action(window, "flatViewAction")->setChecked(flat);
        action(window, qPrintable(QString("viewMode%1Action").arg(mode)))->trigger();
        QTRY_COMPARE(table(window)->rowCount(), flat ? 3 : 2);
        QTRY_VERIFY(table(window)->isEnabled());
        table(window)->sortItems(0, Qt::DescendingOrder);
        const QString second = root.filePath(flat ? directory + '/' + name : directory);
        const int firstRow = diskRow(window, root.filePath(name));
        const int secondRow = diskRow(window, second);
        QVERIFY(firstRow >= 0 && secondRow >= 0);
        table(window)->clearSelection();
        for (int row : {firstRow, secondRow})
            table(window)->selectionModel()->select(table(window)->model()->index(row, 0),
                                                    QItemSelectionModel::Select |
                                                        QItemSelectionModel::Rows);
#ifdef Q_OS_WIN
        // A unique single-item caption identifies this fixture's native sheet.
        // Restore both source indices before testing Hash and Add consumers.
        table(window)->clearSelection();
        table(window)->selectRow(firstRow);
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        std::atomic<bool> stop{false};
        auto observer = std::async(std::launch::async, NativePropertiesFixture::ObserveAndClose,
                                   name, reinterpret_cast<HWND>(window.winId()), std::cref(stop));
        const auto finish = qScopeGuard([&] { stop = true; });
        action(window, "selectedPropertiesAction")->trigger();
        QTRY_VERIFY_WITH_TIMEOUT(
            observer.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready, 15000);
        const auto sheet = observer.get();
        QVERIFY2(sheet.window && sheet.tabs && sheet.visible, qPrintable(sheet.diagnostic));
        QTRY_VERIFY(!IsWindow(sheet.window));
        for (int row : {firstRow, secondRow})
            table(window)->selectionModel()->select(table(window)->model()->index(row, 0),
                                                    QItemSelectionModel::Select |
                                                        QItemSelectionModel::Rows);
#else
        // The portable Properties collector supplies a display prefix without
        // its native separator.
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            dialog->reject();
        });
        action(window, "selectedPropertiesAction")->trigger();
#endif
        for (int row : {firstRow, secondRow}) {
            table(window)->item(row, 1)->setText(root.filePath("forged/display-only.txt"));
            table(window)->item(row, 0)->setData(Qt::UserRole + 1, true);
        }
        QStringList expected = {root.filePath(name), second};
        expected.sort();
        QHash<QString, QString> hashResults;
        QTimer closeResults;
        connect(&closeResults, &QTimer::timeout, this, [&] {
            auto *dialog = qobject_cast<CListViewDialog *>(QApplication::activeModalWidget());
            if (!dialog)
                return;
            FOR_VECTOR(i, dialog->Strings)
            hashResults.insert(ToQString(dialog->Strings[i]), ToQString(dialog->Values[i]));
            dialog->accept();
        });
        closeResults.start(1);
        action(window, "hashSHA256Action")->trigger();
        closeResults.stop();
        // Forged display paths would fail to read. The source panel indices
        // must instead hash both real files, including ordinary folder children.
        QCOMPARE(hashResults.value(ToQString(LangString(IDS_PROP_FILES))), QString("2"));
        QVERIFY(hashResults.value(ToQString(LangString(IDS_PROP_SIZE))).startsWith("28"));
        QVERIFY(!hashResults.contains(ToQString(LangString(IDS_PROP_NUM_ERRORS))));
        QCOMPARE(backend.hashCalls, 0);
        QTRY_VERIFY(action(window, "toolbarAddAction")->isEnabled());

        QStringList dialogSources;
        QTimer::singleShot(0, &window, [&] {
            auto *dialog = qobject_cast<CreateArchiveDialog *>(QApplication::activeModalWidget());
            QVERIFY(dialog);
            dialogSources = dialog->sources();
            for (QString &path : dialogSources)
                path = path.normalized(QString::NormalizationForm_C);
            dialog->reject();
        });
        action(window, "toolbarAddAction")->trigger();
        dialogSources.sort();
        QCOMPARE(dialogSources, expected);
        QCOMPARE(backend.createCalls, 0);
        action(window, "flatViewAction")->setChecked(false);
    }
};

QTEST_MAIN(FilesystemSelectionRuntimeTests)
#include "filesystem_selection_runtime_tests.moc"
