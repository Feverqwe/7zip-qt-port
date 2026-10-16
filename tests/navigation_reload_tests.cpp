#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "layout_inspection.h"
#include "panel_load_folder.h"
#include "window_state_support.h"

class NavigationReloadTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
  private slots:
    void parentFailureReleasesPresentation_data() {
        QTest::addColumn<unsigned>("sourceIndex");
        QTest::addColumn<QString>("language");
        QTest::addColumn<QString>("phase");
        for (unsigned index : {0u, 1u})
            for (const QString language : {"-", "ru"})
                for (const QString phase : {"load", "cancel", "type", "count", "icon"})
                    QTest::newRow(
                        qPrintable(QString("%1-%2-%3").arg(index).arg(language).arg(phase)))
                        << index << language << phase;
    }
    void parentFailureReleasesPresentation() {
        QFETCH(unsigned, sourceIndex);
        QFETCH(QString, language);
        QFETCH(QString, phase);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SaveRegLang(ToUString(language), settings);
        ReloadLang(settings);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString control = fixture.filePath(QStringLiteral("control 文件.txt"));
        QFile file(control);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("untouched"), qint64(9));
        file.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *peerUi = window.findChild<QWidget *>(QString("filePanel%1").arg(1 - sourceIndex));
        auto *peerView =
            static_cast<FileTable *>(peerUi->findChild<QTableWidget *>("archiveTable"));
        auto *peerAddress = peerUi->findChild<QLineEdit *>("archivePath");
        peerAddress->setFocus();
        peerAddress->setText(fixture.path());
        QTest::keyClick(peerAddress, Qt::Key_Return);
        QTRY_COMPARE(peerView->rowCount(), 1);
        CMyComPtr<IFolderFolder> peerFolder = peerView->sourcePanel->GetFolder();
        peerView->sourcePanel->Set_AutoRefresh_Mode(false);
        auto *ui = window.findChild<QWidget *>(QString("filePanel%1").arg(sourceIndex));
        auto *view = static_cast<FileTable *>(ui->findChild<QTableWidget *>("archiveTable"));
        auto *address = ui->findChild<QLineEdit *>("archivePath");
        address->setFocus();
        action(window, "openRootAction")->trigger();
        auto *parentSpec = new LoadFolder;
        CMyComPtr<IFolderFolder> parent = parentSpec;
        auto *childSpec = new LoadFolder;
        CMyComPtr<IFolderFolder> child = childSpec;
        childSpec->folderType = L"NetFolder";
        childSpec->folderPath = L"Network fixture/дочерний/";
        childSpec->parentFolder = parent;
        auto &panel = *view->sourcePanel;
        panel.Set_AutoRefresh_Mode(false);
        panel.SetNewFolder(child);
        panel.LoadFullPath();
        panel.OnReload();
        QCOMPARE(view->rowCount(), 2);
        QVERIFY(!address->text().isEmpty());
        view->selectRow(0);
        view->setFocus();
        QVERIFY(action(window, "parentFolderAction")->isEnabled());
        const HRESULT expected = phase == "count"    ? S_OK
                                 : phase == "cancel" ? E_ABORT
                                 : phase == "type"   ? E_FAIL
                                                     : E_INVALIDARG;
        parentSpec->loadResult = phase == "load" || phase == "cancel" ? expected : S_OK;
        parentSpec->directoryType = phase == "type" ? VT_UI4 : VT_BOOL;
        parentSpec->loadedCountResult = phase == "count" ? E_INVALIDARG : S_OK;
        parentSpec->attributeResult = phase == "icon" ? expected : S_OK;
        action(window, "parentFolderAction")->trigger();
        QCOMPARE(panel.GetFolder(), (IFolderFolder *)parent);
        QCOMPARE(view->rowCount(), phase == "count" ? 2 : 0);
        QVERIFY2(panel.ShouldProcessNotifications() && view->isEnabled() &&
                     action(window, "refreshAction")->isEnabled() && address->text().isEmpty(),
                 qPrintable(QString("notify=%1 view=%2 refresh=%3 address=%4")
                                .arg(panel.ShouldProcessNotifications())
                                .arg(view->isEnabled())
                                .arg(action(window, "refreshAction")->isEnabled())
                                .arg(address->text())));
        QVERIFY(view->updatesEnabled());
        QVERIFY(view->isSortingEnabled());
        QVERIFY(peerView->isEnabled());
        QVERIFY(peerView->updatesEnabled());
        QVERIFY(peerView->sourcePanel->ShouldProcessNotifications());
        QCOMPARE(peerView->sourcePanel->GetFolder(), (IFolderFolder *)peerFolder);
        QCOMPARE(peerAddress->text(), QFileInfo(fixture.path()).canonicalFilePath());
        QCOMPARE(peerView->rowCount(), 1);
        QCOMPARE(panel.RefreshListCtrl_SaveFocused(), expected);
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*view, window));
            QVERIFY(UiInspection::fullyVisible(*peerView, window));
            QVERIFY(UiInspection::fullyVisible(*address, window));
            QVERIFY(UiInspection::fullyVisible(*peerAddress, window));
            QVERIFY(UiInspection::fullyVisible(*window.findChild<QStatusBar *>("panelStatusBar"),
                                               window));
            QVERIFY(UiInspection::fullyVisible(*window.findChild<QToolBar *>(), window));
            for (const auto *label : window.findChildren<QLabel *>())
                if (label->isVisible() && !label->text().isEmpty()) {
                    QVERIFY(UiInspection::fullyVisible(*label, window));
                    QVERIFY(UiInspection::labelFits(*label));
                }
            QVERIFY(UiInspection::writeReport(window, QString("navigation-load-error-%1-%2")
                                                          .arg(QTest::currentDataTag())
                                                          .arg(size.width())));
        }
        parentSpec->loadResult = S_OK;
        parentSpec->directoryType = VT_BOOL;
        parentSpec->loadedCountResult = S_OK;
        parentSpec->attributeResult = S_OK;
        action(window, "refreshAction")->trigger();
        QCOMPARE(view->rowCount(), 2);
        QCOMPARE(panel.GetFolder(), (IFolderFolder *)parent);
        QVERIFY(panel.ShouldProcessNotifications());
        QVERIFY(view->isEnabled());
        QVERIFY(view->updatesEnabled());
        QVERIFY(view->isSortingEnabled());
        QCOMPARE(peerView->sourcePanel->GetFolder(), (IFolderFolder *)peerFolder);
        QCOMPARE(peerView->rowCount(), 1);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("untouched"));
        QCOMPARE(backend.listCalls, 0);
    }
};
QTEST_MAIN(NavigationReloadTests)
#include "navigation_reload_tests.moc"
