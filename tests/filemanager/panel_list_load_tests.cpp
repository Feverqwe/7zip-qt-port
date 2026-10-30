#include "Windows/PropVariant.h"
#include "filemanager/panels/Panel.h"
#include "filemanager/providers/FSFolder.h"
#include "filemanager/support/panel_load_folder.h"
#include "filemanager/support/window_state_support.h"
#include "filemanager/views/filetable.h"
#include "localization/LangUtils.h"
#include "settings/RegistryUtils.h"
#include "ui/progress/ProgressDialog2.h"
#include "ui/support/layout_inspection.h"
#include <new>

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
    void init() {
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        SaveRegLang(UString("-"), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
    }
    void filesystemReloadPreservesProviderResult_data() {
        QTest::addColumn<unsigned>("sourceIndex");
        QTest::addColumn<QString>("caller");
        QTest::addColumn<QString>("language");
        QTest::addColumn<bool>("cancel");
        for (unsigned index : {0u, 1u})
            for (const char *caller : {"result", "reload", "timer", "command"})
                for (const char *language : {"-", "ru"})
                    for (bool cancel : {false, true})
                        QTest::newRow(qPrintable(QString("%1-%2-%3-%4")
                                                     .arg(index)
                                                     .arg(caller)
                                                     .arg(language)
                                                     .arg(cancel)))
                            << index << QString::fromLatin1(caller) << QString::fromLatin1(language)
                            << cancel;
    }
    void filesystemReloadPreservesProviderResult() {
        QFETCH(unsigned, sourceIndex);
        QFETCH(QString, caller);
        QFETCH(QString, language);
        QFETCH(bool, cancel);
        const HRESULT expected = cancel ? E_ABORT : E_OUTOFMEMORY;
        SaveRegLang(ToUString(language), qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        ReloadLang(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString payload = fixture.filePath(QStringLiteral("source 文件 δ.txt"));
        QFile file(payload);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("control"), qint64(7));
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *ui = window.findChild<QWidget *>(QString("filePanel%1").arg(sourceIndex));
        auto *address = ui->findChild<QLineEdit *>("archivePath");
        address->setFocus();
        address->setText(fixture.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto *view = static_cast<FileTable *>(ui->findChild<FileTable *>("archiveTable"));
        QTRY_COMPARE(view->rowCount(), 1);
        auto &panel = *view->sourcePanel;
        CMyComPtr<IFolderFolder> folder = panel.GetFolder();
        auto *spec = dynamic_cast<NFsFolder::CFSFolder *>((IFolderFolder *)folder);
        QVERIFY(spec);
        view->selectRow(0);
        spec->CheckBreak = [cancel](const QString &) {
            if (!cancel)
                throw std::bad_alloc();
            return false;
        };
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
                        panel.MarkPortableFolderChanged();
                        panel.OnTimer();
                    } else
                        action(window, "refreshAction")->trigger();
                },
                QString("filesystem-reload-hresult-%1").arg(QTest::currentDataTag()));
            QCOMPARE(shown, ToQString(HResultToMessage(expected)));
        }
        QCOMPARE(panel.GetFolder(), (IFolderFolder *)folder);
        QCOMPARE(view->rowCount(), 0);
        QVERIFY(panel.ShouldProcessNotifications());
        CSelectedState selection;
        QCOMPARE(panel.RefreshListCtrl(selection), expected);
        spec->CheckBreak = {};
        panel.OnReload();
        QCOMPARE(panel.GetFolder(), (IFolderFolder *)folder);
        QCOMPARE(view->rowCount(), 1);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("control"));
        QVERIFY(action(window, "refreshAction")->isEnabled());
    }
    void loadedDirectoriesAreCachedAndReplaced() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        auto *spec = new LoadFolder;
        CMyComPtr<IFolderFolder> folder = spec;
        CPanel panel(folder);
        panel.AttachFolderManager(nullptr, fixture.filePath("settings.ini"));
        QCOMPARE(panel.LoadListItems(), S_OK);
        spec->directory = false;
        QVERIFY(panel.IsItem_Folder(0));
        QVERIFY(panel.IsItem_Folder(1));
        spec->calls.clear();
        QCOMPARE(panel.LoadListItems(true), S_OK);
        QVERIFY(!panel.IsItem_Folder(0));
        QVERIFY(!panel.IsItem_Folder(1));
        spec->directoryType = VT_EMPTY;
        QCOMPARE(panel.LoadListItems(), S_OK);
        QVERIFY(!panel.IsItem_Folder(0));
    }
    void failedWorkerResultStopsBeforeProperties() {
        auto *spec = new LoadFolder;
        CMyComPtr<IFolderFolder> folder = spec;
        CPanel panel(folder);
        QCOMPARE(panel.LoadListItems(), S_OK);
        spec->calls.clear();
        QCOMPARE(panel.LoadListItems(true, E_ABORT), E_ABORT);
        QVERIFY(spec->calls.isEmpty());
        CSelectedState state;
        QCOMPARE(panel.RefreshListCtrl(state), E_ABORT);
        QCOMPARE(panel.GetFolder(), (IFolderFolder *)folder);
        QCOMPARE(panel.LoadListItems(true), S_OK);
        QVERIFY(panel.IsItem_Folder(0));
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
        QCOMPARE(view->model()->index(0, 0).data().toString(), QStringLiteral("a 文件"));
        for (int row = 0; row < view->rowCount(); ++row) {
            const auto sourceIndex = view->model()->index(row, 0).data(Qt::UserRole + 6).toUInt();
            QCOMPARE(view->model()->index(row, 0).data().toString(),
                     ToQString(view->sourcePanel->GetItemName(sourceIndex)));
            for (int column = 1; column < view->columnCount(); ++column) {
                QVERIFY(view->model()->index(row, column).isValid());
                QCOMPARE(view->model()->index(row, column).data(Qt::UserRole + 6).toUInt(),
                         sourceIndex);
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
        QCOMPARE(view->model()->index(0, 0).data().toString(), QStringLiteral("first δ"));
        QVERIFY(view->model()->index(0, 0).data(Qt::UserRole + 1).toBool());
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
        QVERIFY(!view->model()->index(0, 0).data(Qt::UserRole + 1).toBool());
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
        const HRESULT expected = phase == "count"    ? S_OK
                                 : phase == "cancel" ? E_ABORT
                                 : phase == "type"   ? E_FAIL
                                                     : E_INVALIDARG;
        if (phase == "load" || phase == "cancel")
            spec->loadResult = expected;
        else if (phase == "property")
            spec->directoryResult = expected;
        else if (phase == "type")
            spec->directoryType = VT_UI4;
        else
            spec->loadedCountResult = E_INVALIDARG;
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
            QCOMPARE(shown, phase == "count" ? QString() : ToQString(HResultToMessage(expected)));
        }
        QCOMPARE(view->rowCount(), phase == "count" ? 2 : 0);
        QVERIFY(panel.ShouldProcessNotifications());
        CSelectedState failedSelection;
        QCOMPARE(panel.RefreshListCtrl(failedSelection), expected);
        const QString repeated = closeError(window, [&] { panel.OnReload(); });
        QCOMPARE(repeated, phase == "count" ? QString() : ToQString(HResultToMessage(expected)));
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
