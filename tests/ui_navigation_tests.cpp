#include "window_state_support.h"

class MainWindowNavigationTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

    static void acceptHistoryLocation(MainWindow &window, const QString &location) {
        QTimer::singleShot(0, &window, [&window, location] {
            auto *dialog = window.findChild<QDialog *>("foldersHistoryDialog");
            QVERIFY(dialog);
            auto *list = dialog->findChild<QListWidget *>("foldersHistoryList");
            QVERIFY(list);
            const auto matches = list->findItems(location, Qt::MatchExactly);
            QCOMPARE(matches.size(), 1);
            list->setCurrentItem(matches.first());
            const QString screenshots = qEnvironmentVariable("SEVENZIP_SCREENSHOT_DIR");
            if (!screenshots.isEmpty()) {
                for (const QSize &size : {QSize(540, 360), QSize(360, 240)}) {
                    dialog->resize(size);
                    QApplication::processEvents();
                    QVERIFY(dialog->grab().save(
                        QDir(screenshots)
                            .filePath(QStringLiteral("archive-history-%1.png").arg(size.width()))));
                }
            }
            dialog->accept();
        });
        action(window, "foldersHistoryAction")->trigger();
    }

  private slots:
    void archiveFolderHistoryRestoresPersistentLocations() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString archivePath = temp.filePath(QStringLiteral("архив с пробелом.zip"));
        QFile archive(archivePath);
        QVERIFY(archive.open(QIODevice::WriteOnly));
        archive.close();
        const QString folder = QStringLiteral("папка/внутри");
        const QString location = archivePath + " / " + folder;
        const QList<ArchiveEntry> entries = {ArchiveEntry{folder + "/file.txt", 1, 1}};
        {
            ControlledBackend backend;
            MainWindow window(&backend);
            window.openArchive(archivePath);
            backend.completeList(entries);
            openRow(window, rowWithPath(window, QStringLiteral("папка")));
            openRow(window, rowWithPath(window, folder));
            auto *combo = window.findChild<QComboBox *>("pathCombo");
            QVERIFY(combo);
            QCOMPARE(combo->itemText(0), location);
            QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"),
                               QSettings::IniFormat);
            QVERIFY(settings.value("history/paths").toStringList().contains(location));
        }
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *combo = window.findChild<QComboBox *>("pathCombo");
        auto *address = window.findChild<QLineEdit *>("archivePath");
        const int index = combo->findText(location);
        QVERIFY(index >= 0);
        QVERIFY(
            QMetaObject::invokeMethod(combo, "activated", Qt::DirectConnection, Q_ARG(int, index)));
        QCOMPARE(backend.lastArchive, archivePath);
        backend.completeList(entries);
        QCOMPARE(address->text(), location);
        QCOMPARE(table(window)->rowCount(), 1);
        QCOMPARE(table(window)->item(0, 0)->text(), QStringLiteral("file.txt"));

        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        acceptHistoryLocation(window, location);
        QCOMPARE(backend.lastArchive, archivePath);
        backend.completeList(entries);
        QCOMPARE(address->text(), location);

        address->setText(archivePath + " / папка");
        QTest::keyClick(address, Qt::Key_Return);
        QCOMPARE(backend.listCalls, 2);
        QCOMPARE(table(window)->item(0, 0)->text(), QStringLiteral("внутри"));
        address->setText(location);
        QTest::keyClick(address, Qt::Key_Return);
        QCOMPARE(address->text(), location);
        QCOMPARE(combo->itemText(0), location);
        QCOMPARE(combo->findText(location), 0);

        // Reopening after the deepest folder disappears uses its surviving parent.
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        address->setText(location);
        QTest::keyClick(address, Qt::Key_Return);
        backend.completeList({ArchiveEntry{QStringLiteral("папка/other.txt"), 2, 2}});
        QCOMPARE(address->text(), archivePath + " / папка");
        QCOMPARE(table(window)->item(0, 0)->text(), QStringLiteral("other.txt"));
        QVERIFY(QFile::remove(archivePath));
        address->setText(location);
        QTest::keyClick(address, Qt::Key_Return);
        QCOMPARE(address->text(), archivePath + " / папка");
        QCOMPARE(backend.listCalls, 3);
    }

    void keyboardNavigationAndCommandState() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QVERIFY(QDir(temp.path()).mkdir(QStringLiteral("папка с пробелом")));
        QFile archive(temp.filePath(QStringLiteral("архив с пробелом.zip")));
        QVERIFY(archive.open(QIODevice::WriteOnly));
        archive.close();
        QFile plain(temp.filePath("plain.txt"));
        QVERIFY(plain.open(QIODevice::WriteOnly));
        plain.close();
        ControlledBackend backend;
        QUrl opened;
        MainWindow window(&backend, [&](const QUrl &url) {
            opened = url;
            return true;
        });
        window.show();
        auto *view = table(window);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        view->setFocus();
        QVERIFY(action(window, "parentFolderAction")->isEnabled());
        QCOMPARE(action(window, "openArchiveAction")->shortcut(), QKeySequence::Open);
        QVERIFY(action(window, "selectAllAction")->isEnabled());
        pressSelectAll(view);
        QCOMPARE(view->selectionModel()->selectedRows().size(), 3);

        for (int row = 0; row < view->rowCount(); ++row) {
            if (view->item(row, 0)->text() == "plain.txt")
                view->setCurrentCell(row, 0);
        }
        QVERIFY(action(window, "openItemAction")->isEnabled());
        QTest::keyClick(view, Qt::Key_Return);
        QCOMPARE(QFileInfo(opened.toLocalFile()).canonicalFilePath(),
                 QFileInfo(plain.fileName()).canonicalFilePath());
        QCOMPARE(address->text(), QFileInfo(temp.path()).canonicalFilePath());
        for (int row = 0; row < view->rowCount(); ++row) {
            if (view->item(row, 0)->text() == QStringLiteral("папка с пробелом"))
                view->setCurrentCell(row, 0);
        }
        QVERIFY(action(window, "openItemAction")->isEnabled());
        QTest::keyClick(view, Qt::Key_Return);
        QCOMPARE(address->text(),
                 QFileInfo(temp.filePath(QStringLiteral("папка с пробелом"))).canonicalFilePath());
        QTest::keyClick(view, Qt::Key_Backspace);
        QCOMPARE(address->text(), QFileInfo(temp.path()).canonicalFilePath());

        for (int row = 0; row < view->rowCount(); ++row) {
            if (view->item(row, 0)->text() == QStringLiteral("архив с пробелом.zip"))
                view->setCurrentCell(row, 0);
        }
        QTest::keyClick(view, Qt::Key_Return);
        QCOMPARE(backend.listCalls, 1);
        QVERIFY(!action(window, "openItemAction")->isEnabled());
        QVERIFY(!action(window, "parentFolderAction")->isEnabled());
        QVERIFY(!action(window, "selectAllAction")->isEnabled());
        backend.completeList({ArchiveEntry{QStringLiteral("папка/файл.txt"), 1, 1},
                              ArchiveEntry{QStringLiteral("другой.txt"), 2, 2}});
        pressSelectAll(view);
        QCOMPARE(view->selectionModel()->selectedRows().size(), 2);
        QCOMPARE(action(window, "extractSelectedAction")->isEnabled(), true);
        view->setCurrentCell(rowWithPath(window, QStringLiteral("папка")), 0);
        QTest::keyClick(view, Qt::Key_Return);
        QVERIFY(address->text().endsWith(QStringLiteral(" / папка")));
        QCOMPARE(view->rowCount(), 1);
        view->setCurrentCell(0, 0);
        QVERIFY(action(window, "openItemAction")->isEnabled());
        QTest::keyClick(view, Qt::Key_Backspace);
        QCOMPARE(view->rowCount(), 2);
        QTest::keyClick(view, Qt::Key_Backspace);
        QCOMPARE(address->text(), QFileInfo(temp.path()).canonicalFilePath());

        window.openArchive(archive.fileName());
        backend.completeList({});
        QVERIFY(!action(window, "selectAllAction")->isEnabled());
        QVERIFY(!action(window, "openItemAction")->isEnabled());
        QVERIFY(action(window, "parentFolderAction")->isEnabled());
        QTest::keyClick(view, Qt::Key_Backspace);
        QCOMPARE(address->text(), QFileInfo(temp.path()).canonicalFilePath());
    }

    void archiveTreeSortSelectionAndEmptyFolders() {
        ControlledBackend backend;
        MainWindow window(&backend);
        const QString longName = QString(150, QLatin1Char('x')) + QStringLiteral(" δ.txt");
        window.openArchive("tree.7z");
        backend.completeList({ArchiveEntry{QStringLiteral("пустая папка"), 0, 0, {}, true},
                              ArchiveEntry{QStringLiteral("папка с пробелом/глубже/10.txt"), 10, 8},
                              ArchiveEntry{QStringLiteral("папка с пробелом/глубже/2.txt"), 2, 2},
                              ArchiveEntry{QStringLiteral("папка с пробелом/") + longName, 20, 5},
                              ArchiveEntry{QStringLiteral("root.txt"), 1, 1}});
        QCOMPARE(table(window)->rowCount(), 3);
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!action(window, "upAction")->isEnabled());
        const int empty = rowWithPath(window, QStringLiteral("пустая папка"));
        const int rootFile = rowWithPath(window, QStringLiteral("root.txt"));
        QVERIFY(empty >= 0);
        QVERIFY(rootFile >= 0);
        table(window)->selectionModel()->select(table(window)->model()->index(empty, 0),
                                                QItemSelectionModel::Select |
                                                    QItemSelectionModel::Rows);
        table(window)->selectionModel()->select(table(window)->model()->index(rootFile, 0),
                                                QItemSelectionModel::Select |
                                                    QItemSelectionModel::Rows);
        QCOMPARE(table(window)->selectionModel()->selectedRows().size(), 2);
        window.extractTo("/temporary/destination", true);
        QVERIFY(backend.lastSelected.contains(QStringLiteral("пустая папка")));
        QVERIFY(backend.lastSelected.contains(QStringLiteral("root.txt")));
        backend.cancel();
        const int folder = rowWithPath(window, QStringLiteral("папка с пробелом"));
        QVERIFY(folder >= 0);
        QVERIFY(table(window)->item(folder, 0)->data(Qt::UserRole + 1).toBool());
        openRow(window, folder);
        QCOMPARE(table(window)->rowCount(), 2);
        QVERIFY(action(window, "upAction")->isEnabled());
        QVERIFY(window.findChild<QLineEdit *>("archivePath")
                    ->text()
                    .endsWith(QStringLiteral(" / папка с пробелом")));
        const QString screenshotDir = qEnvironmentVariable("SEVENZIP_SCREENSHOT_DIR");
        if (!screenshotDir.isEmpty()) {
            QVERIFY(QDir().mkpath(screenshotDir));
            for (const QSize size : {QSize(980, 620), QSize(720, 480), QSize(640, 400)}) {
                window.resize(size);
                window.show();
                QCoreApplication::processEvents();
                const QString imagePath =
                    screenshotDir + "/" + QSysInfo::productType() +
                    QStringLiteral("-archive-%1x%2.png").arg(size.width()).arg(size.height());
                QVERIFY(window.grab().save(imagePath));
            }
        }
        const int file = rowWithPath(window, QStringLiteral("папка с пробелом/") + longName);
        QVERIFY(file >= 0);
        openRow(window, file);
        QCOMPARE(backend.lastSelected,
                 QStringList({QStringLiteral("папка с пробелом/") + longName}));
        backend.cancel();
        QCOMPARE(table(window)->rowCount(), 2);
        QCOMPARE(rowWithPath(window, QStringLiteral("папка с пробелом/") + longName), file);
        openRow(window, rowWithPath(window, QStringLiteral("папка с пробелом/глубже")));
        QCOMPARE(table(window)->rowCount(), 2);
        table(window)->sortItems(2, Qt::DescendingOrder);
        QCOMPARE(table(window)->item(0, 0)->text(), QStringLiteral("10.txt"));
        QCOMPARE(table(window)->item(1, 0)->text(), QStringLiteral("2.txt"));
        table(window)->selectRow(1);
        QVERIFY(action(window, "extractSelectedAction")->isEnabled());
        window.extractTo("/temporary/destination", true);
        QCOMPARE(backend.lastSelected,
                 QStringList{QStringLiteral("папка с пробелом/глубже/2.txt")});
        backend.cancel();
        action(window, "upAction")->trigger();
        QCOMPARE(table(window)->rowCount(), 2);
        action(window, "backAction")->trigger();
        QCOMPARE(table(window)->rowCount(), 3);
        openRow(window, rowWithPath(window, QStringLiteral("пустая папка")));
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractSelectedAction")->isEnabled());
        action(window, "upAction")->trigger();
        QCOMPARE(table(window)->rowCount(), 3);
    }

    void reopenFailureAndCancellationAfterFolderNavigation() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive("one.zip");
        backend.completeList({ArchiveEntry{QStringLiteral("nested/file.txt"), 1, 1}});
        openRow(window, rowWithPath(window, QStringLiteral("nested")));
        window.openArchive("two.7z");
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(!action(window, "upAction")->isEnabled());
        backend.cancel();
        QCOMPARE(window.findChild<QLineEdit *>("archivePath")->text(),
                 QFileInfo(QDir::currentPath()).canonicalFilePath());
        window.openArchive("three.zip");
        backend.completeList({});
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(!action(window, "upAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        window.openArchive("failed.7z");
        QTimer::singleShot(0, &window, [] { dismissWarnings(); });
        backend.fail(QStringLiteral("7zz не запустился."));
        QCOMPARE(window.findChild<QLineEdit *>("archivePath")->text(),
                 QFileInfo(QDir::currentPath()).canonicalFilePath());
        QVERIFY(!action(window, "upAction")->isEnabled());
    }

    void reopeningAndEmptyArchive() {
        ControlledBackend backend;
        MainWindow window(&backend);
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
        QVERIFY(action(window, "createArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!action(window, "testArchiveAction")->isEnabled());
        QVERIFY(!action(window, "archiveInfoAction")->isEnabled());
        QVERIFY(!action(window, "copyAction")->isEnabled());
        QVERIFY(!action(window, "moveAction")->isEnabled());
        QVERIFY(!action(window, "deleteAction")->isEnabled());
        QVERIFY(!action(window, "renameAction")->isEnabled());

        window.openArchive("first.7z");
        QCOMPARE(backend.listCalls, 1);
        QVERIFY(!action(window, "openArchiveAction")->isEnabled());
        QVERIFY(!action(window, "createArchiveAction")->isEnabled());
        QVERIFY(!action(window, "openItemAction")->isEnabled());
        QVERIFY(!action(window, "parentFolderAction")->isEnabled());
        QVERIFY(!action(window, "selectAllAction")->isEnabled());
        window.openArchive("ignored.zip");
        QCOMPARE(backend.listCalls, 1);
        backend.completeList({ArchiveEntry{QStringLiteral("имя с пробелом.txt"), 4, 4}});
        QCOMPARE(table(window)->rowCount(), 1);
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(action(window, "archiveInfoAction")->isEnabled());
        QVERIFY(action(window, "selectAllAction")->isEnabled());
        QVERIFY(!action(window, "extractSelectedAction")->isEnabled());
        table(window)->selectRow(0);
        QVERIFY(action(window, "extractSelectedAction")->isEnabled());

        window.openArchive("second.zip");
        QCOMPARE(backend.listCalls, 2);
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!action(window, "copyAction")->isEnabled());
        QVERIFY(!action(window, "moveAction")->isEnabled());
        backend.completeList({});
        QCOMPARE(window.findChild<QLineEdit *>("archivePath")->text(),
                 QFileInfo("second.zip").absoluteFilePath());
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(action(window, "backAction")->isEnabled());
        QVERIFY(action(window, "parentFolderAction")->isEnabled());
        QVERIFY(!action(window, "openItemAction")->isEnabled());
        QVERIFY(!action(window, "selectAllAction")->isEnabled());
        QVERIFY(action(window, "openArchiveAction")->isEnabled());
        QVERIFY(!action(window, "createArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!action(window, "extractSelectedAction")->isEnabled());
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(action(window, "archiveInfoAction")->isEnabled());
        QCOMPARE(window.statusBar()->currentMessage(), QStringLiteral("Архив открыт."));
    }

    void testAndInfoStates() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive("first.7z");
        emit backend.infoReady(ArchiveInfo{QStringLiteral("7z"), 120, 3, 1, 0});
        backend.completeList({ArchiveEntry{QStringLiteral("δ space.txt"), 3, 7}});
        action(window, "archiveInfoAction")->trigger();
        auto *info = window.findChild<QLabel *>("archiveInfoText");
        QVERIFY(info);
        QVERIFY(info->text().contains(QStringLiteral("Формат: 7z")));
        QVERIFY(info->text().contains(QStringLiteral("Размер архива: 120 байт")));
        action(window, "testArchiveAction")->trigger();
        QCOMPARE(backend.testCalls, 1);
        QVERIFY(!action(window, "testArchiveAction")->isEnabled());
        QVERIFY(!action(window, "archiveInfoAction")->isEnabled());
        window.testArchive();
        QCOMPARE(backend.testCalls, 1);
        backend.cancel();
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        window.openArchive("empty.zip");
        QVERIFY(!info->isVisible());
        QVERIFY(!action(window, "testArchiveAction")->isEnabled());
        backend.completeList({});
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        action(window, "archiveInfoAction")->trigger();
        QLabel *emptyInfo = nullptr;
        for (QDialog *dialog : window.findChildren<QDialog *>("archiveInfoDialog")) {
            if (dialog->isVisible())
                emptyInfo = dialog->findChild<QLabel *>("archiveInfoText");
        }
        QVERIFY(emptyInfo);
        QVERIFY(emptyInfo->text().contains(QStringLiteral("Формат: не указан 7zz")));
        action(window, "testArchiveAction")->trigger();
        QCOMPARE(backend.testCalls, 2);
        backend.fail(QStringLiteral("Операция отменена."));
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
    }
};

QTEST_MAIN(MainWindowNavigationTests)
#include "ui_navigation_tests.moc"
