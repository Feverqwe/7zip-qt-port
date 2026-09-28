#include "CPP/7zip/UI/FileManager/Panel.h"

#include <QTest>
#include <limits>

class PanelCopyInfoTests : public QObject {
    Q_OBJECT

  private slots:
    void decimalFormatting_data() {
        QTest::addColumn<quint64>("value");
        QTest::addColumn<QString>("text");
        QTest::newRow("zero") << quint64(0) << QString("0");
        QTest::newRow("two digits") << quint64(12) << QString("12");
        QTest::newRow("three digits") << quint64(999) << QString("999");
        QTest::newRow("four digits") << quint64(1000) << QString("1 000");
        QTest::newRow("five digits") << quint64(12345) << QString("12 345");
        QTest::newRow("six digits") << quint64(123456) << QString("123 456");
        QTest::newRow("seven digits") << quint64(1234567) << QString("1 234 567");
        QTest::newRow("uint32 maximum") << quint64(0xFFFFFFFF) << QString("4 294 967 295");
        QTest::newRow("uint64 branch") << quint64(0x100000000) << QString("4 294 967 296");
        QTest::newRow("uint64 maximum")
            << std::numeric_limits<quint64>::max() << QString("18 446 744 073 709 551 615");
    }

    void decimalFormatting() {
        QFETCH(quint64, value);
        QFETCH(QString, text);
        QCOMPARE(ConvertSizeToString(value), text);
    }

    void mixedSelectionUsesPropertyTotalsAndRelativePaths() {
        CObjectVector<CPanelItem> items;
        items.Add({QStringLiteral("подпапка/папка"), true, true, 1234});
        items.Add({QStringLiteral("имя с пробелом.txt"), false, true, 5});
        items.Add({"empty", false, true, 0});
        CRecordVector<UInt32> indices;
        indices.Add(0);
        indices.Add(1);
        indices.Add(2);
        const CPanel panel(QStringLiteral("archive.7z / вложенная/"), items);
        QCOMPARE(panel.GetItemsInfoString(indices),
                 QStringLiteral("Папок: 1    ( 1 234 байт )\n"
                                "Файлов: 2    ( 5 байт )\n"
                                "Размер: 1 239 байт\n\n"
                                "archive.7z / вложенная/\n"
                                "  подпапка/папка/\n  имя с пробелом.txt\n  empty"));
        CRecordVector<UInt32> files;
        files.Add(2);
        files.Add(1);
        QCOMPARE(panel.GetItemsInfoString(files),
                 QStringLiteral("Файлов: 2    ( 5 байт )\n\n"
                                "archive.7z / вложенная/\n  empty\n  имя с пробелом.txt"));
    }

    void undefinedSizeRemainsUndefinedAndZeroIsDefined() {
        CObjectVector<CPanelItem> items;
        items.Add({"unknown", true, false, 0});
        items.Add({"known", true, true, 9999});
        items.Add({"zero", false, true, 0});
        CRecordVector<UInt32> indices;
        indices.Add(0);
        indices.Add(1);
        indices.Add(2);
        const CPanel panel("/folder/", items);
        const QString text = panel.GetItemsInfoString(indices);
        QVERIFY(text.startsWith("Папок: 2\nФайлов: 1    ( 0 байт )\n\n"));
        QVERIFY(!text.contains("9 999"));
        QVERIFY(!text.contains("Размер:"));
        indices.Clear();
        indices.Add(1);
        indices.Add(0);
        QVERIFY(panel.GetItemsInfoString(indices).startsWith("Папок: 2\n\n"));
        indices.Clear();
        QCOMPARE(panel.GetItemsInfoString(indices), QString("\n/folder/"));
    }

    void fiveNamesThenEllipsisButCountsEntireSelection() {
        CObjectVector<CPanelItem> items;
        CRecordVector<UInt32> indices;
        for (UInt32 i = 0; i < 7; ++i) {
            items.Add({QString::number(i), false, true, 1000});
            indices.Add(i);
        }
        const CPanel panel("/folder/", items);
        QCOMPARE(
            panel.GetItemsInfoString(indices),
            QString("Файлов: 7    ( 7 000 байт )\n\n/folder/\n  0\n  1\n  2\n  3\n  4\n  ..."));
        indices.DeleteBack();
        indices.DeleteBack();
        const QString five = panel.GetItemsInfoString(indices);
        QVERIFY(five.startsWith("Файлов: 5    ( 5 000 байт )"));
        QVERIFY(five.endsWith("  4"));
        QVERIFY(!five.contains("..."));
    }
};

QTEST_MAIN(PanelCopyInfoTests)
#include "panel_copy_info_tests.moc"
