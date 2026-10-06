#include "7zip/UI/FileManager/resource.h"
#include "layout_inspection.h"
#include "window_state_support.h"
#include <QScopeGuard>
#include <QTreeWidget>

class MainWindowNavigationTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

    static void acceptHistoryLocation(MainWindow &window, const QString &location) {
        QTimer::singleShot(0, &window, [&window, location] {
            auto *dialog = window.findChild<QDialog *>("foldersHistoryDialog");
            QVERIFY(dialog);
            const auto close = qScopeGuard([&] {
                if (dialog->isVisible())
                    dialog->reject();
            });
            auto *list = dialog->findChild<QTreeWidget *>("foldersHistoryList");
            QVERIFY(list);
            const auto matches = list->findItems(location, Qt::MatchExactly);
            QCOMPARE(matches.size(), 1);
            list->setCurrentItem(matches.first());
            for (const QSize &size : {QSize(540, 360), QSize(360, 240)}) {
                dialog->resize(size);
                QApplication::processEvents();
                QVERIFY(UiInspection::fullyVisible(*list, *dialog));
                QVERIFY(UiInspection::writeReport(
                    *dialog, QStringLiteral("archive-history-%1").arg(size.width())));
            }
            dialog->accept();
        });
        action(window, "foldersHistoryAction")->trigger();
    }

  private slots:
    void addressEnterPublishesOneFolder_data() {
        QTest::addColumn<int>("key");
        QTest::newRow("return") << int(Qt::Key_Return);
        QTest::newRow("keypad-enter") << int(Qt::Key_Enter);
    }
    void addressEnterPublishesOneFolder() {
        QFETCH(int, key);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QFile file(temp.filePath(QStringLiteral("файл с пробелом.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        action(window, "flatViewAction")->setChecked(false);
        auto *view = table(window);
        auto *address = window.findChild<QLineEdit *>("archivePath");
        int published = 0;
        connect(view->model(), &QAbstractItemModel::rowsInserted, &window,
                [&](const QModelIndex &, int first, int last) { published += last - first + 1; });
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key(key));
        QTRY_VERIFY(view->isEnabled() &&
                    filesystemRow(window, QFileInfo(file.fileName()).fileName()) >= 0);
        QCOMPARE(published, view->rowCount());
        // Enter on a history entry is still a single explicit refresh.
        published = 0;
        address->setFocus();
        QTest::keyClick(address, Qt::Key(key));
        QTRY_VERIFY(view->isEnabled() &&
                    filesystemRow(window, QFileInfo(file.fileName()).fileName()) >= 0);
        QCOMPARE(published, view->rowCount());
    }

    void archiveFolderHistoryRestoresPersistentLocations() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString archivePath = temp.filePath(QStringLiteral("архив с пробелом.zip"));
        QFile archive(archivePath);
        QVERIFY(archive.open(QIODevice::WriteOnly));
        archive.close();
        const QString folder = QStringLiteral("папка/внутри");
        const QString location = archivePath + " / " + folder;
        const QString sourceLocation = QDir::toNativeSeparators(archivePath + '/' + folder + '/');
        const QList<ArchiveEntry> entries = {ArchiveEntry{folder + "/file.txt", 1, 1}};
        {
            ControlledBackend backend;
            MainWindow window(&backend);
            openListedArchive(window, backend, archivePath, entries);
            openRow(window, rowWithPath(window, QStringLiteral("папка")));
            openRow(window, rowWithPath(window, folder));
            auto *combo = window.findChild<QComboBox *>("pathCombo");
            QVERIFY(combo);
            QSettings settings(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"),
                               QSettings::IniFormat);
            QVERIFY(settings.value("FolderHistory").toStringList().contains(sourceLocation));
        }
        ControlledBackend backend;
        MainWindow window(&backend);
        auto *combo = window.findChild<QComboBox *>("pathCombo");
        auto *address = window.findChild<QLineEdit *>("archivePath");
        backend.queueList(entries);
        acceptHistoryLocation(window, sourceLocation);
        QCOMPARE(QDir::fromNativeSeparators(backend.lastArchive), archivePath);
        QCOMPARE(address->text(), location);
        QCOMPARE(table(window)->rowCount(), 1);
        QCOMPARE(table(window)->item(0, 0)->text(), QStringLiteral("file.txt"));

        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        backend.queueList(entries);
        acceptHistoryLocation(window, sourceLocation);
        QCOMPARE(QDir::fromNativeSeparators(backend.lastArchive), archivePath);
        QCOMPARE(address->text(), location);

        address->setText(archivePath + " / папка");
        QTest::keyClick(address, Qt::Key_Return);
        QCOMPARE(backend.listCalls, 2);
        QCOMPARE(table(window)->item(0, 0)->text(), QStringLiteral("внутри"));
        address->setText(location);
        QTest::keyClick(address, Qt::Key_Return);
        QCOMPARE(address->text(), location);
        combo->showPopup();
        QApplication::processEvents();
        QVERIFY(combo->findText(QStringLiteral("внутри")) >= 0);
        combo->hidePopup();

        // Reopening after the deepest folder disappears uses its surviving parent.
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        address->setText(location);
        backend.queueList({ArchiveEntry{QStringLiteral("папка/other.txt"), 2, 2}});
        QTest::keyClick(address, Qt::Key_Return);
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
        QCOMPARE(action(window, "openInsideAction")->shortcut(),
                 QKeySequence(Qt::CTRL | Qt::Key_PageDown));
        QVERIFY(action(window, "selectAllAction")->isEnabled());
        pressSelectAll(view);
        QCOMPARE(view->selectionModel()->selectedRows().size(), 3);

        // Enter operates the selection; choose a single file after Select All.
        view->clearSelection();

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
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] {
                backend.completeList({ArchiveEntry{QStringLiteral("папка/файл.txt"), 1, 1},
                                      ArchiveEntry{QStringLiteral("другой.txt"), 2, 2}});
            });
            QCOMPARE(backend.listCalls, 1);
            QVERIFY(!action(window, "openItemAction")->isEnabled());
            QVERIFY(!action(window, "parentFolderAction")->isEnabled());
            QVERIFY(!action(window, "selectAllAction")->isEnabled());
        });
        QTest::keyClick(view, Qt::Key_Return);
        pressSelectAll(view);
        QCOMPARE(view->selectionModel()->selectedRows().size(), 2);
        QCOMPARE(action(window, "copyAction")->isEnabled(), true);
        view->clearSelection();
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

        openListedArchive(window, backend, archive.fileName(), {});
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
        openListedArchive(window, backend, "tree.7z",
                          {ArchiveEntry{QStringLiteral("пустая папка"), 0, 0, {}, true},
                           ArchiveEntry{QStringLiteral("папка с пробелом/глубже/10.txt"), 10, 8},
                           ArchiveEntry{QStringLiteral("папка с пробелом/глубже/2.txt"), 2, 2},
                           ArchiveEntry{QStringLiteral("папка с пробелом/") + longName, 20, 5},
                           ArchiveEntry{QStringLiteral("root.txt"), 1, 1}});
        QCOMPARE(table(window)->rowCount(), 3);
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        // The shared parent command leaves an archive root for its filesystem.
        QVERIFY(action(window, "upAction")->isEnabled());
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
        const int extractionCalls = backend.extractCalls;
        backend.queueSourceExtractAction([&] { backend.cancel(); });
        openRow(window, file);
        QCOMPARE(backend.extractCalls, extractionCalls + 1);
        QCOMPARE(backend.lastSelected.size(), 1);
        QCOMPARE(QDir::fromNativeSeparators(backend.lastSelected.first()),
                 QStringLiteral("папка с пробелом/") + longName);
        QCOMPARE(table(window)->rowCount(), 2);
        QCOMPARE(rowWithPath(window, QStringLiteral("папка с пробелом/") + longName), file);
        openRow(window, rowWithPath(window, QStringLiteral("папка с пробелом/глубже")));
        QCOMPARE(table(window)->rowCount(), 2);
        table(window)->sortItems(2, Qt::DescendingOrder);
        QCOMPARE(table(window)->item(0, 0)->text(), QStringLiteral("10.txt"));
        QCOMPARE(table(window)->item(1, 0)->text(), QStringLiteral("2.txt"));
        table(window)->selectRow(1);
        QVERIFY(action(window, "copyAction")->isEnabled());
        window.extractTo("/temporary/destination", true);
        QCOMPARE(backend.lastSelected.size(), 1);
        QCOMPARE(QDir::fromNativeSeparators(backend.lastSelected.first()),
                 QStringLiteral("папка с пробелом/глубже/2.txt"));
        backend.cancel();
        action(window, "upAction")->trigger();
        QCOMPARE(table(window)->rowCount(), 2);
        action(window, "backAction")->trigger();
        QCOMPARE(table(window)->rowCount(), 3);
        openRow(window, rowWithPath(window, QStringLiteral("пустая папка")));
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!action(window, "copyAction")->isEnabled());
        action(window, "upAction")->trigger();
        QCOMPARE(table(window)->rowCount(), 3);
    }

    void reopenFailureAndCancellationAfterFolderNavigation() {
        ControlledBackend backend;
        MainWindow window(&backend);
        openListedArchive(window, backend, "one.zip",
                          {ArchiveEntry{QStringLiteral("nested/file.txt"), 1, 1}});
        openRow(window, rowWithPath(window, QStringLiteral("nested")));
        QVERIFY(backend.prepareArchiveFixture("two.7z"));
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] { backend.cancel(); });
            QCOMPARE(table(window)->rowCount(), 0);
            QVERIFY(!action(window, "upAction")->isEnabled());
        });
        window.openArchive("two.7z");
        QCOMPARE(window.findChild<QLineEdit *>("archivePath")->text(),
                 QFileInfo(QDir::currentPath()).canonicalFilePath());
        openListedArchive(window, backend, "three.zip", {});
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(action(window, "upAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(backend.prepareArchiveFixture("failed.7z"));
        QTimer warningResponder;
        connect(&warningResponder, &QTimer::timeout, &window, [] { dismissWarnings(); });
        warningResponder.start(1);
        backend.queueListAction([&] { backend.fail(QStringLiteral("7zz не запустился.")); });
        window.openArchive("failed.7z");
        warningResponder.stop();
        QCOMPARE(window.findChild<QLineEdit *>("archivePath")->text(),
                 QFileInfo(QDir::currentPath()).canonicalFilePath());
        QVERIFY(action(window, "upAction")->isEnabled());
    }

    void reopeningAndEmptyArchive() {
        ControlledBackend backend;
        MainWindow window(&backend);
        QVERIFY(action(window, "toolbarAddAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!action(window, "testArchiveAction")->isEnabled());
        QVERIFY(!action(window, "archiveInfoAction")->isEnabled());
        QVERIFY(!action(window, "copyAction")->isEnabled());
        QVERIFY(!action(window, "moveAction")->isEnabled());
        QVERIFY(!action(window, "deleteAction")->isEnabled());
        QVERIFY(!action(window, "renameAction")->isEnabled());

        QVERIFY(backend.prepareArchiveFixture("first.7z"));
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] {
                backend.completeList({ArchiveEntry{QStringLiteral("имя с пробелом.txt"), 4, 4}});
            });
            QCOMPARE(backend.listCalls, 1);
            QVERIFY(!action(window, "toolbarAddAction")->isEnabled());
            QVERIFY(!action(window, "openItemAction")->isEnabled());
            QVERIFY(!action(window, "parentFolderAction")->isEnabled());
            QVERIFY(!action(window, "selectAllAction")->isEnabled());
            window.openArchive("ignored.zip");
            QCOMPARE(backend.listCalls, 1);
        });
        window.openArchive("first.7z");
        QCOMPARE(table(window)->rowCount(), 1);
        QVERIFY(action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(action(window, "archiveInfoAction")->isEnabled());
        QVERIFY(action(window, "selectAllAction")->isEnabled());
        QVERIFY(!action(window, "copyAction")->isEnabled());
        table(window)->selectRow(0);
        QVERIFY(action(window, "copyAction")->isEnabled());

        QVERIFY(backend.prepareArchiveFixture("second.zip"));
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] { backend.completeList({}); });
            QCOMPARE(backend.listCalls, 2);
            QCOMPARE(table(window)->rowCount(), 0);
            QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
            QVERIFY(!action(window, "copyAction")->isEnabled());
            QVERIFY(!action(window, "moveAction")->isEnabled());
        });
        window.openArchive("second.zip");
        QCOMPARE(window.findChild<QLineEdit *>("archivePath")->text(),
                 QFileInfo("second.zip").absoluteFilePath());
        QCOMPARE(table(window)->rowCount(), 0);
        QVERIFY(action(window, "backAction")->isEnabled());
        QVERIFY(action(window, "parentFolderAction")->isEnabled());
        QVERIFY(!action(window, "openItemAction")->isEnabled());
        QVERIFY(!action(window, "selectAllAction")->isEnabled());
        QVERIFY(!action(window, "extractArchiveAction")->isEnabled());
        QVERIFY(!action(window, "copyAction")->isEnabled());
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(action(window, "archiveInfoAction")->isEnabled());
        QCOMPARE(window.statusBar()->currentMessage(), QStringLiteral("Архив открыт."));
    }

    void testAndPropertiesStates() {
        ControlledBackend backend;
        MainWindow window(&backend);
        QVERIFY(backend.prepareArchiveFixture("first.7z"));
        backend.queueListAction([&] {
            emit backend.infoReady(ArchiveInfo{QStringLiteral("7z"), 120, 3, 1, 0});
            backend.completeList({ArchiveEntry{QStringLiteral("δ space.txt"), 3, 7}});
        });
        window.openArchive("first.7z");
        QCOMPARE(action(window, "archiveInfoAction")->property("nativeCommandID").toUInt(),
                 unsigned(IDM_PROPERTIES));
        table(window)->selectRow(0);
        const auto inspect = [&] {
            QStringList values;
            bool shown = false;
            QTimer::singleShot(0, &window, [&] {
                auto *dialog = window.findChild<QDialog *>("selectedPropertiesDialog");
                QVERIFY(dialog);
                const auto close = qScopeGuard([&] { dialog->reject(); });
                auto *list = dialog->findChild<QTreeWidget *>("propertiesList");
                QVERIFY(list);
                for (int i = 0; i < list->topLevelItemCount(); ++i)
                    values.append(list->topLevelItem(i)->text(1));
                shown = true;
            });
            action(window, "archiveInfoAction")->trigger();
            if (!shown)
                values.clear();
            return values;
        };
        const QStringList itemProperties = inspect();
        QVERIFY(!itemProperties.isEmpty());
        QVERIFY(itemProperties.contains(QStringLiteral("δ space.txt")));
        QVERIFY(!window.findChild<QDialog *>("archiveInfoDialog"));
        action(window, "testArchiveAction")->trigger();
        QCOMPARE(backend.testCalls, 1);
        QVERIFY(!action(window, "testArchiveAction")->isEnabled());
        QVERIFY(!action(window, "archiveInfoAction")->isEnabled());
        window.testArchive();
        QCOMPARE(backend.testCalls, 1);
        backend.cancel();
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        QVERIFY(backend.prepareArchiveFixture("empty.zip"));
        backend.queueListAction([&] {
            const auto finish = qScopeGuard([&] { backend.completeList({}); });
            QVERIFY(!action(window, "testArchiveAction")->isEnabled());
        });
        window.openArchive("empty.zip");
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
        // Original Properties shows the archive/folder section even with no
        // operated entries; Info is the same source command as File Properties.
        QVERIFY(action(window, "selectedPropertiesAction")->isEnabled());
        const QStringList folderProperties = inspect();
        QVERIFY(!folderProperties.isEmpty());
        QVERIFY(folderProperties.join('\n').contains("empty.zip"));
        action(window, "testArchiveAction")->trigger();
        QCOMPARE(backend.testCalls, 2);
        backend.fail(QStringLiteral("Операция отменена."), E_ABORT);
        QVERIFY(action(window, "testArchiveAction")->isEnabled());
    }
};

QTEST_MAIN(MainWindowNavigationTests)
#include "ui_navigation_tests.moc"
