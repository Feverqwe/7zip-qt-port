#include "CPP/7zip/UI/Common/ZipRegistry.h"
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

class ContextMenuSettingsTests final : public QObject {
    Q_OBJECT
  private slots:
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
