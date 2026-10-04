#include "CPP/7zip/UI/Common/ZipRegistry.h"
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class ContextMenuSettingsTests final : public QObject {
    Q_OBJECT
  private slots:
    void diskBooleanPolicies_data() {
        QTest::addColumn<QByteArray>("booleanValues");
        QTest::newRow("qt-boolean-text")
            << QByteArray("CascadedMenu=false\nMenuIcons=true\nElimDupExtract=false\n");
        QTest::newRow("source-dword-values")
            << QByteArray("CascadedMenu=0\nMenuIcons=1\nElimDupExtract=0\n");
    }
    void diskBooleanPolicies() {
        QFETCH(QByteArray, booleanValues);
        QTemporaryDir fixture;
        const QString path = fixture.filePath("policy 空.ini");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        const QByteArray contents = "[Options]\n" + booleanValues + "ContextMenu=2\n";
        QCOMPARE(file.write(contents), qint64(contents.size()));
        file.close();
        CContextMenuInfo info;
        info.Load(path);
        QVERIFY(info.Cascaded.Def && !info.Cascaded.Val);
        QVERIFY(info.MenuIcons.Def && info.MenuIcons.Val);
        QVERIFY(info.ElimDup.Def && !info.ElimDup.Val);
        QCOMPARE(info.Flags, UInt32(2));
    }
    void undefinedDefaultsAndExplicitChoices() {
        QTemporaryDir directory;
        const QString path = directory.filePath("settings.ini");
        CContextMenuInfo info;
        info.Load(path);
        QVERIFY(info.Cascaded.Val);
        QVERIFY(info.ElimDup.Val);
        QVERIFY(!info.MenuIcons.Val);
        QVERIFY(!info.Cascaded.Def);
        QCOMPARE(info.WriteZone, UInt32(-1));
        {
            QSettings key(path, QSettings::IniFormat);
            key.setValue("unrelated/setting", "preserved");
            key.setValue("Options/ContextMenu", 123u);
        }
        info.Cascaded.Val = false;
        info.Cascaded.Def = true;
        info.MenuIcons.Val = true;
        info.MenuIcons.Def = true;
        info.WriteZone = NExtract::NZoneIdMode::kAll;
        info.Save(path);
        CContextMenuInfo restored;
        restored.Load(path);
        QVERIFY(restored.Cascaded.Def);
        QVERIFY(!restored.Cascaded.Val);
        QVERIFY(restored.MenuIcons.Def);
        QVERIFY(restored.MenuIcons.Val);
        QVERIFY(!restored.ElimDup.Def);
        QCOMPARE(restored.WriteZone, UInt32(NExtract::NZoneIdMode::kAll));
        QVERIFY(restored.Flags_Def);
        QCOMPARE(restored.Flags, 123u); // Undefined Save preserves existing flags.
        CContextMenuInfo zoneOnly;
        zoneOnly.Load(path);
        zoneOnly.Cascaded.Def = false;
        zoneOnly.MenuIcons.Def = false;
        zoneOnly.ElimDup.Def = false;
        zoneOnly.Flags_Def = false;
        zoneOnly.WriteZone = NExtract::NZoneIdMode::kOffice;
        zoneOnly.Save(path);
        restored.Load(path);
        QVERIFY(restored.Cascaded.Def && !restored.Cascaded.Val);
        QVERIFY(restored.MenuIcons.Def && restored.MenuIcons.Val);
        QCOMPARE(restored.Flags, 123u);
        QCOMPARE(restored.WriteZone, UInt32(NExtract::NZoneIdMode::kOffice));
        QSettings key(path, QSettings::IniFormat);
        QCOMPARE(key.value("unrelated/setting").toString(), QString("preserved"));
        key.setValue("Options/WriteZoneIdExtract", "invalid");
        key.setValue("Options/ContextMenu", "invalid");
        key.sync();
        restored.Load(path);
        QCOMPARE(restored.WriteZone, UInt32(-1));
        QVERIFY(!restored.Flags_Def);
    }
};
QTEST_GUILESS_MAIN(ContextMenuSettingsTests)
#include "context_menu_settings_tests.moc"
