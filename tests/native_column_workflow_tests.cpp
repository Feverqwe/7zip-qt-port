#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "archivecodeccatalog.h"
#include "layout_inspection.h"
#include "schema_folder.h"
#include "window_state_support.h"
#include <QPersistentModelIndex>
#include <QStandardPaths>

class NativeColumnWorkflowTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    QString engine_, module_;
    quint32 version_ = 0;
    static QByteArray read(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }
    static FileTable &view(MainWindow &window, unsigned pane) {
        return *static_cast<FileTable *>(
            window.findChild<QWidget *>(QString("filePanel%1").arg(pane))
                ->findChild<QTableWidget *>("archiveTable"));
    }
    static void focusPath(MainWindow &window, unsigned pane) {
        window.findChild<QWidget *>(QString("filePanel%1").arg(pane))
            ->findChild<QLineEdit *>("archivePath")
            ->setFocus();
    }
    static void navigate(MainWindow &window, unsigned pane, const QString &path) {
        focusPath(window, pane);
        auto *address = window.findChild<QWidget *>(QString("filePanel%1").arg(pane))
                            ->findChild<QLineEdit *>("archivePath");
        address->setText(path);
        QTest::keyClick(address, Qt::Key_Return);
        QTRY_COMPARE(view(window, pane).rowCount(), 2);
        view(window, pane).sourcePanel->Set_AutoRefresh_Mode(false);
    }
    static void inspect(MainWindow &window, FileTable &list, FileTable &peer,
                        const QString &phase) {
        const auto &columns = list.sourcePanel->GetColumns();
        FOR_VECTOR(i, columns) {
            const int column = list.columnForCatalogProperty(i);
            QVERIFY(column >= 0);
            QCOMPARE(list.isColumnHidden(column), !columns[i].IsVisible);
            QCOMPARE(list.horizontalHeaderItem(column)->text(), ToQString(columns[i].Name));
            for (int row = 0; row < list.rowCount(); ++row) {
                QCOMPARE(list.item(row, column)->textAlignment(),
                         GetColumnAlign(columns[i].ID, columns[i].Type));
                QCOMPARE(list.item(row, column)->text(),
                         list.sourcePanel->GetItemText(
                             list.item(row, 0)->data(Qt::UserRole + 6).toUInt(), columns[i]));
            }
        }
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(list, window));
            QVERIFY(UiInspection::fullyVisible(peer, window));
            QVERIFY(UiInspection::writeReport(window, QString("native-columns-%1-%2-%3")
                                                          .arg(QTest::currentDataTag(), phase)
                                                          .arg(size.width())));
        }
    }
    static void checkDefaults(FileTable &list, bool filesystem) {
        const QSet<PROPID> hidden{kpidATime, kpidChangeTime, kpidAttrib,   kpidPackSize,
                                  kpidINode, kpidLinks,      kpidNtReparse};
        const auto &columns = list.sourcePanel->GetColumns();
        QVERIFY(!columns.IsEmpty());
        FOR_VECTOR(i, columns) {
            QCOMPARE(columns[i].IsVisible, !filesystem || !hidden.contains(columns[i].ID));
            QCOMPARE(columns[i].Width, UInt32(columns[i].ID == kpidName ? 160 : 100));
            QVERIFY(columns[i].ID != kpidIsDir);
        }
        QCOMPARE(list.sourcePanel->_sortID, PROPID(kpidName));
        QCOMPARE(list.sourcePanel->GetVisibleColumns()[0].ID, PROPID(kpidName));
    }
  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; real column workflow unverified");
#else
        engine_ = QStandardPaths::findExecutable("7zz");
        QVERIFY(!engine_.isEmpty());
        module_ = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        version_ = backend.codecCatalog()->version();
#endif
    }
    void providerColumnsAndSettings_data() {
        QTest::addColumn<unsigned>("pane");
        QTest::addColumn<bool>("russian");
        QTest::addColumn<QString>("format");
        for (unsigned pane : {0u, 1u})
            for (bool russian : {false, true})
                for (const QString format : {"zip", "7z"})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(pane).arg(russian).arg(format)))
                        << pane << russian << format;
    }
    void providerColumnsAndSettings() {
        QFETCH(unsigned, pane);
        QFETCH(bool, russian);
        QFETCH(QString, format);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SaveRegLang(UString(russian ? L"ru" : L"-"), settings);
        ReloadLang(settings);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString files = fixture.filePath("files");
        QVERIFY(QDir().mkpath(files));
        const QStringList names{QStringLiteral("first 文件.txt"), QStringLiteral("second δ.txt")};
        for (const QString &name : names) {
            QFile file(QDir(files).filePath(name));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write("untouched"), qint64(9));
        }
        const QString archive = fixture.filePath("archive with spaces." + format);
        QProcess creator;
        creator.setWorkingDirectory(files);
        creator.start(engine_, QStringList{"a", "-t" + format, "--", archive} + names);
        QVERIFY(creator.waitForFinished(10000));
        QCOMPARE(creator.exitCode(), 0);
        const QByteArray archiveBytes = read(archive);
        QVERIFY(!archiveBytes.isEmpty());
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        navigate(window, 1 - pane, files);
        navigate(window, pane, files);
        auto &list = view(window, pane);
        auto &panel = *list.sourcePanel;
        auto &peer = view(window, 1 - pane);
        peer.sourcePanel->Set_AutoRefresh_Mode(false);
        CMyComPtr<IFolderFolder> peerProvider = peer.sourcePanel->GetFolder();
        const int peerRows = peer.rowCount();
        QVERIFY(panel.IsFSFolder());
        checkDefaults(list, true);
        const int time = panel.GetColumns().FindItem_for_PropID(kpidATime);
        QVERIFY(time >= 0);
        QTimer::singleShot(0, [&] {
            auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
            QVERIFY(menu);
            menu->actions()[time]->trigger();
            menu->close();
        });
        QVERIFY(QMetaObject::invokeMethod(list.horizontalHeader(), "customContextMenuRequested",
                                          Qt::DirectConnection, Q_ARG(QPoint, QPoint(10, 5))));
        list.horizontalHeader()->resizeSection(0, 187);
        list.horizontalHeader()->moveSection(list.horizontalHeader()->visualIndex(0),
                                             list.columnCount() - 1);
        inspect(window, list, peer, "filesystem");
        focusPath(window, pane);
        window.openArchive(archive);
        QTRY_VERIFY(panel.IsArcFolder());
        QTRY_COMPARE(list.rowCount(), 2);
        QVERIFY(dynamic_cast<CAgentFolder *>(panel.GetFolder()));
        panel.Set_AutoRefresh_Mode(false);
        checkDefaults(list, false);
        QVERIFY(panel.GetColumns().FindItem_for_PropID(kpidPackSize) >= 0);
        list.horizontalHeader()->resizeSection(0, 213);
        inspect(window, list, peer, "archive");
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        focusPath(window, pane);
        action(window, "openRootAction")->trigger();
        auto *spec = new SchemaFolder;
        CMyComPtr<IFolderFolder> provider = spec;
        spec->rawEnabled = false;
        spec->properties = {
            {kpidName, VT_BSTR}, {kpidCRC, VT_UI4}, {10001, VT_EMPTY, S_OK, UString(L"column α")}};
        panel.SetNewFolder(provider);
        panel.LoadFullPath();
        panel.OnReload();
        QCOMPARE(panel._sortID, PROPID(0));
        const int crc = list.columnForCatalogProperty(1);
        QVERIFY(crc >= 0);
        list.horizontalHeader()->resizeSection(crc, 173);
        panel.SelectAll(true);
        focusPath(window, 1 - pane);
        QWidget *const focused = window.focusWidget();
        const CPropColumns unchanged = panel.GetVisibleColumns();
        CSelectedState beforeInitialization;
        panel.SaveSelectedState(beforeInitialization);
        const QPersistentModelIndex retained = list.model()->index(0, crc);
        QCOMPARE(panel.InitColumns(), S_OK);
        QVERIFY(panel.GetVisibleColumns().IsEqualTo(unchanged));
        QVERIFY(retained.isValid());
        QVERIFY(!panel.IsItemMarked(0));
        QCOMPARE(list.selectionModel()->selectedRows().size(), 2);
        QCOMPARE(panel.RefreshListCtrl(beforeInitialization), S_OK);
        panel.OnReload();
        inspect(window, list, peer, "schema-original");
        spec->properties[1].type = VT_BSTR;
        spec->crcText = L"typed 文件";
        panel.OnReload();
        QCOMPARE(panel.GetFolder(), (IFolderFolder *)provider);
        QVERIFY(!panel.GetVisibleColumns().IsEqualTo(unchanged));
        QCOMPARE(panel.GetVisibleColumns()[1].Width, UInt32(173));
        QCOMPARE(panel.GetVisibleColumns()[1].Type, VARTYPE(VT_BSTR));
        QCOMPARE(list.item(0, list.columnForCatalogProperty(1))->text(),
                 QStringLiteral("typed 文件"));
        QCOMPARE(list.item(0, list.columnForCatalogProperty(1))->textAlignment(),
                 Qt::AlignLeft | Qt::AlignVCenter);
        QCOMPARE(list.horizontalHeaderItem(list.columnForCatalogProperty(2))->text(),
                 QStringLiteral("column α"));
        inspect(window, list, peer, "schema-type");
        const CPropColumns beforeName = panel.GetVisibleColumns();
        spec->properties[2].name = L"column 文件";
        panel.OnReload();
        QVERIFY(!panel.GetVisibleColumns().IsEqualTo(beforeName));
        QCOMPARE(panel.GetVisibleColumns()[1].Width, UInt32(173));
        QCOMPARE(ToQString(panel.GetVisibleColumns()[2].Name), QStringLiteral("column 文件"));
        QCOMPARE(list.horizontalHeaderItem(list.columnForCatalogProperty(2))->text(),
                 QStringLiteral("column 文件"));
        QCOMPARE(list.selectionModel()->selectedRows().size(), 2);
        QCOMPARE(window.focusWidget(), focused);
        inspect(window, list, peer, "schema-name");
        navigate(window, pane, files);
        QVERIFY(panel.IsFSFolder());
        QCOMPARE(list.columnWidth(0), 187);
        const int restoredTime = panel.GetColumns().FindItem_for_PropID(kpidATime);
        QVERIFY(restoredTime >= 0 && panel.GetColumns()[restoredTime].IsVisible);
        QCOMPARE(panel.GetVisibleColumns()[0].Order, int(panel.GetVisibleColumns().Size() - 1));
        inspect(window, list, peer, "filesystem-restored");
        focusPath(window, pane);
        window.openArchive(archive);
        QTRY_VERIFY(panel.IsArcFolder());
        QTRY_COMPARE(list.rowCount(), 2);
        QCOMPARE(list.columnWidth(0), 213);
        inspect(window, list, peer, "archive-restored");
        QCOMPARE(peer.sourcePanel->GetFolder(), (IFolderFolder *)peerProvider);
        QCOMPARE(peer.rowCount(), peerRows);
        QCOMPARE(read(archive), archiveBytes);
        for (const QString &name : names)
            QCOMPARE(read(QDir(files).filePath(name)), QByteArray("untouched"));
    }
};
QTEST_MAIN(NativeColumnWorkflowTests)
#include "native_column_workflow_tests.moc"
