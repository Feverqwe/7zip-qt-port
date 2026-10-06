#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/Panel.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "Windows/PropVariant.h"
#include "layout_inspection.h"
#include "window_state_support.h"

class LoadFolder final : public IFolderFolder,
                         public IFolderSetFlatMode,
                         public IFolderWasChanged,
                         public CMyUnknownImp {
    Z7_COM_QI_BEGIN2(IFolderFolder)
    Z7_COM_QI_ENTRY(IFolderSetFlatMode)
    Z7_COM_QI_ENTRY(IFolderWasChanged)
    Z7_COM_QI_END
    Z7_COM_ADDREF_RELEASE
  public:
    QStringList calls;
    HRESULT loadResult = S_OK;
    HRESULT directoryResult = S_OK;
    HRESULT countResult = S_OK;
    HRESULT loadedCountResult = S_OK;
    VARTYPE directoryType = VT_BOOL;
    bool directory = true;
    bool reverseNames = false;
    bool changed = false;
    Z7_IFACE_COM7_IMP(IFolderFolder)
    Z7_IFACE_COM7_IMP(IFolderSetFlatMode)
    Z7_IFACE_COM7_IMP(IFolderWasChanged)
};

Z7_COM7F_IMF(LoadFolder::SetFlatMode(Int32)) {
    calls << "flat";
    return S_OK;
}
Z7_COM7F_IMF(LoadFolder::LoadItems()) {
    calls << "load";
    countResult = loadedCountResult;
    return loadResult;
}
Z7_COM7F_IMF(LoadFolder::GetNumberOfItems(UInt32 *count)) {
    *count = 2;
    return countResult;
}
Z7_COM7F_IMF(LoadFolder::WasChanged(Int32 *wasChanged)) {
    *wasChanged = BoolToInt(changed);
    changed = false;
    return S_OK;
}
Z7_COM7F_IMF(LoadFolder::GetNumberOfProperties(UInt32 *count)) {
    calls << "columns";
    *count = 1;
    return S_OK;
}
Z7_COM7F_IMF(LoadFolder::GetPropertyInfo(UInt32, BSTR *name, PROPID *id, VARTYPE *type)) {
    *name = nullptr;
    *id = kpidName;
    *type = VT_BSTR;
    return S_OK;
}
Z7_COM7F_IMF(LoadFolder::GetProperty(UInt32 index, PROPID id, PROPVARIANT *value)) {
    NWindows::NCOM::CPropVariant prop;
    if (id == kpidName)
        prop =
            reverseNames ? (index ? L"a 文件" : L"z δ") : (index ? L"second 文件.txt" : L"first δ");
    else if (id == kpidIsDir) {
        calls << "directory";
        if (directoryResult != S_OK)
            return directoryResult;
        if (directoryType == VT_BOOL)
            prop = directory;
        else if (directoryType != VT_EMPTY)
            prop = UInt32(1);
    }
    return prop.Detach(value);
}
Z7_COM7F_IMF(LoadFolder::GetFolderProperty(PROPID id, PROPVARIANT *value)) {
    NWindows::NCOM::CPropVariant prop;
    if (id == kpidType)
        prop = L"RootFolder";
    return prop.Detach(value);
}
Z7_COM7F_IMF(LoadFolder::BindToFolder(UInt32, IFolderFolder **folder)) {
    *folder = nullptr;
    return E_NOTIMPL;
}
Z7_COM7F_IMF(LoadFolder::BindToFolder(const wchar_t *, IFolderFolder **folder)) {
    *folder = nullptr;
    return E_NOTIMPL;
}
Z7_COM7F_IMF(LoadFolder::BindToParentFolder(IFolderFolder **folder)) {
    *folder = nullptr;
    return S_OK;
}

class PanelListLoadTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static QString closeError(MainWindow &window, const std::function<void()> &reload,
                              const QString &tag = {}) {
        QString shown;
        QTimer driver;
        connect(&driver, &QTimer::timeout, &window, [&] {
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible()) {
                    shown = box->text();
                    if (!tag.isEmpty()) {
                        for (const QSize size : {QSize(440, 250), QSize(360, 200)}) {
                            box->resize(size);
                            box->layout()->activate();
                            QTest::qVerify(UiInspection::writeReport(
                                               *box, tag + '-' + QString::number(size.width())),
                                           "report", "Reload error geometry", __FILE__, __LINE__);
                        }
                    }
                    box->accept();
                }
        });
        driver.start(0);
        reload();
        return shown;
    }
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void loadedDirectoriesAreCachedAndReplaced() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        auto *spec = new LoadFolder;
        CMyComPtr<IFolderFolder> folder = spec;
        CPanel panel(folder);
        panel.AttachFolderManager(nullptr, fixture.filePath("settings.ini"));
        QCOMPARE(panel.LoadListItems(), S_OK);
        QCOMPARE(spec->calls, (QStringList{"flat", "load", "columns", "directory", "directory"}));
        spec->directory = false;
        QVERIFY(panel.IsItem_Folder(0));
        QVERIFY(panel.IsItem_Folder(1));
        QCOMPARE(spec->calls.count("directory"), 2);
        spec->calls.clear();
        QCOMPARE(panel.LoadListItems(true), S_OK);
        QCOMPARE(spec->calls, (QStringList{"columns", "directory", "directory"}));
        QVERIFY(!panel.IsItem_Folder(0));
        QVERIFY(!panel.IsItem_Folder(1));
        spec->directoryType = VT_EMPTY;
        QCOMPARE(panel.LoadListItems(), S_OK);
        QVERIFY(!panel.IsItem_Folder(0));
    }
    void namespacePublicationRejectsInvalidDirectories_data() {
        QTest::addColumn<qint32>("result");
        QTest::addColumn<int>("type");
        QTest::addColumn<bool>("loadFailure");
        QTest::newRow("wrong-type") << qint32(S_OK) << int(VT_UI4) << false;
        QTest::newRow("property-failure") << qint32(E_INVALIDARG) << int(VT_BOOL) << false;
        QTest::newRow("load-failure") << qint32(E_INVALIDARG) << int(VT_BOOL) << true;
        QTest::newRow("load-cancel") << qint32(E_ABORT) << int(VT_BOOL) << true;
    }
    void namespaceSortingKeepsColumnSourceIndices() {
        ControlledBackend backend;
        MainWindow window(&backend);
        action(window, "openRootAction")->trigger();
        auto *view = static_cast<FileTable *>(table(window));
        auto *spec = new LoadFolder;
        CMyComPtr<IFolderFolder> folder = spec;
        view->sourcePanel->SetNewFolder(folder);
        action(window, "refreshAction")->trigger();
        QCOMPARE(view->rowCount(), 2);
        action(window, "sort0Action")->trigger();
        spec->reverseNames = true;
        action(window, "refreshAction")->trigger();
        QCOMPARE(view->rowCount(), 2);
        QCOMPARE(view->item(0, 0)->text(), QStringLiteral("a 文件"));
        for (int row = 0; row < view->rowCount(); ++row) {
            const auto sourceIndex = view->item(row, 0)->data(Qt::UserRole + 6).toUInt();
            QCOMPARE(view->item(row, 0)->text(),
                     ToQString(view->sourcePanel->GetItemName(sourceIndex)));
            for (int column = 1; column < view->columnCount(); ++column) {
                QVERIFY(view->item(row, column));
                QCOMPARE(view->item(row, column)->data(Qt::UserRole + 6).toUInt(), sourceIndex);
            }
        }
    }
    void namespacePublicationRejectsInvalidDirectories() {
        QFETCH(qint32, result);
        QFETCH(int, type);
        QFETCH(bool, loadFailure);
        ControlledBackend backend;
        MainWindow window(&backend);
        action(window, "openRootAction")->trigger();
        auto *view = static_cast<FileTable *>(table(window));
        auto *spec = new LoadFolder;
        CMyComPtr<IFolderFolder> folder = spec;
        view->sourcePanel->SetNewFolder(folder);
        action(window, "refreshAction")->trigger();
        QCOMPARE(view->rowCount(), 2);
        QCOMPARE(view->item(0, 0)->text(), QStringLiteral("first δ"));
        QVERIFY(view->item(0, 0)->data(Qt::UserRole + 1).toBool());
        spec->directoryResult = loadFailure ? S_OK : HRESULT(result);
        spec->directoryType = VARTYPE(type);
        spec->loadResult = loadFailure ? HRESULT(result) : S_OK;
        const QString shown =
            closeError(window, [&] { action(window, "refreshAction")->trigger(); });
        QCOMPARE(view->rowCount(), 0);
        QCOMPARE(shown, ToQString(HResultToMessage(result == S_OK ? E_FAIL : HRESULT(result))));
        QVERIFY(view->sourcePanel->ShouldProcessNotifications());
        QVERIFY(window.statusBar()->currentMessage().contains("Could not read root folder"));
        window.show();
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*view, window));
            QVERIFY(UiInspection::writeReport(
                window,
                QString("panel-load-error-%1-%2").arg(QTest::currentDataTag()).arg(size.width())));
        }
        QCOMPARE(view->sourcePanel->LoadListItems(), result == S_OK ? E_FAIL : HRESULT(result));
        spec->directoryResult = S_OK;
        spec->directoryType = VT_EMPTY;
        spec->loadResult = S_OK;
        action(window, "refreshAction")->trigger();
        QCOMPARE(view->rowCount(), 2);
        QVERIFY(!view->item(0, 0)->data(Qt::UserRole + 1).toBool());
    }
    void namespaceReloadReturnsOriginalHRESULT_data() {
        QTest::addColumn<QString>("phase");
        QTest::addColumn<QString>("caller");
        QTest::addColumn<QString>("language");
        for (const char *phase : {"load", "cancel", "property", "type", "count"})
            for (const char *caller : {"result", "reload", "timer", "command"})
                for (const char *language : {"-", "ru"})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(phase).arg(caller).arg(language)))
                        << QString::fromLatin1(phase) << QString::fromLatin1(caller)
                        << QString::fromLatin1(language);
    }
    void namespaceReloadReturnsOriginalHRESULT() {
        QFETCH(QString, phase);
        QFETCH(QString, caller);
        QFETCH(QString, language);
        SaveRegLang(ToUString(language), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ControlledBackend backend;
        MainWindow window(&backend);
        action(window, "openRootAction")->trigger();
        auto *view = static_cast<FileTable *>(table(window));
        auto *spec = new LoadFolder;
        CMyComPtr<IFolderFolder> folder = spec;
        auto &panel = *view->sourcePanel;
        panel.SetNewFolder(folder);
        panel.OnReload();
        QCOMPARE(view->rowCount(), 2);
        view->selectRow(0);
        const HRESULT expected = phase == "cancel" ? E_ABORT
                                 : phase == "type" ? E_FAIL
                                                   : E_INVALIDARG;
        if (phase == "load" || phase == "cancel")
            spec->loadResult = expected;
        else if (phase == "property")
            spec->directoryResult = expected;
        else if (phase == "type")
            spec->directoryType = VT_UI4;
        else
            spec->loadedCountResult = expected;
        if (caller == "result")
            QCOMPARE(panel.RefreshListCtrl_SaveFocused(), expected);
        else {
            const QString shown = closeError(
                window,
                [&] {
                    if (caller == "reload")
                        panel.OnReload();
                    else if (caller == "timer") {
                        panel.Set_AutoRefresh_Mode(true);
                        spec->changed = true;
                        panel.OnTimer();
                    } else
                        action(window, "refreshAction")->trigger();
                },
                QString("panel-reload-hresult-%1").arg(QTest::currentDataTag()));
            QCOMPARE(shown, ToQString(HResultToMessage(expected)));
        }
        QCOMPARE(view->rowCount(), 0);
        QVERIFY(panel.ShouldProcessNotifications());
        spec->loadResult = spec->directoryResult = spec->countResult = spec->loadedCountResult =
            S_OK;
        spec->directoryType = VT_BOOL;
        panel.OnReload();
        QCOMPARE(view->rowCount(), 2);
        QVERIFY(action(window, "refreshAction")->isEnabled());
    }
};

QTEST_MAIN(PanelListLoadTests)
#include "panel_list_load_tests.moc"
