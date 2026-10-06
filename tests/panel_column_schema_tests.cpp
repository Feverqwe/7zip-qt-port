#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "layout_inspection.h"
#include "schema_folder.h"
#include "window_state_support.h"

class PanelColumnSchemaTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static int propertyColumn(FileTable &view, PROPID id, bool raw) {
        for (int col = 0; col < view.columnCount(); ++col)
            if (view.propertyId(col) == id && view.isRawProperty(col) == raw)
                return col;
        return -1;
    }
  private slots:
    void owningDescriptorSurvivesPublication_data() {
        QTest::addColumn<unsigned>("pane");
        QTest::addColumn<bool>("russian");
        QTest::addColumn<bool>("reserved");
        for (unsigned pane : {0u, 1u})
            for (bool russian : {false, true})
                for (bool reserved : {false, true})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(pane).arg(russian).arg(reserved)))
                        << pane << russian << reserved;
    }
    void owningDescriptorSurvivesPublication() {
        QFETCH(unsigned, pane);
        QFETCH(bool, russian);
        QFETCH(bool, reserved);
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
        auto *spec = new SchemaFolder;
        spec->rawId = reserved ? kpidSize : kpidCRC;
        CMyComPtr<IFolderFolder> provider = spec;
        auto &panel = *view->sourcePanel;
        panel.Set_AutoRefresh_Mode(false);
        panel.SetNewFolder(provider);
        panel.LoadFullPath();
        panel.OnReload();
        QCOMPARE(view->rowCount(), 2);
        auto *peerUi = window.findChild<QWidget *>(QString("filePanel%1").arg(1 - pane));
        auto *peer = static_cast<FileTable *>(peerUi->findChild<QTableWidget *>("archiveTable"));
        CMyComPtr<IFolderFolder> peerProvider = peer->sourcePanel->GetFolder();
        peer->sourcePanel->Set_AutoRefresh_Mode(false);
        const int peerRows = peer->rowCount();
        peerUi->findChild<QLineEdit *>("archivePath")->setFocus();
        const int generic = propertyColumn(*view, spec->rawId, false);
        const int raw = propertyColumn(*view, spec->rawId, true);
        QVERIFY(generic >= 0);
        QVERIFY(raw >= 7);
        QVERIFY(generic != raw);
        QCOMPARE(panel.GetVisibleColumns().Size(), 4u);
        int rawSource = -1;
        for (unsigned index = 0; index < panel.GetColumns().Size(); ++index)
            if (panel.GetColumns()[index].IsRawProp)
                rawSource = int(index);
        QVERIFY(rawSource >= 0);
        for (int row = 0; row < view->rowCount(); ++row) {
            const UInt32 source = view->item(row, 0)->data(Qt::UserRole + 6).toUInt();
            QCOMPARE(view->item(row, raw)->text(),
                     panel.GetItemText(source, panel.GetColumns()[rawSource]));
            QVERIFY(view->item(row, raw)->text() != view->item(row, generic)->text());
            QCOMPARE(view->item(row, raw)->textAlignment(), GetColumnAlign(spec->rawId, VT_EMPTY));
        }
        panel.SelectAll(true);
        const CPropColumns catalog = panel.GetColumns();
        view->horizontalHeader()->resizeSection(generic, 137);
        view->horizontalHeader()->resizeSection(raw, 193);
        QVERIFY(panel.GetColumns().IsEqualTo(catalog));
        bool foundRaw = false;
        for (unsigned index = 0; index < panel.GetVisibleColumns().Size(); ++index) {
            const auto &column = panel.GetVisibleColumns()[index];
            if (column.ID == spec->rawId && column.IsRawProp) {
                QCOMPARE(column.Width, UInt32(193));
                foundRaw = true;
            }
        }
        QVERIFY(foundRaw);
        QTimer::singleShot(0, [&] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            QVERIFY(menu);
            QCOMPARE(menu->actions().size(), int(panel.GetColumns().Size()));
            QVERIFY(!menu->actions()[rawSource]->isEnabled());
            menu->close();
        });
        QVERIFY(QMetaObject::invokeMethod(view->horizontalHeader(), "customContextMenuRequested",
                                          Qt::DirectConnection, Q_ARG(QPoint, QPoint(10, 5))));
        QVERIFY(panel.GetColumns().IsEqualTo(catalog));
        const unsigned queries = spec->rawQueries;
        {
            CPanel::CDisableNotify guard(panel);
            view->item(0, raw)->setData(Qt::DisplayRole, QVariant());
            view->item(0, raw)->text();
            QCOMPARE(spec->rawQueries, queries);
        }
        QCOMPARE(view->item(0, raw)->text(), reserved ? QString("deadbeef") : QString("DEADBEEF"));
        for (HRESULT result : {E_FAIL, E_ABORT}) {
            spec->rawResult = result;
            view->item(0, raw)->setData(Qt::DisplayRole, QVariant());
            QVERIFY(view->item(0, raw)->text().isEmpty());
        }
        spec->rawResult = S_OK;
        view->item(0, raw)->setData(Qt::DisplayRole, QVariant());
        QCOMPARE(view->item(0, raw)->text(), reserved ? QString("deadbeef") : QString("DEADBEEF"));
        panel.OnReload();
        QCOMPARE(panel.GetFolder(), (IFolderFolder *)provider);
        QCOMPARE(view->selectionModel()->selectedRows().size(), 2);
        // Saved duplicate IDs retain upstream's first-match initialization.
        QCOMPARE(view->horizontalHeader()->sectionSize(raw), 100);
        QCOMPARE(peer->sourcePanel->GetFolder(), (IFolderFolder *)peerProvider);
        QCOMPARE(peer->rowCount(), peerRows);
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*view, window));
            QVERIFY(UiInspection::writeReport(
                window,
                QString("column-schema-%1-%2").arg(QTest::currentDataTag()).arg(size.width())));
        }
        QVERIFY(control.open(QIODevice::ReadOnly));
        QCOMPARE(control.readAll(), QByteArray("untouched"));
        QCOMPARE(backend.listCalls, 0);
    }
};
QTEST_MAIN(PanelColumnSchemaTests)
#include "panel_column_schema_tests.moc"
