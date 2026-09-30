#include "window_state_support.h"
#include <QScopeGuard>
#include <QTreeWidget>

class MainWindowStateTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

  private slots:
    void initTestCase() {
        QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"), QSettings::IniFormat);
        settings.clear();
    }

    void numericNameSortMatchesFileManager() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        for (const QString &name : {"10.txt", "2.txt", "1.txt"}) {
            QFile file(temp.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto *view = table(window);
        QCOMPARE(view->item(0, 0)->text(), QStringLiteral("1.txt"));
        QCOMPARE(view->item(1, 0)->text(), QStringLiteral("2.txt"));
        QCOMPARE(view->item(2, 0)->text(), QStringLiteral("10.txt"));

        openListedArchive(window, backend, temp.filePath("test.7z"),
                          {ArchiveEntry{"10.txt", 10, 10}, ArchiveEntry{"2.txt", 2, 2},
                           ArchiveEntry{"1.txt", 1, 1}});
        QCOMPARE(view->item(0, 0)->text(), QStringLiteral("1.txt"));
        QCOMPARE(view->item(1, 0)->text(), QStringLiteral("2.txt"));
        QCOMPARE(view->item(2, 0)->text(), QStringLiteral("10.txt"));
    }

    void viewSortCommandsReorderArchiveAndSurviveRefresh() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        ControlledBackend backend;
        MainWindow window(&backend);
        const QList<ArchiveEntry> entries = {ArchiveEntry{"z.bin", 30, 1, "2024-01-01 00:00:00"},
                                             ArchiveEntry{"a.txt", 10, 1, "2023-01-01 00:00:00"},
                                             ArchiveEntry{"c.bin", 20, 1, "2022-01-01 00:00:00"}};
        openListedArchive(window, backend, temp.filePath("sort.7z"), entries);
        auto names = [&] {
            QStringList result;
            for (int row = 0; row < table(window)->rowCount(); ++row)
                result << table(window)->item(row, 0)->text();
            return result;
        };
        QCOMPARE(names(), (QStringList{"a.txt", "c.bin", "z.bin"}));
        action(window, "sort1Action")->trigger();
        QCOMPARE(names(), (QStringList{"c.bin", "z.bin", "a.txt"}));
        action(window, "sort2Action")->trigger();
        QCOMPARE(names(), (QStringList{"z.bin", "a.txt", "c.bin"}));
        action(window, "sort3Action")->trigger();
        QCOMPARE(names(), (QStringList{"z.bin", "c.bin", "a.txt"}));
        action(window, "sort4Action")->trigger();
        QCOMPARE(names(), (QStringList{"z.bin", "a.txt", "c.bin"}));
        backend.queueList(entries);
        action(window, "refreshAction")->trigger();
        QCOMPARE(names(), (QStringList{"z.bin", "a.txt", "c.bin"}));
        QVERIFY(action(window, "sort4Action")->isChecked());
    }

    void createFileDialogCreatesUnicodeFileAndCancelLeavesDirectoryUntouched() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        const QString name = QStringLiteral("новый файл.txt");
        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("comboDialog");
            QVERIFY(dialog);
            dialog->findChild<QComboBox *>("comboValue")->setEditText(name);
            dialog->accept();
        });
        action(window, "createFileAction")->trigger();
        QVERIFY(QFileInfo::exists(temp.filePath(name)));
        QCOMPARE(filesystemRow(window, name) >= 0, true);
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);
        {
            QFile existing(temp.filePath(name));
            QVERIFY(existing.open(QIODevice::WriteOnly));
            QCOMPARE(existing.write("protected"), 9);
        }
        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("comboDialog");
            QVERIFY(dialog);
            dialog->findChild<QComboBox *>("comboValue")->setEditText("cancelled.txt");
            dialog->reject();
        });
        action(window, "createFileAction")->trigger();
        QVERIFY(!QFileInfo::exists(temp.filePath("cancelled.txt")));
        QTimer warningTimer;
        connect(&warningTimer, &QTimer::timeout, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible())
                    box->accept();
        });
        warningTimer.start(1);
        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("comboDialog");
            QVERIFY(dialog);
            dialog->findChild<QComboBox *>("comboValue")->setEditText(name);
            dialog->accept();
        });
        action(window, "createFileAction")->trigger();
        warningTimer.stop();
        QFile existing(temp.filePath(name));
        QVERIFY(existing.open(QIODevice::ReadOnly));
        QCOMPARE(existing.readAll(), QByteArray("protected"));
    }

    void maskAndTypeSelectionFollowFileManager() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        for (const QString &name : {QStringLiteral("Alpha.txt"), QStringLiteral("beta.TXT"),
                                    QStringLiteral("image.png")}) {
            QFile file(temp.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto applyMask = [&](const char *actionName, const QString &mask) {
            QTimer::singleShot(0, [&] {
                auto *dialog = window.findChild<QDialog *>("comboDialog");
                QVERIFY(dialog);
                dialog->findChild<QComboBox *>("comboValue")->setEditText(mask);
                dialog->accept();
            });
            action(window, actionName)->trigger();
        };
        applyMask("selectMaskAction", "*.txt");
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 2);
        applyMask("deselectMaskAction", "alpha.*");
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);
        const int focusedBeforeCancel = table(window)->currentRow();
        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("comboDialog");
            QVERIFY(dialog);
            dialog->findChild<QComboBox *>("comboValue")->setEditText("*");
            dialog->reject();
        });
        action(window, "selectMaskAction")->trigger();
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(table(window)->currentRow(), focusedBeforeCancel);
        table(window)->selectionModel()->setCurrentIndex(
            table(window)->model()->index(filesystemRow(window, "image.png"), 0),
            QItemSelectionModel::NoUpdate);
        action(window, "selectTypeAction")->trigger();
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 2);
        action(window, "deselectTypeAction")->trigger();
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);

        openListedArchive(window, backend, temp.filePath("test.7z"),
                          {ArchiveEntry{"first.txt", 1, 1}, ArchiveEntry{"second.TXT", 1, 1},
                           ArchiveEntry{"other.bin", 1, 1}});
        applyMask("selectMaskAction", "*.txt");
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 2);
        QVERIFY(!action(window, "createFileAction")->isEnabled());
    }

    void foldersHistoryDialogPersistsAndDeletesEntries() {
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString first = temp.filePath("first folder");
        const QString second = temp.filePath(QStringLiteral("вторая папка"));
        QVERIFY(QDir().mkpath(first));
        QVERIFY(QDir().mkpath(second));
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(first);
        QTest::keyClick(address, Qt::Key_Return);
        address->setText(second);
        QTest::keyClick(address, Qt::Key_Return);
        const QString canonicalFirst = QFileInfo(first).canonicalFilePath();
        const QString canonicalSecond = QFileInfo(second).canonicalFilePath();
        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("foldersHistoryDialog");
            QVERIFY(dialog);
            auto *list = dialog->findChild<QTreeWidget *>("foldersHistoryList");
            QCOMPARE(list->topLevelItem(0)->text(0), canonicalSecond);
            QCOMPARE(list->topLevelItem(1)->text(0), canonicalFirst);
            list->setCurrentItem(list->topLevelItem(0));
            QTest::keyClick(list, Qt::Key_Delete);
            list->setCurrentItem(list->topLevelItem(0));
            dialog->accept();
        });
        action(window, "foldersHistoryAction")->trigger();
        QCOMPARE(address->text(), canonicalFirst);
        MainWindow reopened(&backend);
        auto *combo = reopened.findChild<QComboBox *>("pathCombo");
        QVERIFY(combo->findText(canonicalFirst) >= 0);
        QCOMPARE(combo->findText(canonicalSecond), -1);
    }

    void favoritesFollowFoldersAndArchivePaths() {
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString first = temp.filePath(QStringLiteral("папка с пробелом"));
        const QString second = temp.filePath("other");
        QVERIFY(QDir().mkpath(first));
        QVERIFY(QDir().mkpath(second));
        const QString archive = temp.filePath(QStringLiteral("архив с пробелом.7z"));
        QFile file(archive);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();

        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        auto *view = table(window);
        address->setText(first);
        QTest::keyClick(address, Qt::Key_Return);
        QTest::keyClick(view, Qt::Key_1, Qt::AltModifier | Qt::ShiftModifier);
        QCOMPARE(action(window, "openBookmark1Action")->isEnabled(), true);
        address->setText(second);
        QTest::keyClick(address, Qt::Key_Return);
        QTest::keyClick(view, Qt::Key_1, Qt::AltModifier);
        QCOMPARE(address->text(), QFileInfo(first).canonicalFilePath());

        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] {
                backend.completeList({ArchiveEntry{QStringLiteral("внутри/файл.txt"), 1, 1}});
            });
            QVERIFY(!action(window, "openBookmark1Action")->isEnabled());
        });
        window.openArchive(archive);
        openRow(window, rowWithPath(window, QStringLiteral("внутри")));
        action(window, "saveBookmark2Action")->trigger();
        action(window, "openBookmark1Action")->trigger();
        QCOMPARE(address->text(), QFileInfo(first).canonicalFilePath());
        backend.queueList({ArchiveEntry{QStringLiteral("внутри/файл.txt"), 1, 1}});
        action(window, "openBookmark2Action")->trigger();
        QCOMPARE(backend.listCalls, 2);
        QVERIFY(address->text().endsWith(QStringLiteral(" / внутри")));
        QCOMPARE(view->rowCount(), 1);

        MainWindow reopened;
        QVERIFY(action(reopened, "openBookmark1Action")->isEnabled());
        QVERIFY(action(reopened, "openBookmark2Action")->isEnabled());
        QFile::remove(archive);
        action(reopened, "openBookmark2Action")->trigger();
        QCOMPARE(reopened.findChild<QLineEdit *>("archivePath")->text(),
                 QFileInfo(temp.path()).canonicalFilePath());
    }

    void mainWindowFolderAndSelectionCommands() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString first = temp.filePath(QStringLiteral("файл с пробелом.txt"));
        QFile file(first);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        QCOMPARE(table(window)->rowCount(), 1);
        auto *createFolder = action(window, "createFolderAction");
        auto *refresh = action(window, "refreshAction");
        QVERIFY(createFolder->isEnabled());
        QVERIFY(refresh->isEnabled());

        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<CComboDialog *>(widget))
                    dialog->reject();
            }
        });
        createFolder->trigger();
        QCOMPARE(table(window)->rowCount(), 1);
        QTimer::singleShot(0, &window, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *dialog = qobject_cast<CComboDialog *>(widget)) {
                    dialog->findChild<QComboBox *>("comboValue")
                        ->setCurrentText(QStringLiteral("папка с пробелом"));
                    dialog->accept();
                }
            }
        });
        createFolder->trigger();
        const QString folder = temp.filePath(QStringLiteral("папка с пробелом"));
        QVERIFY(QFileInfo(folder).isDir());
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(table(window)
                     ->item(table(window)->selectionModel()->selectedRows().first().row(), 0)
                     ->text(),
                 QStringLiteral("папка с пробелом"));

        action(window, "invertSelectionAction")->trigger();
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(table(window)
                     ->item(table(window)->selectionModel()->selectedRows().first().row(), 0)
                     ->text(),
                 QStringLiteral("файл с пробелом.txt"));
        action(window, "deselectAllAction")->trigger();
        QVERIFY(table(window)->selectionModel()->selectedRows().isEmpty());
        table(window)->selectRow(filesystemRow(window, QStringLiteral("файл с пробелом.txt")));
        const QString later = temp.filePath("later.txt");
        QFile newFile(later);
        QVERIFY(newFile.open(QIODevice::WriteOnly));
        newFile.close();
        refresh->trigger();
        QCOMPARE(table(window)->rowCount(), 3);
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(table(window)
                     ->item(table(window)->selectionModel()->selectedRows().first().row(), 0)
                     ->text(),
                 QStringLiteral("файл с пробелом.txt"));
        QCOMPARE(backend.listCalls, 0);
    }

    void refreshArchiveKeepsFolderAndSelection() {
        ControlledBackend backend;
        MainWindow window(&backend);
        ArchiveEntry first;
        first.path = QStringLiteral("папка с пробелом/первый.txt");
        openListedArchive(window, backend, "example.7z", {first});
        openRow(window, rowWithPath(window, QStringLiteral("папка с пробелом")));
        QCOMPARE(table(window)->rowCount(), 1);
        table(window)->selectRow(0);
        ArchiveEntry second;
        second.path = QStringLiteral("папка с пробелом/второй.txt");
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] { backend.completeList({first, second}); });
            QCOMPARE(backend.listCalls, 2);
            QVERIFY(!action(window, "refreshAction")->isEnabled());
            QVERIFY(!action(window, "createFolderAction")->isEnabled());
            action(window, "refreshAction")->trigger();
            QCOMPARE(backend.listCalls, 2);
        });
        action(window, "refreshAction")->trigger();
        QCOMPARE(table(window)->rowCount(), 2);
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(table(window)
                     ->item(table(window)->selectionModel()->selectedRows().first().row(), 0)
                     ->text(),
                 QStringLiteral("первый.txt"));
        QVERIFY(window.findChild<QLineEdit *>("archivePath")
                    ->text()
                    .endsWith(QStringLiteral(" / папка с пробелом")));
        QVERIFY(action(window, "refreshAction")->isEnabled());
        backend.queueList({});
        action(window, "refreshAction")->trigger();
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(!window.findChild<QLineEdit *>("archivePath")
                     ->text()
                     .endsWith(QStringLiteral(" / папка с пробелом")));
        backend.queueListAction([&] { backend.cancel(); });
        action(window, "refreshAction")->trigger();
        QVERIFY(action(window, "createFolderAction")->isEnabled());
        QVERIFY(action(window, "refreshAction")->isEnabled());
        QVERIFY(!action(window, "testArchiveAction")->isEnabled());

        openListedArchive(window, backend, "example.7z", {first});
        QTimer warningResponder;
        connect(&warningResponder, &QTimer::timeout, &window, [] { dismissWarnings(); });
        warningResponder.start(1);
        backend.queueListAction(
            [&] { backend.fail(QStringLiteral("Не удалось перечитать архив.")); });
        action(window, "refreshAction")->trigger();
        warningResponder.stop();
        QVERIFY(action(window, "createFolderAction")->isEnabled());
        QVERIFY(!action(window, "testArchiveAction")->isEnabled());
    }
};

QTEST_MAIN(MainWindowStateTests)
#include "ui_state_tests.moc"
