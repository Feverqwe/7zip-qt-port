#include "layout_inspection.h"
#include "mainwindow.h"
#include "qtlocationfolder.h"
#include "window_state_support.h"
#include <QScopeGuard>
#include <QTreeWidget>
#include <QtTest>

class PropertiesTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static CMyComPtr<IFolderFolder> archiveFolder(const QList<ArchiveEntry> &entries) {
        CMyComPtr<IFolderFolder> folder;
        if (CreateArchiveFolder(entries, UString(L"7z"), &folder) != S_OK)
            return {};
        return folder;
    }
    static CRecordVector<UInt32> allIndices(IFolderFolder *folder) {
        CRecordVector<UInt32> indices;
        UInt32 count;
        if (folder->GetNumberOfItems(&count) == S_OK)
            for (UInt32 i = 0; i < count; ++i)
                indices.Add(i);
        return indices;
    }
    static QString value(const QDialog &dialog, const QString &name) {
        auto *list = dialog.findChild<QTreeWidget *>("propertiesList");
        if (!list)
            return QString();
        const auto found = list->findItems(name, Qt::MatchExactly, 0);
        return found.isEmpty() ? QString() : found.first()->text(1);
    }

  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void selectedFilesAndFolderShowSummary() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString original = QDir::currentPath();
        QVERIFY(QDir::setCurrent(temporary.path()));
        for (const auto &name :
             {QStringLiteral("один файл.txt"), QStringLiteral("два файла.txt")}) {
            QFile file(temporary.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("hello"), qint64(5));
        }
        QVERIFY(QDir().mkdir(QStringLiteral("папка")));
        MainWindow window;
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *action = window.findChild<QAction *>("selectedPropertiesAction");
        QVERIFY(table && action);
        QCOMPARE(table->rowCount(), 3);
        table->selectAll();
        bool checked = false;
        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("selectedPropertiesDialog");
            QVERIFY(dialog);
            const auto closeDialog = qScopeGuard([dialog] { dialog->reject(); });
            QCOMPARE(dialog->windowTitle(), QString("Свойства"));
            QCOMPARE(value(*dialog, "Файлов"), QString("2"));
            QCOMPARE(value(*dialog, "Папок"), QString("1"));
            QCOMPARE(value(*dialog, "Размер"), QString("10"));
            QCOMPARE(value(*dialog, "Сжатый размер"), QString("10"));
            QCOMPARE(QDir::fromNativeSeparators(value(*dialog, "Путь")),
                     QFileInfo(temporary.path()).canonicalFilePath() + '/');
            checked = true;
            dialog->reject();
        });
        action->trigger();
        QVERIFY(checked);
        QVERIFY(QDir::setCurrent(original));
    }

    void archiveSummaryIncludesPackedSize() {
        auto folder = archiveFolder({ArchiveEntry{"a.txt", 10, 4}, ArchiveEntry{"b.txt", 5, 2}});
        QVERIFY(folder);
        CPanel panel(folder);
        panel.SetCurrentFolderPrefix("archive.7z");
        PropertiesDialog dialog(panel, allIndices(folder));
        QCOMPARE(value(dialog, "Сжатый размер"), QString("6"));
        QCOMPARE(value(dialog, "Размер"), QString("15"));
        QCOMPARE(value(dialog, "Файлов"), QString("2"));
    }

    void archiveFolderPropertiesIncludeNestedFiles() {
        ControlledBackend backend;
        MainWindow window(&backend);
        openListedArchive(window, backend, "folder.7z",
                          {ArchiveEntry{QStringLiteral("папка/внутри.txt"), 13, 5},
                           ArchiveEntry{QStringLiteral("папка/sub/ещё.txt"), 7, 3}});
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *action = window.findChild<QAction *>("selectedPropertiesAction");
        QCOMPARE(table->rowCount(), 1);
        table->selectRow(0);
        bool checked = false;
        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("selectedPropertiesDialog");
            QVERIFY(dialog);
            const auto closeDialog = qScopeGuard([dialog] { dialog->reject(); });
            QCOMPARE(value(*dialog, "Файлов"), QString("2"));
            QCOMPARE(value(*dialog, "Папок"), QString("1"));
            QCOMPARE(value(*dialog, "Размер"), QString("20"));
            QCOMPARE(value(*dialog, "Сжатый размер"), QString("8"));
            checked = true;
            dialog->reject();
        });
        action->trigger();
        QVERIFY(checked);
    }

    void sourceSummaryUsesDescendantsOfTheSelectedFolder() {
        auto folder = archiveFolder({ArchiveEntry{QStringLiteral("папка/файл.txt"), 13, 5},
                                     ArchiveEntry{QStringLiteral("папка/sub/второй.txt"), 7, 3},
                                     ArchiveEntry{QStringLiteral("другой.txt"), 4, 2}});
        QVERIFY(folder);
        CPanel panel(folder);
        PropertiesDialog dialog(panel, allIndices(folder));
        QCOMPARE(value(dialog, "Размер"), QString("24"));
        QCOMPARE(value(dialog, "Сжатый размер"), QString("10"));
        QCOMPARE(value(dialog, "Файлов"), QString("3"));
        QCOMPARE(value(dialog, "Папок"), QString("2"));
    }

    void singleFileFormatsSizeAndOmitsUnavailableDate() {
        auto folder = archiveFolder({ArchiveEntry{QStringLiteral("файл 😀.txt"), 12345, 3}});
        QVERIFY(folder);
        CPanel panel(folder);
        panel.SetCurrentFolderPrefix("archive.7z");
        PropertiesDialog dialog(panel, allIndices(folder));
        QCOMPARE(value(dialog, "Имя"), QStringLiteral("файл 😀.txt"));
        QCOMPARE(value(dialog, "Размер"), QString("12 345"));
        QVERIFY(value(dialog, "Изменён").isNull());
        QCOMPARE(value(dialog, "Сжатый размер"), QString("3"));
        QVERIFY(value(dialog, "Файлов").isNull());
        QVERIFY(value(dialog, "Папок").isNull());
        auto *list = dialog.findChild<QTreeWidget *>();
        QVERIFY(!list->header()->isHidden());
        QCOMPARE(list->columnCount(), 2);
    }

    void geometry_data() {
        QTest::addColumn<QSize>("size");
        QTest::newRow("normal") << QSize(540, 360);
        QTest::newRow("compact") << QSize(360, 240);
    }
    void geometry() {
        QFETCH(QSize, size);
        auto folder =
            archiveFolder({ArchiveEntry{QStringLiteral("длинное имя 😀.txt"), 12345, 5432}});
        QVERIFY(folder);
        CPanel panel(folder);
        panel.SetCurrentFolderPrefix("archive.zip" + QString("/long path/").repeated(150));
        PropertiesDialog dialog(panel, allIndices(folder));
        dialog.resize(size);
        dialog.show();
        QApplication::processEvents();
        auto *list = dialog.findChild<QTreeWidget *>();
        auto *buttons = dialog.findChild<QDialogButtonBox *>();
        QVERIFY(UiInspection::fullyVisible(*list, dialog));
        QVERIFY(UiInspection::fullyVisible(*buttons, dialog));
        QVERIFY(UiInspection::separate(*list, *buttons, dialog));
        QVERIFY(UiInspection::fullyVisible(*list->header(), dialog));
        for (auto *button : buttons->buttons())
            QVERIFY(UiInspection::fullyVisible(*button, dialog));
        QVERIFY(UiInspection::writeReport(dialog, QString("properties-%1").arg(size.width())));
    }

    void filesystemPropertiesUseLoadedMetadataUntilReload() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString original = QDir::currentPath();
        QVERIFY(QDir::setCurrent(temporary.path()));
        const QString name = QStringLiteral("файл й с пробелом.txt");
        QFile file(temporary.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("fixture"), qint64(7));
        file.close();
        MainWindow window;
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *action = window.findChild<QAction *>("selectedPropertiesAction");
        QVERIFY(table && action);
        QCOMPARE(table->rowCount(), 1);
        table->selectRow(0);
        int inspected = 0;
        auto inspect = [&] {
            QTimer::singleShot(0, [&] {
                auto *dialog = window.findChild<QDialog *>("selectedPropertiesDialog");
                QVERIFY(dialog);
                QCOMPARE(value(*dialog, "Имя").normalized(QString::NormalizationForm_C), name);
                QCOMPARE(value(*dialog, "Размер"), QString("7"));
                QCOMPARE(value(*dialog, "Сжатый размер"), QString("7"));
                QVERIFY(!value(*dialog, "Изменён").isEmpty());
                ++inspected;
                dialog->reject();
            });
            action->trigger();
        };
        inspect(); // Also primes upstream's lazy packed-size cache on Windows.
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(file.write(QByteArray(23, 'x')), qint64(23));
        file.close();
        inspect(); // A row/QFileInfo reconstruction would incorrectly show 23.
        QCOMPARE(inspected, 2);
        QVERIFY(QDir::setCurrent(original));
    }

    void archivePropertiesUsePresenceAndRetainSuppliedTimestampText() {
        ControlledBackend backend;
        MainWindow window(&backend);
        openListedArchive(window, backend, "metadata.7z",
                          {ArchiveEntry{"unknown.txt", -1, -1, "2026-09-28 12:34:56.1234567"},
                           ArchiveEntry{"zero.txt", 0, 0}});
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *action = window.findChild<QAction *>("selectedPropertiesAction");
        QVERIFY(table && action);
        int unknownRow = -1;
        for (int row = 0; row < table->rowCount(); ++row)
            if (table->item(row, 0)->text() == "unknown.txt")
                unknownRow = row;
        QVERIFY(unknownRow >= 0);
        table->selectRow(unknownRow);
        // Presentation cells are not authoritative metadata.
        table->item(unknownRow, 2)->setText("999");
        table->item(unknownRow, 3)->setText("888");
        table->item(unknownRow, 4)->setText("changed presentation");
        bool inspected = false;
        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("selectedPropertiesDialog");
            QVERIFY(dialog);
            const auto closeDialog = qScopeGuard([dialog] { dialog->reject(); });
            QCOMPARE(value(*dialog, "Имя"), QString("unknown.txt"));
            QVERIFY(value(*dialog, "Размер").isNull());
            QVERIFY(value(*dialog, "Сжатый размер").isNull());
            QCOMPARE(value(*dialog, "Изменён"), QString("2026-09-28 12:34:56.1234567"));
            inspected = true;
            dialog->reject();
        });
        action->trigger();
        QVERIFY(inspected);
    }
};
QTEST_MAIN(PropertiesTests)
#include "properties_tests.moc"
