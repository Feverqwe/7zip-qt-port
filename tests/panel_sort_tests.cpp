#include "window_state_support.h"
#include <QHeaderView>

class PanelSortTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
  private:
    static QStringList names(MainWindow &window) {
        QStringList result;
        for (int row = 0; row < table(window)->rowCount(); ++row)
            result << table(window)->item(row, 0)->text();
        return result;
    }

  private slots:
    void archiveDirectionGroupingAndRefresh() {
        QTemporaryDir temp;
        ControlledBackend backend;
        MainWindow window(&backend);
        window.openArchive(temp.filePath("sort.7z"));
        const QList<ArchiveEntry> entries = {{"file2.txt", 2, 20, "2022-01-01"},
                                             {"file10.txt", 10, 10, "2024-01-01"},
                                             {QStringLiteral("папка с пробелом"), 0, 0, {}, true},
                                             {"folder", 0, 0, {}, true}};
        backend.completeList(entries);
        const QStringList folders = {"folder", QStringLiteral("папка с пробелом")};
        action(window, "sort3Action")->trigger();
        QCOMPARE(names(window), (QStringList{folders[1], folders[0], "file10.txt", "file2.txt"}));
        QCOMPARE(table(window)->horizontalHeader()->sortIndicatorOrder(), Qt::DescendingOrder);
        action(window, "refreshAction")->trigger();
        backend.completeList(entries);
        QCOMPARE(names(window), (QStringList{folders[1], folders[0], "file10.txt", "file2.txt"}));
        action(window, "sort3Action")->trigger();
        QCOMPARE(names(window), (QStringList{folders[0], folders[1], "file2.txt", "file10.txt"}));
        // A header property not exposed by the sort menu must survive refresh too.
        table(window)->sortItems(3, Qt::DescendingOrder);
        QCOMPARE(names(window).mid(2), (QStringList{"file2.txt", "file10.txt"}));
        action(window, "refreshAction")->trigger();
        backend.completeList(entries);
        QCOMPARE(table(window)->horizontalHeader()->sortIndicatorSection(), 3);
        QCOMPARE(table(window)->horizontalHeader()->sortIndicatorOrder(), Qt::DescendingOrder);
        QCOMPARE(names(window).mid(2), (QStringList{"file2.txt", "file10.txt"}));
        action(window, "sort2Action")->trigger();
        QCOMPARE(names(window).mid(2), (QStringList{"file10.txt", "file2.txt"}));
        action(window, "sort0Action")->trigger();
        action(window, "sort0Action")->trigger();
        QCOMPARE(names(window), (QStringList{folders[1], folders[0], "file10.txt", "file2.txt"}));
    }

    void headerDefaultsAndFilesystemNavigation() {
        QTemporaryDir temp;
        QVERIFY(QDir(temp.path()).mkdir(QStringLiteral("папка с пробелом")));
        for (const QString &name : {"file2.txt", "file10.txt"}) {
            QFile file(temp.filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(name == "file2.txt" ? "xx" : "xxxxxxxxxx");
        }
        ControlledBackend backend;
        MainWindow window(&backend);
        window.resize(980, 620);
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(temp.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto *header = table(window)->horizontalHeader();
        QTest::mouseClick(header->viewport(), Qt::LeftButton, Qt::NoModifier,
                          QPoint(header->sectionViewportPosition(2) + 10, header->height() / 2));
        QCOMPARE(header->sortIndicatorOrder(), Qt::DescendingOrder);
        QCOMPARE(names(window),
                 (QStringList{QStringLiteral("папка с пробелом"), "file10.txt", "file2.txt"}));
        action(window, "refreshAction")->trigger();
        QCOMPARE(header->sortIndicatorOrder(), Qt::DescendingOrder);
        QCOMPARE(names(window).mid(1), (QStringList{"file10.txt", "file2.txt"}));
        address->setText(temp.filePath(QStringLiteral("папка с пробелом")));
        QTest::keyClick(address, Qt::Key_Return);
        action(window, "parentFolderAction")->trigger();
        QCOMPARE(header->sortIndicatorOrder(), Qt::DescendingOrder);
        QCOMPARE(names(window).mid(1), (QStringList{"file10.txt", "file2.txt"}));
        const QString screenshots = qEnvironmentVariable("SEVENZIP_SCREENSHOT_DIR");
        if (!screenshots.isEmpty()) {
            QDir().mkpath(screenshots);
            for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
                window.resize(size);
                QCoreApplication::processEvents();
                QVERIFY(window.grab().save(screenshots +
                                           QStringLiteral("/sort-%1.png").arg(size.width())));
            }
        }
    }
};
QTEST_MAIN(PanelSortTests)
#include "panel_sort_tests.moc"
