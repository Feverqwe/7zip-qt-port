#include "CPP/7zip/UI/FileManager/App.h"
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

namespace {
class Presentation final : public IQtAppPresentation {
  public:
    QString path;
    QStringList events;
    void Initialize(CApp &) override {}
    bool CreatePanel(CPanel &, unsigned) override { return true; }
    CPanelCallback *PanelCallback(unsigned) override { return nullptr; }
    void SetListSettings(unsigned, const CFmSettings &) override {}
    void ShowPanel(unsigned, bool) override {}
    void ReloadToolbars(CApp &) override {
        events.append(QStringLiteral("reload:%1").arg(ReadToolbarsMask(path)));
    }
    void MoveSubWindows() override { events.append("move"); }
};
} // namespace

class NativeToolbarStateTests : public QObject {
    Q_OBJECT
  private slots:
    void nativeMaskIsAuthoritativeAndSwitchSavesBeforeReload() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        Presentation presentation;
        presentation.path = temp.filePath("toolbar state.ini");
        {
            QSettings settings(presentation.path, QSettings::IniFormat);
            settings.setValue("Toolbars", 5u);          // Text + standard toolbar.
            settings.setValue("toolbar/archive", true); // Old Qt key must not override.
            settings.setValue("toolbar/large", true);
        }
        CApp app;
        app.NumPanels = 1;
        COpenResult opened;
        int widths[] = {300, 300};
        QCOMPARE(app.Create(nullptr, ToUString(temp.path()), UString(), widths, false, opened,
                            presentation, presentation.path),
                 S_OK);
        QVERIFY(app.ShowButtonsLables && app.ShowStandardToolbar);
        QVERIFY(!app.LargeButtons && !app.ShowArchiveToolbar);
        presentation.events.clear();
        app.SwitchArchiveToolbar();
        QCOMPARE(presentation.events, (QStringList{"reload:13", "move"}));
        QCOMPARE(ReadToolbarsMask(presentation.path), UInt32(13));
        presentation.events.clear();
        app.SwitchLargeButtons();
        QCOMPARE(presentation.events, (QStringList{"reload:15", "move"}));
        app.ReadToolbar();
        QVERIFY(app.ShowButtonsLables && app.LargeButtons && app.ShowStandardToolbar &&
                app.ShowArchiveToolbar);
        SaveToolbarsMask(0, presentation.path);
        app.ReadToolbar();
        QVERIFY(!app.ShowButtonsLables && !app.LargeButtons && !app.ShowStandardToolbar &&
                !app.ShowArchiveToolbar);
    }
    void invalidValuesUseOriginalDefaultAndSmallScreenLabelRule() {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        Presentation presentation;
        presentation.path = temp.filePath("invalid toolbar.ini");
        {
            QSettings settings(presentation.path, QSettings::IniFormat);
            settings.setValue("Toolbars", qulonglong(1) << 40);
        }
        QCOMPARE(ReadToolbarsMask(presentation.path), UInt32(0x8000000du));
        CApp app;
        app.NumPanels = 1;
        COpenResult opened;
        int widths[] = {300, 300};
        QCOMPARE(app.Create(nullptr, ToUString(temp.path()), UString(), widths, false, opened,
                            presentation, presentation.path),
                 S_OK);
        QVERIFY(app.ShowButtonsLables && !app.LargeButtons && app.ShowStandardToolbar &&
                app.ShowArchiveToolbar);
        app.ReadToolbar(true);
        QVERIFY(!app.ShowButtonsLables && !app.LargeButtons);
        {
            QSettings settings(presentation.path, QSettings::IniFormat);
            settings.setValue("Toolbars", "invalid");
        }
        QCOMPARE(ReadToolbarsMask(presentation.path), UInt32(0x8000000du));
    }
};
QTEST_MAIN(NativeToolbarStateTests)
#include "native_toolbar_state_tests.moc"
