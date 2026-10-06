#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "layout_inspection.h"
#include "window_state_support.h"

class PanelColumnInitializationTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static QList<PROPID> visibleOrder(FileTable &view) {
        QList<PROPID> result;
        for (int visual = 0; visual < view.columnCount(); ++visual) {
            const int logical = view.horizontalHeader()->logicalIndex(visual);
            if (!view.isColumnHidden(logical))
                result.append(view.propertyId(logical));
        }
        return result;
    }
    static void makeFixture(QTemporaryDir &fixture) {
        QVERIFY(fixture.isValid());
        QFile file(fixture.filePath(QStringLiteral("control 文件.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("untouched"), qint64(9));
    }
    static FileTable *paneView(MainWindow &window, unsigned pane) {
        return static_cast<FileTable *>(
            window.findChild<QWidget *>(QString("filePanel%1").arg(pane))
                ->findChild<QTableWidget *>("archiveTable"));
    }
    static void navigate(MainWindow &window, unsigned pane, const QString &path) {
        auto *address = window.findChild<QWidget *>(QString("filePanel%1").arg(pane))
                            ->findChild<QLineEdit *>("archivePath");
        address->setFocus();
        address->setText(path);
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_COMPARE(paneView(window, pane)->rowCount(), 1);
        paneView(window, pane)->sourcePanel->Set_AutoRefresh_Mode(false);
    }
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void saveProjectionKeepsCatalogAndPersistsVisibleMetadata() {
        QTemporaryDir fixture;
        makeFixture(fixture);
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        navigate(window, 0, fixture.path());
        auto &view = *paneView(window, 0);
        auto &panel = *view.sourcePanel;
        const CPropColumns catalog = panel.GetColumns();
        const CPropColumns logical = panel.GetVisibleColumns();
        view.horizontalHeader()->resizeSection(0, 187);
        view.horizontalHeader()->moveSection(view.horizontalHeader()->visualIndex(0),
                                             view.columnCount() - 1);
        QVERIFY(panel.GetColumns().IsEqualTo(catalog));
        QCOMPARE(panel.GetVisibleColumns().Size(), logical.Size());
        for (unsigned index = 0; index < logical.Size(); ++index)
            QCOMPARE(panel.GetVisibleColumns()[index].ID, logical[index].ID);
        QCOMPARE(panel.GetVisibleColumns()[0].Width, UInt32(187));
        QCOMPARE(panel.GetVisibleColumns()[0].Order, int(logical.Size() - 1));
        CListViewInfo saved;
        saved.Read(panel.GetFolderTypeID(), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QVERIFY(saved.IsLoaded);
        QVERIFY(saved.Columns.Size() >= logical.Size());
        QCOMPARE(saved.Columns[logical.Size() - 1].PropID, PROPID(kpidName));
        QCOMPARE(saved.Columns[logical.Size() - 1].Width, UInt32(187));
        QCOMPARE(panel.InitColumns(), S_OK);
        const int name = panel.GetColumns().FindItem_for_PropID(kpidName);
        QVERIFY(name >= 0);
        QCOMPARE(panel.GetColumns()[name].Width, UInt32(187));
        QCOMPARE(panel.GetColumns()[name].Order, int(logical.Size() - 1));
        QCOMPARE(backend.listCalls, 0);
    }
    void directInitializationClearsSourceMarks() {
        QTemporaryDir fixture;
        makeFixture(fixture);
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        navigate(window, 0, fixture.path());
        auto &view = *paneView(window, 0);
        auto &panel = *view.sourcePanel;
        panel.SelectAll(true);
        QVERIFY(panel.IsItemMarked(0));
        CMyComPtr<IFolderFolder> provider = panel.GetFolder();
        const auto before = visibleOrder(view);
        const CPropColumns columns = panel.GetVisibleColumns();
        QCOMPARE(panel.InitColumns(), S_OK);
        QVERIFY(!panel.IsItemMarked(0));
        QCOMPARE(panel.GetFolder(), (IFolderFolder *)provider);
        QVERIFY(panel.GetVisibleColumns().IsEqualTo(columns));
        QCOMPARE(visibleOrder(view), before);
        QCOMPARE(view.rowCount(), 1);
        QCOMPARE(view.selectionModel()->selectedRows().size(), 1);
        panel.OnReload();
        QCOMPARE(view.rowCount(), 1);
        QCOMPARE(view.selectionModel()->selectedRows().size(), 1);
        QVERIFY(panel.IsItemMarked(0));
        QCOMPARE(backend.listCalls, 0);
    }
    void savedVisualOrderSurvivesPublication_data() {
        QTest::addColumn<unsigned>("pane");
        QTest::addColumn<bool>("russian");
        for (unsigned pane : {0u, 1u})
            for (bool russian : {false, true})
                QTest::newRow(qPrintable(QString("%1-%2").arg(pane).arg(russian)))
                    << pane << russian;
    }
    void savedVisualOrderSurvivesPublication() {
        QFETCH(unsigned, pane);
        QFETCH(bool, russian);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        SaveRegLang(UString(russian ? L"ru" : L"-"), settings);
        ReloadLang(settings);
        QTemporaryDir fixture;
        makeFixture(fixture);
        ControlledBackend backend;
        QList<PROPID> expected;
        {
            MainWindow window(&backend);
            window.show();
            QVERIFY(QTest::qWaitForWindowActive(&window));
            action(window, "twoPanelsAction")->trigger();
            navigate(window, 0, fixture.path());
            navigate(window, 1, fixture.path());
            auto &view = *paneView(window, pane);
            auto &peer = *paneView(window, 1 - pane);
            const auto peerOrder = visibleOrder(peer);
            CMyComPtr<IFolderFolder> provider = view.sourcePanel->GetFolder();
            view.horizontalHeader()->resizeSection(0, 187);
            view.horizontalHeader()->moveSection(view.horizontalHeader()->visualIndex(0),
                                                 view.columnCount() - 1);
            expected = visibleOrder(view);
            QCOMPARE(expected.last(), PROPID(kpidName));
            view.selectRow(0);
            view.sourcePanel->OnReload();
            QCOMPARE(visibleOrder(view), expected);
            QCOMPARE(view.horizontalHeader()->sectionSize(0), 187);
            QCOMPARE(view.sourcePanel->GetVisibleColumns()[0].ID, PROPID(kpidName));
            QCOMPARE(view.sourcePanel->GetFolder(), (IFolderFolder *)provider);
            QCOMPARE(visibleOrder(peer), peerOrder);
            QCOMPARE(view.selectionModel()->selectedRows().size(), 1);
        }
        MainWindow reopened(&backend);
        reopened.show();
        QVERIFY(QTest::qWaitForWindowActive(&reopened));
        navigate(reopened, pane, fixture.path());
        auto &view = *paneView(reopened, pane);
        QCOMPARE(visibleOrder(view), expected);
        QCOMPARE(view.horizontalHeader()->sectionSize(0), 187);
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            reopened.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(view, reopened));
            QVERIFY(UiInspection::writeReport(
                reopened,
                QString("column-init-%1-%2").arg(QTest::currentDataTag()).arg(size.width())));
        }
        QFile file(fixture.filePath(QStringLiteral("control 文件.txt")));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("untouched"));
        QCOMPARE(backend.listCalls, 0);
    }
};
QTEST_MAIN(PanelColumnInitializationTests)
#include "panel_column_initialization_tests.moc"
