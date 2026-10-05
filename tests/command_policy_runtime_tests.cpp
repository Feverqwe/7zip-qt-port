#include "7zip/UI/FileManager/resource.h"
#include "window_state_support.h"

class CommandPolicyRuntimeTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
  private slots:
    void insideUsesFocusedFolderRatherThanAnotherSelectedFile() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString directory = temp.filePath(QStringLiteral("папка внутри"));
        QVERIFY(QDir().mkpath(directory));
        QFile file(temp.filePath("selected.txt"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        ControlledBackend backend;
        int opened = 0;
        MainWindow window(&backend, [&](const QUrl &) {
            ++opened;
            return true;
        });
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto *view = table(window);
        view->selectRow(filesystemRow(window, "selected.txt"));
        view->setCurrentCell(filesystemRow(window, QStringLiteral("папка внутри")), 0,
                             QItemSelectionModel::NoUpdate);
        auto *inside = action(window, "openInsideAction");
        QVERIFY(inside->isEnabled());
        inside->trigger();
        QCOMPARE(address->text(), QFileInfo(directory).canonicalFilePath());
        QVERIFY(QDir::fromNativeSeparators(window.windowTitle())
                    .startsWith(QFileInfo(directory).canonicalFilePath()));
        QCOMPARE(opened, 0);
        QCOMPARE(backend.listCalls, 0);
    }
    void typedReadOnlyAndEncryptedMetadataCannotBeBypassed_data() {
        QTest::addColumn<QString>("format");
        QTest::addColumn<bool>("encrypted");
        QTest::newRow("reader-without-writer") << QStringLiteral("rar") << false;
        QTest::newRow("encrypted-source-mutations") << QStringLiteral("7z") << true;
    }
    void typedReadOnlyAndEncryptedMetadataCannotBeBypassed() {
        QFETCH(QString, format);
        QFETCH(bool, encrypted);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        ControlledBackend backend;
        MainWindow window(&backend);
        const QString archive = temp.filePath("fixture.7z");
        QVERIFY(backend.prepareArchiveFixture(archive));
        backend.queueListAction([&] {
            ArchiveInfo info;
            info.format = format;
            info.encrypted = encrypted;
            backend.infoReady(info);
            backend.completeList({ArchiveEntry{"file.txt", 1, 1}});
        });
        window.openArchive(archive);
        table(window)->selectRow(0);
        QVERIFY(action(window, "editFileAction")->isEnabled());
        QVERIFY(!window.findChild<QAction *>("builtinTextEditAction"));
        for (const char *name : {"moveAction"}) {
            auto *command = action(window, name);
            QVERIFY(!command->isEnabled());
            command->setEnabled(true);
            command->trigger();
        }
        for (const char *name : {"toolbarAddAction"}) {
            auto *command = action(window, name);
            QCOMPARE(command->isEnabled(), encrypted);
            if (!encrypted) {
                command->setEnabled(true);
                command->trigger();
            }
        }
        for (const char *name : {"deleteAction", "renameAction", "createFolderAction"})
            QCOMPARE(action(window, name)->isEnabled(), encrypted);
        QCOMPARE(backend.removeCalls, 0);
        QCOMPARE(backend.renameCalls, 0);
        QCOMPARE(backend.addCalls, 0);
        QCOMPARE(backend.extractCalls, 0);
        QVERIFY(action(window, "copyAction")->isEnabled());
        QVERIFY(action(window, "selectedPropertiesAction")->isEnabled());
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
    }
    void selectedNativeIndicesDriveEveryExternalOpenAndKeepSelection() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QStringList paths;
        for (const QString &name : {QStringLiteral("Я файл.txt"), QStringLiteral("A file.txt")}) {
            const QString path = temp.filePath(name);
            QFile file(path);
            QVERIFY(file.open(QIODevice::WriteOnly));
            paths.append(
                QFileInfo(path).canonicalFilePath().normalized(QString::NormalizationForm_C));
        }
        ControlledBackend backend;
        QStringList opened;
        MainWindow window(&backend, [&](const QUrl &url) {
            opened.append(url.toLocalFile().normalized(QString::NormalizationForm_C));
            return true;
        });
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto *view = table(window);
        QCOMPARE(view->rowCount(), 2);
        view->selectAll();
        for (int row = 0; row < 2; ++row) {
            view->item(row, 0)->setText("forged name");
            view->item(row, 1)->setText("/missing/forged path");
        }
        auto *outside = action(window, "openOutsideAction");
        QCOMPARE(outside->property("nativeCommandID").toUInt(), unsigned(IDM_OPEN_OUTSIDE));
        outside->trigger();
        opened.sort();
        paths.sort();
        QCOMPARE(opened, paths);
        QCOMPARE(view->selectionModel()->selectedRows().size(), 2);
    }
    void staleEnabledActionsCannotBypassBusyOwnerOrClosedLifecycle() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        ControlledBackend backend;
        int opened = 0;
        MainWindow window(&backend, [&](const QUrl &) {
            ++opened;
            return true;
        });
        openListedArchive(window, backend, temp.filePath("busy архив.7z"),
                          {ArchiveEntry{"file.txt", 1, 1}});
        table(window)->selectRow(0);
        action(window, "testArchiveAction")->trigger();
        QCOMPARE(backend.testCalls, 1);
        for (const char *name : {"testArchiveAction", "openOutsideAction", "createFileAction",
                                 "refreshAction", "copyAction", "selectAllAction"}) {
            auto *command = action(window, name);
            QVERIFY(!command->isEnabled());
            command->setEnabled(true); // A stale queued trigger must recheck CApp.
            command->trigger();
        }
        QCOMPARE(backend.testCalls, 1);
        QCOMPARE(backend.listCalls, 1);
        QCOMPARE(opened, 0);
        QVERIFY(!window.findChild<QDialog *>("comboDialog"));
        backend.completeExtraction();
        auto *outside = action(window, "openOutsideAction");
        QVERIFY(outside->isEnabled());
        QVERIFY(window.close());
        outside->setEnabled(true);
        outside->trigger();
        QCOMPARE(opened, 0);
    }
};
QTEST_MAIN(CommandPolicyRuntimeTests)
#include "command_policy_runtime_tests.moc"
