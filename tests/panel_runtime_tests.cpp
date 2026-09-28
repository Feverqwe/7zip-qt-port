#include "7zip/PropID.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "filesystemscanner.h"
#include "window_state_support.h"
#include <QSet>
#include <QThread>

using NWindows::NCOM::CPropVariant;

class PanelRuntimeTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT

    static void createFile(const QString &path, const QByteArray &bytes = "fixture") {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(bytes), qint64(bytes.size()));
    }
    static void navigate(MainWindow &window, const QString &path) {
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(path);
        QTest::keyClick(address, Qt::Key_Return);
    }
    static int diskRow(MainWindow &window, const QString &path) {
        auto *view = table(window);
        for (int row = 0; row < view->rowCount(); ++row)
            if (view->item(row, 1)->text() == path)
                return row;
        return -1;
    }
    static void selectOnly(QTableWidget *view, int row) {
        QVERIFY(row >= 0);
        view->clearSelection();
        view->selectionModel()->select(view->model()->index(row, 0),
                                       QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }

  private slots:
    void filesystemArchiveTransitions_data() {
        QTest::addColumn<bool>("flat");
        QTest::addColumn<int>("mode");
        for (bool flat : {false, true})
            for (int mode = 0; mode < 4; ++mode)
                QTest::newRow(qPrintable(QString("flat%1-mode%2").arg(flat).arg(mode)))
                    << flat << mode;
    }
    void filesystemArchiveTransitions() {
        QFETCH(bool, flat);
        QFETCH(int, mode);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QDir root(QFileInfo(temp.path()).canonicalFilePath());
        const QString directory = QStringLiteral("папка й с пробелом");
        const QString name = QStringLiteral("текущий й.txt");
        QVERIFY(root.mkdir(directory));
        createFile(root.filePath(name));
        createFile(root.filePath(directory + '/' + name));
        createFile(root.filePath("zero"), {});
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        action(window, "flatViewAction")->setChecked(false);
        action(window, qPrintable(QString("viewMode%1Action").arg(mode)))->trigger();
        navigate(window, root.path());
        auto *scanner = window.findChild<FilesystemScanner *>();
        QVERIFY(scanner);
        CMyComPtr<IFolderFolder> sourceFolder;
        connect(scanner, &FilesystemScanner::folderReady, &window,
                [&](IFolderFolder *folder) { sourceFolder = folder; });
        action(window, "flatViewAction")->setChecked(flat);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        auto *view = table(window);
        QCOMPARE(view->rowCount(), flat ? 4 : 3);
        if (flat) {
            QVERIFY(sourceFolder);
            UInt32 count = 0;
            QCOMPARE(sourceFolder->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(view->rowCount()));
            QSet<quint32> indices;
            for (int row = 0; row < view->rowCount(); ++row) {
                const quint32 index = view->item(row, 0)->data(Qt::UserRole + 6).toUInt();
                QVERIFY(index < count && !indices.contains(index));
                indices.insert(index);
                CPropVariant sourceName, prefix;
                QCOMPARE(sourceFolder->GetProperty(index, kpidName, &sourceName), S_OK);
                QCOMPARE(sourceFolder->GetProperty(index, kpidPrefix, &prefix), S_OK);
                const QString relPath = (QString::fromWCharArray(prefix.bstrVal) +
                                         QString::fromWCharArray(sourceName.bstrVal))
                                            .normalized(QString::NormalizationForm_C);
                QCOMPARE(view->item(row, 0)->text(), relPath);
            }
        }
        selectOnly(view, diskRow(window, root.filePath(name)));
        window.openArchive("runtime.zip");
        QVERIFY(view->selectionModel()->selectedRows().isEmpty());
        QList<ArchiveEntry> entries = {
            {name, 11, 3}, {directory + '/' + name, 13, 5}, {"zero", 0, 0}, {"unknown", -1, -1}};
        backend.completeList(entries);
        entries.clear();
        action(window, "flatViewAction")->setChecked(flat);
        QCOMPARE(view->rowCount(), flat ? 5 : 4);
        const int archiveRow = rowWithPath(window, name);
        QVERIFY(archiveRow >= 0);
        QCOMPARE(view->item(archiveRow, 2)->text(), QStringLiteral("11"));
        const int unknownRow = rowWithPath(window, "unknown");
        QVERIFY(unknownRow >= 0);
        QVERIFY(view->item(unknownRow, 2)->text().isEmpty());
        selectOnly(view, archiveRow);
        view->setCurrentCell(unknownRow, 0, QItemSelectionModel::NoUpdate);
        navigate(window, root.path());
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        QCOMPARE(view->rowCount(), flat ? 4 : 3);
        QVERIFY(view->selectionModel()->selectedRows().isEmpty());
        const int diskFile = diskRow(window, root.filePath(name));
        QVERIFY(diskFile >= 0);
        QCOMPARE(view->item(diskFile, 2)->text(), QStringLiteral("7"));
        selectOnly(view, diskFile);
        const int zeroRow = diskRow(window, root.filePath("zero"));
        QVERIFY(zeroRow >= 0);
        view->setCurrentCell(zeroRow, 0, QItemSelectionModel::NoUpdate);
        action(window, "refreshAction")->trigger();
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        QCOMPARE(view->item(view->currentRow(), 1)->text(), root.filePath("zero"));
        QCOMPARE(view->selectionModel()->selectedRows().size(), 1);
        QCOMPARE(view->item(view->selectionModel()->selectedRows().first().row(), 1)->text(),
                 root.filePath(name));
        action(window, "flatViewAction")->setChecked(false);
    }

    void filesystemPresentationRetainsLoadedMetadata() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString path = QFileInfo(temp.path()).canonicalFilePath();
        const QString filePath = QDir(path).filePath(QStringLiteral("размер й.txt"));
        createFile(filePath);
        ControlledBackend backend;
        MainWindow window(&backend);
        action(window, "flatViewAction")->setChecked(false);
        navigate(window, path);
        auto *scanner = window.findChild<FilesystemScanner *>();
        QVERIFY(scanner);
        bool changed = false;
        connect(scanner, &FilesystemScanner::folderReady, &window, [&](IFolderFolder *) {
            createFile(filePath, "much larger replacement");
            changed = true;
        });
        action(window, "flatViewAction")->setChecked(true);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        QVERIFY(changed);
        const int row = diskRow(window, filePath);
        QVERIFY(row >= 0);
        QCOMPARE(table(window)->item(row, 2)->text(), QStringLiteral("7"));
        QCOMPARE(QFileInfo(filePath).size(), qint64(23));
        action(window, "flatViewAction")->setChecked(false);
        QCOMPARE(table(window)->item(0, 2)->text(), QStringLiteral("23"));
    }

    void archiveNavigationRejectsUnfinishedFilesystemOwner() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        createFile(temp.filePath("source"));
        ControlledBackend backend;
        MainWindow window(&backend);
        action(window, "flatViewAction")->setChecked(false);
        navigate(window, temp.path());
        auto *scanner = window.findChild<FilesystemScanner *>();
        QVERIFY(scanner);
        QSignalSpy finished(scanner, &FilesystemScanner::completed);
        int readyCount = 0;
        connect(scanner, &FilesystemScanner::folderReady, &window,
                [&](IFolderFolder *) { ++readyCount; });
        action(window, "flatViewAction")->setChecked(true);
        QVERIFY(!action(window, "refreshAction")->isEnabled());
        window.openArchive("replacement.zip");
        backend.completeList({{"source", 29, 17}});
        auto *view = table(window);
        QCOMPARE(view->rowCount(), 1);
        QCOMPARE(view->item(0, 2)->text(), QStringLiteral("29"));
        // Retired native workers are deleted only after their queued handoff.
        QTRY_VERIFY(window.findChildren<QThread *>().isEmpty());
        QCoreApplication::processEvents();
        QCOMPARE(readyCount, 0);
        QCOMPARE(finished.size(), 0);
        QCOMPARE(view->rowCount(), 1);
        QCOMPARE(view->item(0, 2)->text(), QStringLiteral("29"));
        action(window, "flatViewAction")->setChecked(false);
    }
};

QTEST_MAIN(PanelRuntimeTests)
#include "panel_runtime_tests.moc"
