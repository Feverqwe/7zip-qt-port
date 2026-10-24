#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "filetable.h"
#include "layout_inspection.h"
#include "panel_model_support.h"
#include "qtlangcontrols.h"
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
    void providerNamesSurviveLanguageChanges_data() {
        QTest::addColumn<unsigned>("pane");
        QTest::newRow("left") << 0u;
        QTest::newRow("right") << 1u;
    }
    void providerNamesSurviveLanguageChanges() {
        QFETCH(unsigned, pane);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SaveRegLang(UString(L"-"), settings);
        ReloadLang(settings);
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *ui = window.findChild<QWidget *>(QString("filePanel%1").arg(pane));
        auto *view = static_cast<FileTable *>(ui->findChild<FileTable *>("archiveTable"));
        auto *spec = new SchemaFolder;
        const PROPID customId = 65536;
        const QString customName = QStringLiteral("Method 空");
        spec->properties.append({customId, VT_BSTR, S_OK, ToUString(customName)});
        spec->rawEnabled = false;
        CMyComPtr<IFolderFolder> provider = spec;
        auto &panel = *view->sourcePanel;
        panel.Set_AutoRefresh_Mode(false);
        panel.SetNewFolder(provider);
        panel.LoadFullPath();
        panel.OnReload();
        const int custom = propertyColumn(*view, customId, false);
        const int name = propertyColumn(*view, kpidName, false);
        QVERIFY(custom >= 0);
        QVERIFY(name >= 0);
        QCOMPARE(view->model()->headerData(custom, Qt::Horizontal).toString(), customName);
        const unsigned queries = spec->propertyInfoQueries;
        for (const char *language : {"ru", "-"}) {
            SaveRegLang(UString(language), settings);
            ReloadLang(settings);
            QtNotifyLanguageChanged();
            QCOMPARE(view->model()->headerData(custom, Qt::Horizontal).toString(), customName);
            QCOMPARE(view->model()->headerData(name, Qt::Horizontal).toString(),
                     QString(language) == "ru" ? QStringLiteral("Имя") : QString("Name"));
            QCOMPARE(panel.GetFolder(), (IFolderFolder *)provider);
            QCOMPARE(spec->propertyInfoQueries, queries);
        }
    }
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
        auto *view = static_cast<FileTable *>(ui->findChild<FileTable *>("archiveTable"));
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
        auto *peer = static_cast<FileTable *>(peerUi->findChild<FileTable *>("archiveTable"));
        CMyComPtr<IFolderFolder> peerProvider = peer->sourcePanel->GetFolder();
        peer->sourcePanel->Set_AutoRefresh_Mode(false);
        const int peerRows = peer->rowCount();
        peerUi->findChild<QLineEdit *>("archivePath")->setFocus();
        QWidget *const focused = QApplication::focusWidget();
        QVERIFY(focused);
        const int generic = propertyColumn(*view, spec->rawId, false);
        const int raw = propertyColumn(*view, spec->rawId, true);
        QVERIFY(generic >= 0);
        QVERIFY(raw >= 0);
        QVERIFY(generic != raw);
        QCOMPARE(panel.GetVisibleColumns().Size(), 4u);
        int rawSource = -1;
        for (unsigned index = 0; index < panel.GetColumns().Size(); ++index)
            if (panel.GetColumns()[index].IsRawProp)
                rawSource = int(index);
        QVERIFY(rawSource >= 0);
        for (int row = 0; row < view->rowCount(); ++row) {
            const UInt32 source = view->model()->index(row, 0).data(Qt::UserRole + 6).toUInt();
            QCOMPARE(view->model()->index(row, raw).data().toString(),
                     panel.GetItemText(source, panel.GetColumns()[rawSource]));
            QVERIFY(view->model()->index(row, raw).data().toString() !=
                    view->model()->index(row, generic).data().toString());
            QCOMPARE(
                view->model()->index(row, raw).data(Qt::TextAlignmentRole).value<Qt::Alignment>(),
                GetColumnAlign(spec->rawId, VT_EMPTY));
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
            menu->close();
            QCOMPARE(menu->actions().size(), int(panel.GetColumns().Size()));
            QVERIFY(menu->actions()[rawSource]->isEnabled());
        });
        QVERIFY(QMetaObject::invokeMethod(view->horizontalHeader(), "customContextMenuRequested",
                                          Qt::DirectConnection, Q_ARG(QPoint, QPoint(10, 5))));
        QVERIFY(panel.GetColumns().IsEqualTo(catalog));
        window.activateWindow();
        QTRY_COMPARE(QApplication::focusWidget(), focused);
        const unsigned queries = spec->rawQueries;
        {
            CPanel::CDisableNotify guard(panel);
            SetPanelData(view->model()->index(0, raw), QVariant(), Qt::DisplayRole);
            view->model()->index(0, raw).data().toString();
            QCOMPARE(spec->rawQueries, queries);
        }
        QCOMPARE(view->model()->index(0, raw).data().toString(),
                 reserved ? QString("deadbeef") : QString("DEADBEEF"));
        for (HRESULT result : {E_FAIL, E_ABORT}) {
            spec->rawResult = result;
            SetPanelData(view->model()->index(0, raw), QVariant(), Qt::DisplayRole);
            QVERIFY(view->model()->index(0, raw).data().toString().isEmpty());
        }
        spec->rawResult = S_OK;
        SetPanelData(view->model()->index(0, raw), QVariant(), Qt::DisplayRole);
        QCOMPARE(view->model()->index(0, raw).data().toString(),
                 reserved ? QString("deadbeef") : QString("DEADBEEF"));
        auto toggleRaw = [&] {
            QTimer::singleShot(0, [&] {
                auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
                if (menu) {
                    menu->actions()[rawSource]->trigger();
                    menu->close();
                }
            });
            return QMetaObject::invokeMethod(view->horizontalHeader(), "customContextMenuRequested",
                                             Qt::DirectConnection, Q_ARG(QPoint, QPoint(10, 5)));
        };
        QVERIFY(toggleRaw());
        QVERIFY(!panel.GetColumns()[rawSource].IsVisible);
        QCOMPARE(panel.GetVisibleColumns().Size(), 3u);
        QVERIFY(view->isColumnHidden(generic));
        QVERIFY(!view->isColumnHidden(raw));
        QCOMPARE(view->selectionModel()->selectedRows().size(), 2);
        QVERIFY(toggleRaw());
        QCOMPARE(panel.GetVisibleColumns().Size(), 4u);
        const int appended = view->columnForVisibleProperty(3);
        QVERIFY(appended >= 0);
        QVERIFY(appended != raw);
        QVERIFY(!view->isColumnHidden(appended));
        QCOMPARE(view->horizontalHeader()->sectionSize(appended), int(catalog[rawSource].Width));
        QCOMPARE(view->horizontalHeader()->logicalIndex(view->columnCount() - 1), appended);
        for (int row = 0; row < view->rowCount(); ++row) {
            QVERIFY(view->model()->index(row, appended).isValid());
            QCOMPARE(view->model()->index(row, appended).data(Qt::UserRole + 6),
                     view->model()->index(row, 0).data(Qt::UserRole + 6));
            QCOMPARE(view->model()->index(row, appended).data().toString(),
                     view->model()->index(row, raw).data().toString());
            QCOMPARE(view->model()
                         ->index(row, appended)
                         .data(Qt::TextAlignmentRole)
                         .value<Qt::Alignment>(),
                     GetColumnAlign(spec->rawId, VT_EMPTY));
        }
        view->horizontalHeader()->resizeSection(appended, 211);
        QCOMPARE(panel.GetVisibleColumns()[2].Width, UInt32(193));
        QCOMPARE(panel.GetVisibleColumns()[3].Width, UInt32(211));
        QCOMPARE(view->selectionModel()->selectedRows().size(), 2);
        QCOMPARE(window.focusWidget(), focused);
        window.activateWindow();
        QTRY_COMPARE(QApplication::focusWidget(), focused);
        QCOMPARE(panel.GetFolder(), (IFolderFolder *)provider);
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*view, window));
            QVERIFY(UiInspection::fullyVisible(*peer, window));
            QVERIFY(UiInspection::writeReport(
                window,
                QString("column-duplicates-%1-%2").arg(QTest::currentDataTag()).arg(size.width())));
        }
        QVERIFY(toggleRaw());
        QVERIFY(view->isColumnHidden(raw));
        QVERIFY(!view->isColumnHidden(appended));
        QCOMPARE(view->columnForVisibleProperty(2), appended);
        QVERIFY(toggleRaw());
        QCOMPARE(panel.GetVisibleColumns().Size(), 4u);
        QCOMPARE(view->columnForVisibleProperty(3), raw);
        QCOMPARE(view->horizontalHeader()->sectionSize(raw), int(catalog[rawSource].Width));
        QCOMPARE(view->selectionModel()->selectedRows().size(), 2);
        panel.OnReload();
        QCOMPARE(panel.GetFolder(), (IFolderFolder *)provider);
        QCOMPARE(view->selectionModel()->selectedRows().size(), 2);
        // Saved duplicate IDs retain upstream's first-match initialization.
        QCOMPARE(view->horizontalHeader()->sectionSize(raw), int(catalog[rawSource].Width));
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
