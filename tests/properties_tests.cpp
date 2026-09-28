#include "layout_inspection.h"
#include "mainwindow.h"
#include "window_state_support.h"
#include <QTreeWidget>
#include <QtTest>

class PropertiesTests : public QObject {
    Q_OBJECT
    static QString value(const QDialog &dialog, const QString &name) {
        auto *list = dialog.findChild<QTreeWidget *>("propertiesList");
        if (!list)
            return QString();
        const auto found = list->findItems(name, Qt::MatchExactly, 0);
        return found.isEmpty() ? QString() : found.first()->text(1);
    }

  private slots:
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
            QCOMPARE(dialog->windowTitle(), QString("Свойства"));
            QCOMPARE(value(*dialog, "Файлов"), QString("2"));
            QCOMPARE(value(*dialog, "Папок"), QString("1"));
            QCOMPARE(value(*dialog, "Размер"), QString("10"));
            QVERIFY(value(*dialog, "Сжатый размер").isNull());
            QCOMPARE(value(*dialog, "Путь"), QFileInfo(temporary.path()).canonicalFilePath());
            checked = true;
            dialog->reject();
        });
        action->trigger();
        QVERIFY(checked);
        QVERIFY(QDir::setCurrent(original));
    }

    void archiveSummaryIncludesPackedSize() {
        PropertiesDialog dialog(
            {{"a.txt", "a.txt", {}, false, 10, 4}, {"b.txt", "b.txt", {}, false, 5, 2}}, true,
            "archive.7z");
        QCOMPARE(value(dialog, "Сжатый размер"), QString("6"));
        QCOMPARE(value(dialog, "Размер"), QString("15"));
        QCOMPARE(value(dialog, "Файлов"), QString("2"));
    }

    void archiveFolderPropertiesIncludeNestedFiles() {
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive("folder.7z");
        backend.completeList({ArchiveEntry{QStringLiteral("папка/внутри.txt"), 13, 5},
                              ArchiveEntry{QStringLiteral("папка/sub/ещё.txt"), 7, 3}});
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        auto *action = window.findChild<QAction *>("selectedPropertiesAction");
        QCOMPARE(table->rowCount(), 1);
        table->selectRow(0);
        bool checked = false;
        QTimer::singleShot(0, [&] {
            auto *dialog = window.findChild<QDialog *>("selectedPropertiesDialog");
            QVERIFY(dialog);
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

    void upstreamSummaryIncludesDescendantCountsAndEverySelectedRow() {
        PropertiesDialog dialog(
            {{QStringLiteral("папка"), QStringLiteral("папка"), {}, true, 20, 8, 2, 1},
             {QStringLiteral("файл.txt"), QStringLiteral("папка/файл.txt"), {}, false, 13, 5},
             {QStringLiteral("другой.txt"), QStringLiteral("другой.txt"), {}, false, 4, 2}},
            true, "archive.7z");
        QCOMPARE(value(dialog, "Размер"), QString("37"));
        QCOMPARE(value(dialog, "Сжатый размер"), QString("15"));
        QCOMPARE(value(dialog, "Файлов"), QString("4"));
        QCOMPARE(value(dialog, "Папок"), QString("2"));
    }

    void singleFileFormatsSizeAndOmitsUnavailableDate() {
        PropertiesDialog dialog(
            {{QStringLiteral("файл 😀.txt"), "/tmp/file.txt", {}, false, 12345, 3}}, false, "/tmp");
        QCOMPARE(value(dialog, "Имя"), QStringLiteral("файл 😀.txt"));
        QCOMPARE(value(dialog, "Размер"), QString("12 345"));
        QVERIFY(value(dialog, "Изменён").isNull());
        QVERIFY(value(dialog, "Сжатый размер").isNull());
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
        PropertiesDialog dialog(
            {{QStringLiteral("длинное имя 😀.txt"), QString("/long path/").repeated(150),
              "2026-09-28 12:00:00", false, 12345, 5432}},
            true, "archive.zip");
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
};
QTEST_MAIN(PropertiesTests)
#include "properties_tests.moc"
