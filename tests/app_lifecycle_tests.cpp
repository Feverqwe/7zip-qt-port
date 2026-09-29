#include "CPP/7zip/UI/FileManager/App.h"
#include "CPP/7zip/UI/FileManager/RegistryUtils.h"
#include "qtfilemanagerwindowservices.h"
#include <QDir>
#include <QFocusEvent>
#include <QLineEdit>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>
#include <functional>

namespace {
class OwnerPresentation final : public IQtAppPresentation {
  public:
    QList<unsigned> created;
    bool createResult = true;
    void Initialize(CApp &) override {}
    bool CreatePanel(CPanel &panel, unsigned index) override {
        panel.LoadFullPath();
        created.append(index);
        return createResult;
    }
    CPanelCallback *PanelCallback(unsigned) override { return nullptr; }
    void SetListSettings(unsigned, const CFmSettings &) override {}
};

class ReleaseObservedFolder final : public IFolderFolder, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderFolder)
  public:
    explicit ReleaseObservedFolder(std::function<void()> observe) : observe_(std::move(observe)) {}
    ~ReleaseObservedFolder() { observe_(); }
#define UNAVAILABLE_FOLDER_METHOD(signature)                                                       \
    Z7_COM7F_IMF(signature) override { return E_NOTIMPL; }
  Z7_IFACEM_IFolderFolder(UNAVAILABLE_FOLDER_METHOD)
#undef UNAVAILABLE_FOLDER_METHOD
      private : std::function<void()> observe_;
};
} // namespace

class AppLifecycleTests : public QObject {
    Q_OBJECT
  private slots:
    void closeRetainsNativeOrderAndRejectsLiveCallers() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        OwnerPresentation presentation;
        CApp app;
        COpenResult opened;
        int widths[2] = {300, 300};
        QCOMPARE(app.Create(nullptr, ToUString(fixture.path()), UString(), widths, false, opened,
                            presentation, fixture.filePath("close settings.ini")),
                 S_OK);
        QStringList events;
        class Services final : public IQtFileManagerWindowServices {
            CApp &app;
            QStringList &events;

          public:
            Services(CApp &owner, QStringList &log) : app(owner), events(log) {}
            bool CanCloseNow() const override { return true; }
            void Deactivate() override {
                events << "inactive";
                app.QtOperations().Close();
                for (auto &panel : app.Panels)
                    panel.Disable_Processing_Timer_Notify_StatusBar();
            }
            void ExitExternalWorkers(bool hard) override {
                events << (hard ? "exit-hard" : "exit-soft");
            }
            void RevokeDropTarget() override { events << "revoke"; }
            void QuiesceOperations() override {
                QVERIFY(app.Panels[0].GetFolder());
                QVERIFY(app.Panels[1].GetFolder());
                events << "join";
            }
            bool WindowWasCreated() const override { return true; }
            void SaveWindowInfo() override {
                QVERIFY(!app.Panels[0].GetFolder());
                QVERIFY(!app.Panels[1].GetFolder());
                events << "window-info";
            }
        } services(app, events);
        for (unsigned i = 0; i < kNumPanelsMax; ++i)
            app.Panels[i].SetNewFolder(new ReleaseObservedFolder([&, i] {
                QVERIFY(!app.Panels[0].ShouldProcessNotifications());
                QVERIFY(!app.Panels[1].ShouldProcessNotifications());
                events << QStringLiteral("release%1").arg(i);
            }));
        {
            QtAppOperationState::CallScope call(app.QtOperations());
            QVERIFY(!WndProc(QtFileManagerMessage::Close, app, services));
            QVERIFY(events.isEmpty());
        }
        QVERIFY(app.QtOperations().Begin(QtAppOperationState::Kind::Open));
        QVERIFY(!WndProc(QtFileManagerMessage::Close, app, services));
        QVERIFY(events.isEmpty());
        QVERIFY(app.QtOperations().Begin(QtAppOperationState::Kind::Operation));
        QVERIFY(WndProc(QtFileManagerMessage::Close, app, services));
        QCOMPARE(events, QStringList({"inactive", "exit-soft", "revoke", "join", "release0",
                                      "release1", "window-info", "exit-hard"}));
        QVERIFY(WndProc(QtFileManagerMessage::Close, app, services));
        QCOMPARE(events.size(), 8);
    }

    void operationGuardsStayWithAppUntilCompletionOrRelease() {
        CApp app;
        CApp independent;
        QVERIFY(app.QtOperations().Begin(QtAppOperationState::Kind::Open));
        QVERIFY(app.QtOperations().blocking());
        for (auto &panel : app.Panels)
            QVERIFY(!panel.ShouldProcessNotifications());
        QVERIFY(independent.Panels[0].ShouldProcessNotifications());
        QVERIFY(app.QtOperations().Begin(QtAppOperationState::Kind::Operation));
        app.QtOperations().End();
        QVERIFY(!app.QtOperations().active());
        for (auto &panel : app.Panels)
            QVERIFY(panel.ShouldProcessNotifications());
        QVERIFY(app.QtOperations().Begin(QtAppOperationState::Kind::Scan));
        QVERIFY(app.QtOperations().active());
        QVERIFY(!app.QtOperations().blocking());
        app.ReleaseApp();
        QVERIFY(app.QtOperations().closed());
        QVERIFY(!app.QtOperations().active());
        for (auto &panel : app.Panels)
            QVERIFY(!panel.ShouldProcessNotifications());
        QVERIFY(!app.QtOperations().Begin(QtAppOperationState::Kind::Operation));
        app.QtOperations().End();
        for (auto &panel : app.Panels)
            QVERIFY(!panel.ShouldProcessNotifications());
    }

    void createdPanelCallbacksKeepIndependentFocusAndStopAfterRelease() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString settings = fixture.filePath("settings.ini");
        SavePanelPath(1, ToUString(fixture.path()), settings);
        OwnerPresentation presentation;
        CApp app;
        CApp independent;
        int widths[2] = {250, 350};
        COpenResult opened;
        QCOMPARE(app.Create(nullptr, ToUString(fixture.path()), UString(), widths, false, opened,
                            presentation, settings),
                 S_OK);
        QWidget firstList, secondList;
        QLineEdit firstAddress, secondAddress;
        app.Panels[0].AttachPresentation(&firstList, &firstAddress);
        app.Panels[1].AttachPresentation(&secondList, &secondAddress);
        QFocusEvent focus(QEvent::FocusIn);
        QApplication::sendEvent(&secondList, &focus);
        QCOMPARE(app.GetFocusedPanelIndex(), 1u);
        QVERIFY(app.Panels[1]._lastFocusedIsList);
        QApplication::sendEvent(&firstAddress, &focus);
        QCOMPARE(app.GetFocusedPanelIndex(), 0u);
        QVERIFY(!app.Panels[0]._lastFocusedIsList);
        QVERIFY(app.Panels[1]._lastFocusedIsList);
        QCOMPARE(independent.GetFocusedPanelIndex(), 0u);
        QVERIFY(independent.Panels[0]._lastFocusedIsList);
        app.ReleaseApp();
        QApplication::sendEvent(&secondList, &focus);
        QCOMPARE(app.GetFocusedPanelIndex(), 0u);
    }

    void applicationCreatesVisibleOwnerAndRestoresOtherSlotOnDemand() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString settings = fixture.filePath("settings.ini");
        QVERIFY(QDir(fixture.path()).mkdir("правая папка"));
        SavePanelPath(1, ToUString(fixture.filePath("правая папка")), settings);
        CListMode modes;
        modes.Panels[0] = 2;
        modes.Panels[1] = 1;
        modes.Save(settings);
        OwnerPresentation presentation;
        CApp app;
        app.NumPanels = 1;
        int widths[2] = {250, 350};
        COpenResult opened;
        QCOMPARE(app.Create(nullptr, ToUString(fixture.path()), UString(), widths, false, opened,
                            presentation, settings),
                 S_OK);
        QCOMPARE(presentation.created, QList<unsigned>({0}));
        QCOMPARE(app.Panels[0]._xSize, 600);
        QVERIFY(!app.Panels[1].PanelCreated);
        QCOMPARE(app.Panels[0].GetListViewMode(), 2u);
        QCOMPARE(app.Panels[1].GetListViewMode(), 1u);
        QCOMPARE(app.CreateOnePanel(1, UString(), UString(), false, opened), S_OK);
        QCOMPARE(presentation.created, QList<unsigned>({0, 1}));
        QCOMPARE(QDir::cleanPath(ToQString(app.Panels[1].GetCurrentFolderPrefix())),
                 fixture.filePath("правая папка"));
        QCOMPARE(app.CreateOnePanel(1, UString(L"/missing"), UString(), false, opened), S_OK);
        QCOMPARE(presentation.created.size(), 2);
        app.Save();
        UString restored;
        QVERIFY(ReadPanelPath(1, restored, settings));
        QCOMPARE(ToQString(restored), ToQString(app.Panels[1].GetCurrentFolderPrefix()));
        app.ReleaseApp();
    }

    void mainArchiveOwnerIsAttemptedFirstAndStopsOnNonArchive() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        OwnerPresentation presentation;
        CApp app;
        app.LastFocusedPanel = 1;
        int widths[2] = {250, 350};
        COpenResult opened;
        QCOMPARE(app.Create(nullptr, ToUString(fixture.path()), UString(), widths, true, opened,
                            presentation, fixture.filePath("settings.ini")),
                 S_OK);
        QVERIFY(presentation.created.isEmpty());
        QVERIFY(!app.Panels[0].GetFolder());
        QVERIFY(app.Panels[1].GetFolder());
        QVERIFY(!app.Panels[0].PanelCreated);
        QVERIFY(!app.Panels[1].PanelCreated);
        QVERIFY(!opened.ArchiveIsOpened);
        app.ReleaseApp();
    }

    void failedPresentationPropagatesWithoutCreatingRemainingPanel() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        OwnerPresentation presentation;
        presentation.createResult = false;
        CApp app;
        app.LastFocusedPanel = 99;
        int widths[2] = {250, 350};
        COpenResult opened;
        QCOMPARE(app.Create(nullptr, ToUString(fixture.path()), UString(), widths, false, opened,
                            presentation, fixture.filePath("settings.ini")),
                 E_FAIL);
        QCOMPARE(app.LastFocusedPanel, 0u);
        QCOMPARE(presentation.created, QList<unsigned>({0}));
        QVERIFY(!app.Panels[0].PanelCreated);
        QVERIFY(!app.Panels[1].PanelCreated);
        app.ReleaseApp();
    }

    void panelCreateBindsBeforePresentationAndInitializesSourceState() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        CApp app;
        CPanel &panel = app.Panels[0];
        panel._listViewMode = 255;
        panel._lastFocusedIsList = false;
        panel.Disable_Processing_Timer_Notify_StatusBar();
        int creates = 0;
        panel.AttachCreationPresentation([&](CPanel &created) {
            ++creates;
            created.LoadFullPath();
            return created.IsFSFolder() &&
                   QDir::cleanPath(ToQString(created.GetCurrentFolderPrefix())) == fixture.path() &&
                   created.ShouldProcessNotifications();
        });
        COpenResult opened;
        const UString relative = ToUString(QDir::current().relativeFilePath(fixture.path()));
        UString path(L"./");
        path += relative;
        QCOMPARE(panel.Create(nullptr, nullptr, 1000, path, UString(), nullptr, &app.AppState,
                              false, opened),
                 S_OK);
        QCOMPARE(creates, 1);
        QVERIFY(panel.PanelCreated);
        QVERIFY(panel._lastFocusedIsList);
        QCOMPARE(panel.GetListViewMode(), 3u);
        panel.ReleasePanel();
        QVERIFY(!panel.GetFolder());
    }

    void panelCreateMissingArchiveAndPresentationFailure() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        CPanel panel;
        int creates = 0;
        panel.AttachCreationPresentation([&](CPanel &) {
            ++creates;
            return false;
        });
        COpenResult opened;
        QCOMPARE(panel.Create(nullptr, nullptr, 1000, ToUString(fixture.path()), UString(), nullptr,
                              nullptr, true, opened),
                 S_OK);
        QVERIFY(!opened.ArchiveIsOpened);
        QVERIFY(!panel.PanelCreated);
        QCOMPARE(creates, 0);
        QCOMPARE(panel.Create(nullptr, nullptr, 1000, ToUString(fixture.path()), UString(), nullptr,
                              nullptr, false, opened),
                 E_FAIL);
        QVERIFY(!panel.PanelCreated);
        QCOMPARE(creates, 1);
    }

    void disableAllPanelsBeforeAnyFolderRelease() {
        CApp app;
        app.NumPanels = 1;
        QStringList released;
        for (unsigned i = 0; i < kNumPanelsMax; ++i) {
            app.Panels[i].SetNewFolder(new ReleaseObservedFolder([&, i] {
                QVERIFY(!app.Panels[0].ShouldProcessNotifications());
                QVERIFY(!app.Panels[1].ShouldProcessNotifications());
                if (i == 0)
                    QVERIFY(app.Panels[1].GetFolder());
                released.append(QString::number(i));
            }));
        }
        app.ReleaseApp();
        QCOMPARE(released, QStringList({"0", "1"}));
        QVERIFY(!app.Panels[0].GetFolder());
        QVERIFY(!app.Panels[1].GetFolder());
        app.ReleaseApp();
        QCOMPARE(released.size(), 2);
        CApp independent;
        QVERIFY(independent.Panels[0].ShouldProcessNotifications());
        QVERIFY(independent.Panels[1].ShouldProcessNotifications());
    }

    void panelSettingsUseSourceKeysAndIndependentSlots() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString settings = fixture.filePath("panel settings.ini");
        CListMode mode;
        mode.Panels[0] = 0;
        mode.Panels[1] = 2;
        mode.Save(settings);
        SavePanelPath(0, UString(L"/папка с пробелом/"), settings);
        SavePanelPath(1, UString(L"/other/"), settings);
        SaveFlatView(0, true, settings);
        SaveFlatView(1, false, settings);
        CListMode read;
        read.Read(settings);
        QCOMPARE(read.Panels[0], 0u);
        QCOMPARE(read.Panels[1], 2u);
        UString path;
        QVERIFY(ReadPanelPath(0, path, settings));
        QCOMPARE(ToQString(path), QStringLiteral("/папка с пробелом/"));
        QVERIFY(ReadPanelPath(1, path, settings));
        QCOMPARE(ToQString(path), QStringLiteral("/other/"));
        QVERIFY(ReadFlatView(0, settings));
        QVERIFY(!ReadFlatView(1, settings));
        QSettings key(settings, QSettings::IniFormat);
        QCOMPARE(key.value("ListMode").toUInt(), 0x200u);
        key.setValue("ListMode", QStringLiteral("invalid"));
        key.setValue("FM/FlatViewArc0", QStringLiteral("invalid"));
        key.sync();
        read.Read(settings);
        QCOMPARE(read.Panels[0], kListMode_Report);
        QCOMPARE(read.Panels[1], kListMode_Report);
        QVERIFY(!ReadFlatView(0, settings));
    }
};

QTEST_MAIN(AppLifecycleTests)
#include "app_lifecycle_tests.moc"
