#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class ViewSettingsColumnsTests final : public QObject {
    Q_OBJECT

  private slots:
    void sourceBinaryRoundTripKeepsFolderTypesIndependent() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString path = temp.filePath("columns.ini");
        CListViewInfo fs;
        fs.SortID = 8;
        fs.Ascending = false;
        fs.Columns.Add({3, true, 180});
        fs.Columns.Add({8, false, 92});
        const UString fsType = ToUString(QStringLiteral("FS папка/😀"));
        fs.Save(fsType, path);

        CListViewInfo archive;
        archive.SortID = 3;
        archive.Columns.Add({3, true, 240});
        archive.Save(UString(L"7-Zip"), path);

        CListViewInfo restored;
        restored.Read(fsType, path);
        QVERIFY(restored.IsLoaded);
        QVERIFY(restored.IsEqual(fs));
        QCOMPARE(restored.Columns.Size(), 2u);
        restored.Read(UString(L"7-Zip"), path);
        QVERIFY(restored.IsLoaded);
        QVERIFY(restored.IsEqual(archive));

        QSettings settings(path, QSettings::IniFormat);
        bool checkedSourceBytes = false;
        for (const QString &key : settings.allKeys()) {
            const QByteArray bytes = settings.value(key).toByteArray();
            if (bytes.size() == 36) {
                checkedSourceBytes = true;
                QCOMPARE(bytes.left(12).toHex(), QByteArray("010000000800000000000000"));
                QCOMPARE(bytes.mid(12).toHex(),
                         QByteArray("0300000001000000b400000008000000000000005c000000"));
            }
        }
        QVERIFY(checkedSourceBytes);
    }

    void invalidVersionAndMalformedColumnsFollowSourceReadPolicy() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString path = temp.filePath("columns.ini");
        CListViewInfo info;
        info.SortID = 8;
        info.Columns.Add({3, true, 160});
        info.Save(UString(L"FSFolder"), path);
        QSettings settings(path, QSettings::IniFormat);
        const QString key = settings.allKeys().constFirst();
        QByteArray bytes = settings.value(key).toByteArray();
        bytes[0] = char(2);
        settings.setValue(key, bytes);
        CListViewInfo restored;
        restored.Read(UString(L"FSFolder"), path);
        QVERIFY(!restored.IsLoaded);
        QVERIFY(restored.Columns.IsEmpty());
        QCOMPARE(restored.SortID, PROPID(0));

        bytes[0] = char(1);
        bytes.append('x');
        settings.setValue(key, bytes);
        restored.Read(UString(L"FSFolder"), path);
        QVERIFY(restored.IsLoaded);
        QCOMPARE(restored.SortID, PROPID(8));
        QVERIFY(restored.Columns.IsEmpty());
    }
};

QTEST_GUILESS_MAIN(ViewSettingsColumnsTests)
#include "view_settings_columns_tests.moc"
