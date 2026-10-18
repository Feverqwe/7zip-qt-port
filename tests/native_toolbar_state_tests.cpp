#include "CPP/7zip/UI/FileManager/App.h"
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

namespace {
class Presentation final : public IQtAppPresentation {
  public:
    QString path;
    QStringList events;
    bool smallScreen = false;
    void Initialize(CApp &) override {}
    bool IsSmallScreen() const override { return smallScreen; }
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
    void startupToolbarDefaultUsesScreenCapability_data() {
        QTest::addColumn<bool>("smallScreen");
        QTest::addColumn<bool>("saved");
        QTest::addColumn<quint32>("mask");
        QTest::addColumn<bool>("labels");
        QTest::newRow("missing-large") << false << false << quint32(0) << true;
        QTest::newRow("missing-small") << true << false << quint32(0) << false;
        QTest::newRow("default-large") << false << true << quint32(0x8000000d) << true;
        QTest::newRow("default-small") << true << true << quint32(0x8000000d) << false;
        QTest::newRow("explicit-labels-small") << true << true << quint32(13) << true;
        QTest::newRow("explicit-icons-large") << false << true << quint32(12) << false;
    }
    void startupToolbarDefaultUsesScreenCapability() {
        QFETCH(bool, smallScreen);
        QFETCH(bool, saved);
        QFETCH(quint32, mask);
        QFETCH(bool, labels);
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        Presentation presentation;
        presentation.path = temp.filePath("startup toolbar.ini");
        presentation.smallScreen = smallScreen;
        if (saved)
            SaveToolbarsMask(mask, presentation.path);
        const UInt32 original = ReadToolbarsMask(presentation.path);
        CApp app;
        app.NumPanels = 1;
        COpenResult opened;
        int widths[] = {300, 300};
        QCOMPARE(app.Create(nullptr, ToUString(temp.path()), UString(), widths, false, opened,
                            presentation, presentation.path),
                 S_OK);
        QCOMPARE(app.ShowButtonsLables, labels);
        QVERIFY(!app.LargeButtons && app.ShowStandardToolbar && app.ShowArchiveToolbar);
        QCOMPARE(ReadToolbarsMask(presentation.path), original);
    }

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
        QVERIFY(presentation.events.contains("reload:13"));
        QVERIFY(presentation.events.contains("move"));
        QCOMPARE(ReadToolbarsMask(presentation.path), UInt32(13));
        presentation.events.clear();
        app.SwitchLargeButtons();
        QVERIFY(presentation.events.contains("reload:15"));
        QVERIFY(presentation.events.contains("move"));
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
        app.ReadToolbar();
        QVERIFY(app.ShowButtonsLables && !app.LargeButtons && app.ShowStandardToolbar &&
                app.ShowArchiveToolbar);
    }
};
QTEST_MAIN(NativeToolbarStateTests)
#include "native_toolbar_state_tests.moc"
