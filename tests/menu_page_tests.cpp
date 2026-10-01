#include "Common/MyTypes.h"

#include "7zip/UI/Explorer/ContextMenuFlags.h"
#include "CPP/7zip/UI/Common/ZipRegistry.h"
#include "CPP/7zip/UI/FileManager/MenuPage.h"
#include "layout_inspection.h"
#include "qtfmoptionsdialog.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QtTest>

class MenuPageTests : public QObject {
    Q_OBJECT
  private slots:
    void capabilitiesAndGeometry() {
        QTemporaryDir fixture;
        QtFmOptionsDialog dialog(fixture.filePath("settings.ini"));
        auto *pages = dialog.findChild<QTabWidget *>("optionsPages");
        QVERIFY(pages->isTabEnabled(1));
        QCOMPARE(pages->tabText(1), QString("7-Zip"));
        QCOMPARE(pages->widget(1)->property("sourcePageID").toUInt(), 2300u);
        pages->setCurrentIndex(1);
        dialog.show();
        auto *zone = dialog.findChild<QComboBox *>("menuZone");
        QCOMPARE(zone->isEnabled(), IsZonePropagationSupported());
        QCOMPARE(zone->count(), 3);
        QCOMPARE(zone->currentText(), QString("* No"));
        auto *items = dialog.findChild<QListWidget *>("menuItems");
        QCOMPARE(items->count(), 14);
        QCOMPARE(items->item(0)->text(), QString("Open archive"));
        QCOMPARE(items->item(1)->text(), QString("Open archive >"));
        QCOMPARE(items->item(4)->text(), QString("Extract to <Folder>"));
        QCOMPARE(items->item(7)->text(), QString("Add to <Archive>.7z"));
        QCOMPARE(items->item(8)->text(), QString("Add to <Archive>.zip"));
        QCOMPARE(items->item(13)->text(), QString("7-Zip > CRC SHA >"));
        QCOMPARE(items->item(13)->data(Qt::UserRole).toUInt(), NContextMenuFlags::kCRC_Cascaded);
        for (int i = 0; i < items->count(); i++)
            QCOMPARE(items->item(i)->checkState(), Qt::Checked);
        QVERIFY(dialog.findChild<QCheckBox *>("cascadedMenu")->isChecked());
        QVERIFY(dialog.findChild<QCheckBox *>("menuElimDup")->isChecked());
        QVERIFY(!dialog.findChild<QCheckBox *>("menuIcons")->isChecked());
        for (const char *name : {"shellIntegration", "shellIntegration32", "cascadedMenu",
                                 "menuIcons", "menuElimDup", "menuItems"}) {
            auto *control = dialog.findChild<QWidget *>(name);
            QVERIFY(!control->isEnabled());
            QVERIFY(!control->toolTip().isEmpty());
        }
        auto *apply = dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply);
        QTest::mouseClick(dialog.findChild<QCheckBox *>("cascadedMenu"), Qt::LeftButton);
        QVERIFY(!apply->isEnabled());
        for (QSize size : {QSize(640, 390), QSize(520, 380)}) {
            dialog.resize(size);
            QApplication::processEvents();
            QCOMPARE(dialog.size(), size);
            QWidget *previous = nullptr;
            for (const char *name :
                 {"shellIntegration", "shellIntegration32", "cascadedMenu", "menuIcons",
                  "menuElimDup", "menuZone", "menuItemsLabel", "menuItems"}) {
                auto *control = dialog.findChild<QWidget *>(name);
                QVERIFY(UiInspection::fullyVisible(*control, dialog));
                if (previous)
                    QVERIFY(UiInspection::bounds(*previous, dialog).bottom() <
                            UiInspection::bounds(*control, dialog).top());
                if (qobject_cast<QCheckBox *>(control))
                    QVERIFY2(control->width() >= control->sizeHint().width(), name);
                previous = control;
            }
            QVERIFY(UiInspection::separate(*dialog.findChild<QWidget *>("menuZoneLabel"), *zone,
                                           dialog));
            QVERIFY(
                UiInspection::separate(*items, *dialog.findChild<QDialogButtonBox *>(), dialog));
            QVERIFY(
                UiInspection::writeReport(dialog, QString("options-menu-%1").arg(size.width())));
        }
    }
    void sourceZoneApplyCancelAndCustomValue() {
        QTemporaryDir fixture;
        const QString path = fixture.filePath("settings.ini");
        {
            QSettings key(path, QSettings::IniFormat);
            key.setValue("Options/CascadedMenu", 0u);
            key.setValue("Options/MenuIcons", 1u);
            key.setValue("Options/ElimDupExtract", 0u);
            key.setValue("Options/ContextMenu", NContextMenuFlags::kOpen | NContextMenuFlags::kCRC);
            key.setValue("Options/WriteZoneIdExtract", 7u);
        }
        {
            CMenuPage cancel(path, nullptr, true);
            auto *zone = cancel.findChild<QComboBox *>("menuZone");
            QCOMPARE(zone->count(), 4);
            QCOMPARE(zone->currentText(), QString("7"));
            zone->setCurrentIndex(1);
            QVERIFY(cancel.IsModified());
        }
        CContextMenuInfo info;
        info.Load(path);
        QCOMPARE(info.WriteZone, 7u);
        CMenuPage page(path, nullptr, true);
        auto *zone = page.findChild<QComboBox *>("menuZone");
        zone->setCurrentIndex(2);
        QCOMPARE(page.OnApply(), 0L);
        QVERIFY(!page.IsModified());
        info.Load(path);
        QCOMPARE(info.WriteZone, UInt32(NExtract::NZoneIdMode::kOffice));
        QVERIFY(info.Cascaded.Def && !info.Cascaded.Val);
        QVERIFY(info.MenuIcons.Def && info.MenuIcons.Val);
        QVERIFY(info.ElimDup.Def && !info.ElimDup.Val);
        QCOMPARE(info.Flags, NContextMenuFlags::kOpen | NContextMenuFlags::kCRC);
        zone->setCurrentIndex(0);
        QCOMPARE(page.OnApply(), 0L);
        info.Load(path);
        QCOMPARE(info.WriteZone, UInt32(-1));
        QVERIFY(!QSettings(path, QSettings::IniFormat).contains("Options/WriteZoneIdExtract"));
        CMenuPage unavailable(path, nullptr, false);
        unavailable.findChild<QComboBox *>("menuZone")->setCurrentIndex(1);
        QVERIFY(!unavailable.IsModified());
        QCOMPARE(unavailable.OnApply(), 0L);
        info.Load(path);
        QCOMPARE(info.WriteZone, UInt32(-1));
    }
};
QTEST_MAIN(MenuPageTests)
#include "menu_page_tests.moc"
