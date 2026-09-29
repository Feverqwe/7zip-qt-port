#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class WindowInfoTests : public QObject {
    Q_OBJECT
  private slots:
    void originalBinaryCoordinatesAndPanelFields() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString settings = fixture.filePath("window.ini");
        CWindowInfo info{{-40, 12, 960, 620}, true, 2, 1, 470};
        info.Save(settings);
        QSettings key(settings, QSettings::IniFormat);
        QCOMPARE(key.value("Position").toByteArray(),
                 QByteArray::fromHex("D8FFFFFF0C000000C00300006C02000001000000"));
        QCOMPARE(key.value("Panels").toByteArray(),
                 QByteArray::fromHex("0200000001000000D6010000"));
        CWindowInfo read{};
        bool position, panels;
        read.Read(position, panels, settings);
        QVERIFY(position && panels);
        QCOMPARE(read.rect.left, -40);
        QCOMPARE(read.rect.top, 12);
        QCOMPARE(read.rect.right, 960);
        QCOMPARE(read.rect.bottom, 620);
        QVERIFY(read.maximized);
        QCOMPARE(read.numPanels, 2u);
        QCOMPARE(read.currentPanel, 1u);
        QCOMPARE(read.splitterPos, 470u);
    }

    void malformedValuesDoNotOverwriteUnrelatedRecord() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString settings = fixture.filePath("window.ini");
        CWindowInfo info{{1, 2, 3, 4}, false, 1, 0, 200};
        info.Save(settings);
        QSettings key(settings, QSettings::IniFormat);
        key.setValue("Position", QString(20, '0')); // REG_SZ is not REG_BINARY.
        key.sync();
        CWindowInfo read{{99, 98, 97, 96}, true, 88, 87, 86};
        bool position, panels;
        read.Read(position, panels, settings);
        QVERIFY(!position && panels);
        QCOMPARE(read.rect.left, 99);
        QCOMPARE(read.numPanels, 1u);
        key.setValue("Position", QByteArray(19, '\0'));
        key.setValue("Panels", QByteArray(13, '\0'));
        key.sync();
        read.Read(position, panels, settings);
        QVERIFY(!position && !panels);
        QCOMPARE(read.rect.left, 99);
        QCOMPARE(read.numPanels, 1u);
    }
};

QTEST_APPLESS_MAIN(WindowInfoTests)
#include "window_info_tests.moc"
