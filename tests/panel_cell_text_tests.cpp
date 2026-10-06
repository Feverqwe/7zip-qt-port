#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "layout_inspection.h"
#include "schema_folder.h"
#include "window_state_support.h"
#include <QPersistentModelIndex>

class PanelCellTextTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static QString uncached(FileTable &view, int column) {
        view.item(0, column)->setData(Qt::DisplayRole, QVariant());
        return view.item(0, column)->text();
    }
    static void reportViews(MainWindow &window, QWidget &visibleView, FileTable &peer,
                            const QString &phase) {
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(visibleView, window));
            QVERIFY(UiInspection::fullyVisible(peer, window));
            QVERIFY(UiInspection::writeReport(window, QString("cell-text-%1-%2-%3")
                                                          .arg(QTest::currentDataTag(), phase)
                                                          .arg(size.width())));
        }
    }
  private slots:
    void publishedIndicesUseSourceProperties_data() {
        QTest::addColumn<unsigned>("pane");
        QTest::addColumn<bool>("russian");
        QTest::addColumn<bool>("icons");
        for (unsigned pane : {0u, 1u})
            for (bool russian : {false, true})
                for (bool icons : {false, true})
                    QTest::newRow(qPrintable(QString("%1-%2-%3").arg(pane).arg(russian).arg(icons)))
                        << pane << russian << icons;
    }
    void publishedIndicesUseSourceProperties() {
        QFETCH(unsigned, pane);
        QFETCH(bool, russian);
        QFETCH(bool, icons);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SaveRegLang(UString(russian ? L"ru" : L"-"), settings);
        ReloadLang(settings);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QFile control(fixture.filePath(QStringLiteral("control 文件.txt")));
        QVERIFY(control.open(QIODevice::WriteOnly));
        QCOMPARE(control.write("untouched"), qint64(9));
        control.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *ui = window.findChild<QWidget *>(QString("filePanel%1").arg(pane));
        auto *view = static_cast<FileTable *>(ui->findChild<QTableWidget *>("archiveTable"));
        ui->findChild<QLineEdit *>("archivePath")->setFocus();
        action(window, "openRootAction")->trigger();
        if (icons)
            action(window, "viewMode0Action")->trigger();
        auto *spec = new SchemaFolder;
        CMyComPtr<IFolderFolder> provider = spec;
        auto &panel = *view->sourcePanel;
        panel.Set_AutoRefresh_Mode(false);
        panel.SetNewFolder(provider);
        panel.LoadFullPath();
        panel.OnReload();
        QCOMPARE(view->rowCount(), 2);
        panel.SelectAll(true);
        view->setSortingEnabled(false);
        const int generic = view->columnForCatalogProperty(2);
        const int raw = view->columnForCatalogProperty(3);
        QVERIFY(generic >= 0 && raw >= 0 && generic != raw);
        auto *peerUi = window.findChild<QWidget *>(QString("filePanel%1").arg(1 - pane));
        auto *peer = static_cast<FileTable *>(peerUi->findChild<QTableWidget *>("archiveTable"));
        peer->sourcePanel->Set_AutoRefresh_Mode(false);
        CMyComPtr<IFolderFolder> peerProvider = peer->sourcePanel->GetFolder();
        const int peerRows = peer->rowCount();
        peerUi->findChild<QLineEdit *>("archivePath")->setFocus();
        QWidget *const focused = window.focusWidget();
        const UInt32 sourceIndex = view->item(0, 0)->data(Qt::UserRole + 6).toUInt();
        const QString sourceName =
            ToQString(sourceIndex ? spec->items->secondName : spec->items->firstName);
        for (HRESULT result : {S_FALSE, E_FAIL, E_ABORT}) {
            spec->items->countResult = result;
            const unsigned queries = spec->countQueries;
            QCOMPARE(uncached(*view, 0), sourceName);
            QCOMPARE(uncached(*view, generic), QString("12345678"));
            QCOMPARE(uncached(*view, raw), QString("DEADBEEF"));
            QCOMPARE(spec->countQueries, queries);
            spec->crcResult = result;
            spec->rawResult = result;
            QCOMPARE(uncached(*view, generic), QString("Error: "));
            QCOMPARE(uncached(*view, raw), QString());
            QCOMPARE(spec->countQueries, queries);
            spec->crcResult = spec->rawResult = S_OK;
        }
        spec->items->countResult = S_OK;
        QCOMPARE(uncached(*view, generic), QString("12345678"));
        QCOMPARE(uncached(*view, raw), QString("DEADBEEF"));
        const unsigned rawQueries = spec->rawQueries;
        {
            CPanel::CDisableNotify guard(panel);
            QCOMPARE(view->item(0, raw)->text(), QString("DEADBEEF"));
            QCOMPARE(uncached(*view, raw), QString());
            QCOMPARE(spec->rawQueries, rawQueries);
        }
        const auto savedFolder = view->textFolder;
        view->textFolder = peerProvider;
        QCOMPARE(uncached(*view, raw), QString());
        QCOMPARE(spec->rawQueries, rawQueries);
        view->textFolder = savedFolder;
        QCOMPARE(uncached(*view, raw), QString("DEADBEEF"));
        panel.OnReload();
        QCOMPARE(panel.GetFolder(), (IFolderFolder *)provider);
        QCOMPARE(view->rowCount(), 2);
        QCOMPARE(view->selectionModel()->selectedRows().size(), 2);
        QCOMPARE(window.focusWidget(), focused);
        QCOMPARE(peer->sourcePanel->GetFolder(), (IFolderFolder *)peerProvider);
        QCOMPARE(peer->rowCount(), peerRows);
        QWidget &visibleView =
            icons ? static_cast<QWidget &>(*ui->findChild<QListView *>("iconList")) : *view;
        reportViews(window, visibleView, *peer, "recovery");
        const QPersistentModelIndex previous = view->model()->index(0, raw);
        auto *replacement = new SchemaFolder;
        replacement->items->firstName = L"replacement δ";
        replacement->items->secondName = L"replacement 文件.txt";
        panel.SetNewFolder(replacement);
        const unsigned previousRawQueries = spec->rawQueries;
        panel.LoadFullPath();
        panel.OnReload();
        QVERIFY(!previous.isValid());
        QCOMPARE(view->rowCount(), 2);
        const UInt32 replacementIndex = view->item(0, 0)->data(Qt::UserRole + 6).toUInt();
        QCOMPARE(uncached(*view, 0), ToQString(replacementIndex ? replacement->items->secondName
                                                                : replacement->items->firstName));
        reportViews(window, visibleView, *peer, "rebind");
        QCOMPARE(spec->rawQueries, previousRawQueries);
        QCOMPARE(window.focusWidget(), focused);
        QCOMPARE(peer->sourcePanel->GetFolder(), (IFolderFolder *)peerProvider);
        QCOMPARE(peer->rowCount(), peerRows);
        QVERIFY(control.open(QIODevice::ReadOnly));
        QCOMPARE(control.readAll(), QByteArray("untouched"));
        QCOMPARE(backend.listCalls, 0);
    }
};
QTEST_MAIN(PanelCellTextTests)
#include "panel_cell_text_tests.moc"
