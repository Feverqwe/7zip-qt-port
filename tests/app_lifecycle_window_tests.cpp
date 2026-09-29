#include "qtarchivefoldermanager.h"
#include "window_state_support.h"

namespace {
class ArchiveOwnerPresentation final : public IQtAppPresentation {
  public:
    int creates = 0;
    void Initialize(CApp &) override {}
    bool CreatePanel(CPanel &panel, unsigned) override {
        ++creates;
        panel.LoadFullPath();
        return true;
    }
    CPanelCallback *PanelCallback(unsigned) override { return nullptr; }
    void SetListSettings(unsigned, const CFmSettings &) override {}
};
} // namespace

class AppLifecycleWindowTests : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
  private slots:
    void archiveStartupUsesJoinedSourceOwner_data() {
        QTest::addColumn<int>("result");
        QTest::newRow("success") << int(S_OK);
        QTest::newRow("failure") << int(E_FAIL);
        QTest::newRow("cancel") << int(E_ABORT);
    }

    void archiveStartupUsesJoinedSourceOwner() {
        QFETCH(int, result);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString archive = fixture.filePath("архив startup.zip");
        ControlledBackend backend;
        QVERIFY(backend.prepareArchiveFixture(archive));
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        ArchiveOwnerPresentation presentation;
        CApp app;
        app.NumPanels = 1;
        for (auto &panel : app.Panels)
            panel.AttachFolderManager(manager, qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        backend.queueListAction([&] {
            if (result == S_OK)
                backend.completeList({ArchiveEntry{"folder/имя с пробелом.txt", 1, 1}});
            else if (result == E_ABORT)
                backend.cancel();
            else
                backend.fail(QStringLiteral("startup failure"));
        });
        COpenResult opened;
        int widths[2] = {300, 300};
        QCOMPARE(app.Create(nullptr, ToUString(archive), UString(), widths, true, opened,
                            presentation, qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")),
                 HRESULT(result));
        QVERIFY(!backend.busy());
        QCOMPARE(backend.listCalls, 1);
        QCOMPARE(presentation.creates, result == S_OK ? 1 : 0);
        QCOMPARE(app.Panels[0].PanelCreated, result == S_OK);
        QCOMPARE(opened.ArchiveIsOpened, result == S_OK);
        if (result == S_OK) {
            QVERIFY(app.Panels[0].IsArcFolder());
            QCOMPARE(app.Panels[0].GetParentFolders().Size(), 1u);
        }
        app.ReleaseApp();
        QVERIFY(!app.Panels[0].GetFolder());
        QVERIFY(app.Panels[0].GetParentFolders().IsEmpty());
    }

    void init() {
        QVERIFY(QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")) ||
                !QFileInfo::exists(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")));
    }

    void legacyPreferencesMigrateAndArchiveSaveUsesOuterParent() {
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        {
            QSettings key(settings, QSettings::IniFormat);
            key.setValue("view/mode", 2);
            key.setValue("view/flatArchive", true);
        }
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        ControlledBackend backend;
        {
            MainWindow window(&backend);
            QVERIFY(action(window, "viewMode2Action")->isChecked());
            openListedArchive(window, backend, fixture.filePath("архив с пробелом.zip"),
                              {ArchiveEntry{"folder/item.txt", 1, 1}});
            QVERIFY(action(window, "flatViewAction")->isChecked());
            QCOMPARE(table(window)->rowCount(), 2);
            bool childVisible = false;
            for (int row = 0; row < table(window)->rowCount(); ++row)
                childVisible |= table(window)->item(row, 0)->data(Qt::UserRole).toString() ==
                                QStringLiteral("folder/item.txt");
            QVERIFY(childVisible);
        }
        QSettings key(settings, QSettings::IniFormat);
        QCOMPARE(key.value("ListMode").toUInt() & 0xFF, 2u);
        QVERIFY(key.value("FM/FlatViewArc0").toBool());
        QCOMPARE(QDir::cleanPath(key.value("PanelPath0").toString()), fixture.path());
    }

    void originalPerPanelPreferencesWinOverLegacyKeys() {
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        {
            QSettings key(settings, QSettings::IniFormat);
            key.setValue("view/mode", 2);
            key.setValue("view/flatArchive", true);
        }
        CListMode modes;
        modes.Panels[0] = 1;
        modes.Panels[1] = 0;
        modes.Save(settings);
        SaveFlatView(0, false, settings);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        ControlledBackend backend;
        {
            MainWindow window(&backend);
            QVERIFY(action(window, "viewMode1Action")->isChecked());
            openListedArchive(window, backend, fixture.filePath("archive.zip"),
                              {ArchiveEntry{"folder/item.txt", 1, 1}});
            QVERIFY(!action(window, "flatViewAction")->isChecked());
            QCOMPARE(table(window)->rowCount(), 1);
            action(window, "viewMode0Action")->trigger();
        }
        modes.Read(settings);
        QCOMPARE(modes.Panels[0], 0u);
        QCOMPARE(modes.Panels[1], 0u);
        QVERIFY(!ReadFlatView(0, settings));
    }
};

QTEST_MAIN(AppLifecycleWindowTests)
#include "app_lifecycle_window_tests.moc"
