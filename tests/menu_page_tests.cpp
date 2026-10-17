#include "Common/MyTypes.h"

#include "7zip/UI/Explorer/ContextMenuFlags.h"
#include "CPP/7zip/UI/Common/ZipRegistry.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/MenuPage.h"
#include "Common/StringConvert.h"
#include "layout_inspection.h"
#include "qtfmoptionsdialog.h"
#include "qtmenupageservices.h"
#include "qtnativemenupageservices.h"
#include "qtshellregistryservices.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QScopeGuard>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QtTest>

namespace {
class CShellServices final : public IQtMenuPageServices {
  public:
    bool x64 = true, other = true, exists = true;
    QHash<UInt32, bool> Registered;
    QHash<UInt32, LONG> Failures;
    QList<UInt32> Applies;
    QList<LONG> Errors;
    bool CommandsAvailable() const override { return true; }
    bool Is64Bit() const override { return x64; }
    bool OtherBitnessAvailable() const override { return other; }
    FString GetModuleDirPrefix() const override { return us2fs(UString(L"/test install 空/")); }
    const char *GetDllName(unsigned index) const override {
        return index == 0 ? "sevenzip_shell.dll" : "sevenzip_shell32.dll";
    }
    bool DoesFileExist(const FString &) const override { return exists; }
    bool CheckContextMenuHandler(const UString &, UInt32 view) override {
        return Registered.value(view);
    }
    LONG SetContextMenuHandler(bool value, const UString &path, UInt32 view) override {
        Applies.append(view);
        if (!QString::fromWCharArray(path.Ptr()).startsWith("/test install 空/"))
            return 2;
        const LONG error = Failures.value(view);
        if (!error)
            Registered.insert(view, value);
        return error;
    }
    void ShowMenuErrorMessage(LONG error, QWidget *) override { Errors.append(error); }
};
} // namespace

class MenuPageTests : public QObject {
    Q_OBJECT
  private slots:
    void sourceShellApplyOrderAndRequeryAfterFailure() {
        QTemporaryDir fixture;
        auto services = std::make_shared<CShellServices>();
        services->Registered[0] = true;
        services->Failures[NQtShellRegistry::kWow32] = 5;
        CMenuPage page(fixture.filePath("settings.ini"), nullptr, true, services);
        page.resize(640, 390);
        page.show();
        auto *native = page.findChild<QCheckBox *>("shellIntegration");
        auto *other = page.findChild<QCheckBox *>("shellIntegration32");
        QVERIFY(native->isEnabled() && other->isEnabled());
        QVERIFY(native->isChecked() && !other->isChecked());
        QTest::mouseClick(native, Qt::LeftButton, Qt::NoModifier, QPoint(10, native->height() / 2));
        QTest::mouseClick(other, Qt::LeftButton, Qt::NoModifier, QPoint(10, other->height() / 2));
        QVERIFY(page.IsModified());
        QCOMPARE(page.OnApply(), 0L);
        QCOMPARE(services->Applies, QList<UInt32>({NQtShellRegistry::kWow32, 0}));
        QCOMPARE(services->Errors, QList<LONG>({5}));
        QVERIFY(!native->isChecked() && !other->isChecked());
        QVERIFY(!page.IsModified());
        services->Failures.clear();
        services->Applies.clear();
        QTest::mouseClick(other, Qt::LeftButton, Qt::NoModifier, QPoint(10, other->height() / 2));
        QCOMPARE(page.OnApply(), 0L);
        QCOMPARE(services->Applies, QList<UInt32>({NQtShellRegistry::kWow32}));
        QVERIFY(other->isChecked());
        page.OnApply();
        QCOMPARE(services->Applies.size(), 1);
        const UString oldLanguage = g_LangID;
        const auto restoreLanguage = qScopeGuard([&] {
            SaveRegLang(oldLanguage, fixture.filePath("language.ini"));
            ReloadLang(fixture.filePath("language.ini"));
        });
        // Exercise native-sized text even when offscreen defaults to a small font.
        QFont font = page.font();
        font.setPointSize(13);
        page.setFont(font);
        for (const char *language : {"-", "ru"}) {
            SaveRegLang(UString(language), fixture.filePath("language.ini"));
            ReloadLang(fixture.filePath("language.ini"));
            page.ReloadLangItems();
            QVERIFY(native->toolTip().isEmpty() && other->toolTip().isEmpty());
            QVERIFY(page.findChild<QCheckBox *>("cascadedMenu")->toolTip().isEmpty());
            for (QSize size : {QSize(640, 390), QSize(520, 380)}) {
                page.resize(size);
                QApplication::processEvents();
                QVERIFY(UiInspection::writeReport(page, QString("menu-shell-%1-%2")
                                                            .arg(language[0] == '-' ? "en" : "ru")
                                                            .arg(size.width())));
                QWidget *previous = nullptr;
                for (const char *name :
                     {"shellIntegration", "shellIntegration32", "cascadedMenu", "menuIcons",
                      "menuElimDup", "menuZone", "menuItemsLabel", "menuItems"}) {
                    auto *control = page.findChild<QWidget *>(name);
                    QVERIFY(UiInspection::fullyVisible(*control, page));
                    if (previous)
                        QVERIFY2(UiInspection::bounds(*previous, page).bottom() <
                                     UiInspection::bounds(*control, page).top(),
                                 qPrintable(previous->objectName() + " before " +
                                            control->objectName()));
                    if (qobject_cast<QCheckBox *>(control))
                        QVERIFY(control->width() >= control->sizeHint().width());
                    previous = control;
                }
                auto *zoneLabel = page.findChild<QLabel *>("menuZoneLabel");
                auto *zoneCombo = page.findChild<QComboBox *>("menuZone");
                const QFontMetrics metrics(zoneLabel->font());
                const int singleLineWidth =
                    metrics.horizontalAdvance(zoneLabel->text().simplified());
                const int available = UiInspection::bounds(*zoneCombo, page).left() -
                                      UiInspection::bounds(*zoneLabel, page).left();
                if (available >= singleLineWidth) {
                    // Leave a caption on one line when the row has room for it.
                    QVERIFY(zoneLabel->width() >= singleLineWidth);
                    QVERIFY(metrics.size(Qt::TextShowMnemonic, zoneLabel->text()).height() <=
                            metrics.height());
                }
                QVERIFY(UiInspection::labelFits(*zoneLabel));
                QVERIFY(UiInspection::separate(*zoneLabel, *zoneCombo, page));
            }
        }
    }
    void sourceShellDiscoveryCancelAndPersistedFlags() {
        QTemporaryDir fixture;
        const QString path = fixture.filePath("settings.ini");
        auto services = std::make_shared<CShellServices>();
        services->x64 = services->other = false;
        {
            CMenuPage page(path, nullptr, true, services);
            QVERIFY(page.findChild<QCheckBox *>("shellIntegration32")->isHidden());
            QVERIFY(page.findChild<QCheckBox *>("shellIntegration32")->text().contains("64-bit"));
            page.findChild<QCheckBox *>("shellIntegration")->click();
            QVERIFY(page.IsModified());
        }
        QVERIFY(services->Applies.isEmpty());
        services->exists = false;
        CMenuPage page(path, nullptr, true, services);
        QVERIFY(!page.findChild<QCheckBox *>("shellIntegration")->isEnabled());
        page.findChild<QCheckBox *>("cascadedMenu")->click();
        page.findChild<QListWidget *>("menuItems")->item(0)->setCheckState(Qt::Unchecked);
        QCOMPARE(page.OnApply(), 0L);
        CContextMenuInfo info;
        info.Load(path);
        QVERIFY(info.Cascaded.Def && !info.Cascaded.Val);
        QVERIFY(!(info.Flags & NContextMenuFlags::kOpen));
        QVERIFY(services->Applies.isEmpty());
    }
    void capabilitiesAndGeometry() {
        QTemporaryDir fixture;
        QtFmOptionsDialog dialog(fixture.filePath("settings.ini"));
        auto *pages = dialog.findChild<QTabWidget *>("optionsPages");
        QVERIFY(pages->isTabEnabled(1));
        QCOMPARE(pages->tabText(1), QString("7-Zip"));
        QCOMPARE(pages->widget(1)->property("sourcePageID").toUInt(), 2300u);
        pages->setCurrentIndex(1);
        dialog.show();
        QVERIFY(QTest::qWaitForWindowActive(&dialog));
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
        for (const char *name : {"cascadedMenu", "menuIcons", "menuElimDup", "menuItems"}) {
            auto *control = dialog.findChild<QWidget *>(name);
            QVERIFY(control->isEnabled());
            QVERIFY(control->toolTip().isEmpty());
        }
        auto *apply = dialog.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Apply);
        auto *cascaded = dialog.findChild<QCheckBox *>("cascadedMenu");
        QTest::mouseClick(cascaded, Qt::LeftButton, Qt::NoModifier,
                          QPoint(10, cascaded->height() / 2));
        QVERIFY(apply->isEnabled());
        for (QSize size : {QSize(640, 390), QSize(520, 380)}) {
            dialog.resize(size);
            QApplication::processEvents();
            QTRY_COMPARE(dialog.size(), size.expandedTo(dialog.minimumSizeHint()));
            QWidget *previous = nullptr;
            for (const char *name :
                 {"shellIntegration", "shellIntegration32", "cascadedMenu", "menuIcons",
                  "menuElimDup", "menuZone", "menuItemsLabel", "menuItems"}) {
                auto *control = dialog.findChild<QWidget *>(name);
                QVERIFY(UiInspection::fullyVisible(*control, dialog));
                if (previous)
                    QVERIFY2(
                        UiInspection::bounds(*previous, dialog).bottom() <
                            UiInspection::bounds(*control, dialog).top(),
                        qPrintable(previous->objectName() + " before " + control->objectName()));
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
    void productionModuleNamesAndCommandAvailability() {
        auto services = CreateNativeMenuPageServices();
        QVERIFY(services);
        QVERIFY(services->CommandsAvailable());
#ifdef Q_OS_WIN
        QCOMPARE(QByteArray(services->GetDllName(0)), QByteArray("7-zip.dll"));
        QCOMPARE(QByteArray(services->GetDllName(1)),
                 sizeof(void *) == 8 ? QByteArray("7-zip32.dll") : QByteArray("7-zip64.dll"));
        FString path = services->GetModuleDirPrefix();
        path += services->GetDllName(0);
        QCOMPARE(services->DoesFileExist(path),
                 QFileInfo(QCoreApplication::applicationDirPath() + "/7-zip.dll").isFile());
#else
        QVERIFY(!services->OtherBitnessAvailable());
        QVERIFY(!services->DoesFileExist(FString()));
#endif
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
