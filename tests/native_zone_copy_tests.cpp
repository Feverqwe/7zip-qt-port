#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/Common/ZipRegistry.h"
#include "CPP/7zip/UI/FileManager/ViewSettings.h"
#include "archivecodeccatalog.h"
#include "copydialog.h"
#include "fixtures/signal_wait_support.h"
#include "layout_inspection.h"
#include "qtfilemanagerstartup.h"
#include "source_extract_fixture.h"
#include "window_state_support.h"

class NativeZoneCopyTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    QString engine_;
    std::unique_ptr<ArchiveBackend> catalogOwner_;

  private slots:
    void initTestCase() {
        engine_ = QStandardPaths::findExecutable("7zz");
        QVERIFY(!engine_.isEmpty());
        const QString module = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveCodecCatalog catalog(module, 0);
        QCOMPARE(catalog.result(), S_OK);
        catalogOwner_ = std::make_unique<ArchiveBackend>();
        QCOMPARE(catalogOwner_->loadCodecCatalog(module, catalog.version()), S_OK);
    }
    void copyPreservesStreamsAndAppliesContainerZone_data() {
        QTest::addColumn<int>("mode");
        QTest::addColumn<unsigned>("pane");
        QTest::addColumn<bool>("largePeer");
        for (int mode : {int(NExtract::NZoneIdMode::kNone), int(NExtract::NZoneIdMode::kAll),
                         int(NExtract::NZoneIdMode::kOffice)})
            for (unsigned pane : {0u, 1u})
                QTest::newRow(qPrintable(QString("%1-%2").arg(mode).arg(pane)))
                    << mode << pane << false;
        for (unsigned pane : {0u, 1u})
            QTest::newRow(qPrintable(QString("all-large-peer-%1").arg(pane)))
                << int(NExtract::NZoneIdMode::kAll) << pane << true;
    }
    void copyPreservesStreamsAndAppliesContainerZone() {
        QFETCH(int, mode);
        QFETCH(unsigned, pane);
        QFETCH(bool, largePeer);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QVERIFY(!settings.isEmpty());
        QFile::remove(settings);
        CContextMenuInfo menu;
        menu.Load(settings);
        menu.WriteZone = UInt32(mode);
        menu.Save(settings);
        QSettings presentation(settings, QSettings::IniFormat);
        presentation.setValue("FM/ShowDots", false);
        presentation.sync();
        const QString output = fixture.filePath("output δ with space");
        QVERIFY(QDir().mkpath(output));
        QVERIFY(SourceExtractFixture::writeFile(QDir(output).filePath("peer control.txt"),
                                                "peer bytes"));
        if (largePeer)
            for (int i = 0; i < 300; ++i)
                QVERIFY(SourceExtractFixture::writeFile(
                    QDir(output).filePath(QString("filler-%1.txt").arg(i)), "peer filler"));
        SavePanelPath(1 - pane, ToUString(output), settings);
        CWindowInfo windowInfo{};
        windowInfo.rect.right = 980;
        windowInfo.rect.bottom = 620;
        windowInfo.numPanels = 2;
        windowInfo.currentPanel = pane;
        windowInfo.splitterPos = 490;
        windowInfo.Save(settings);
        const QStringList names{QStringLiteral("документ δ.docx"),
                                QStringLiteral("обычный файл δ.bin")};
        const QByteArray storedZone("[ZoneTransfer]\r\nZoneId=1\r\n");
        const QByteArray containerZone("[ZoneTransfer]\r\nZoneId=3\r\n");
        for (const auto &name : names) {
            const QString path = fixture.filePath(name);
            QVERIFY(SourceExtractFixture::writeFile(path, name.toUtf8()));
            QVERIFY(SourceExtractFixture::writeFile(path + ":поток δ", "stream bytes"));
            QVERIFY(SourceExtractFixture::writeFile(path + ":Zone.Identifier", storedZone));
        }
        const QString archive = fixture.filePath("native streams.wim");
        QProcess create;
        create.setWorkingDirectory(fixture.path());
        create.start(engine_, QStringList{"a", "-twim", "-sns", "--", archive} + names);
        QVERIFY(create.waitForFinished(10000));
        QCOMPARE(create.exitCode(), 0);
        QVERIFY(SourceExtractFixture::writeFile(archive + ":Zone.Identifier", containerZone));
        const QByteArray archiveBytes = SourceExtractFixture::readFile(archive);
        std::unique_ptr<ArchiveBackend> backend(catalogOwner_->createIndependentBackend());
        QtFileManagerStartup startup;
        startup.FullPath = ToUString(archive);
        startup.NeedOpenArc = true;
        MainWindow window(backend.get(), {}, nullptr, &startup);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        auto *sourceUi = window.findChild<QWidget *>(QString("filePanel%1").arg(pane));
        auto *peerUi = window.findChild<QWidget *>(QString("filePanel%1").arg(1 - pane));
        QVERIFY(sourceUi && peerUi);
        auto *source =
            static_cast<FileTable *>(sourceUi->findChild<QTableWidget *>("archiveTable"));
        auto *peer = static_cast<FileTable *>(peerUi->findChild<QTableWidget *>("archiveTable"));
        QVERIFY(source && peer);
        QVERIFY2(SignalWaitSupport::waitForCount(*source->model(), 2),
                 qPrintable(QString("native WIM root rows=%1").arg(source->rowCount())));
        QVERIFY(SignalWaitSupport::waitForCount(*peer->model(), largePeer ? 301 : 1));
        source->setCurrentCell(0, 0);
        source->setFocus();
        auto &panel = *source->sourcePanel;
        auto &peerPanel = *peer->sourcePanel;
        panel.Set_AutoRefresh_Mode(false);
        peerPanel.Set_AutoRefresh_Mode(false);
        CMyComPtr<IFolderFolder> provider = panel.GetFolder();
        CMyComPtr<IFolderFolder> peerProvider = peerPanel.GetFolder();
        QVERIFY(dynamic_cast<CAgentFolder *>(provider.Interface()));
        panel.SelectAll(true);
        for (int i = 0; i < peer->rowCount(); ++i)
            if (peer->item(i, 0)->text() == "peer control.txt")
                peer->selectRow(i);
        CSelectedState peerSelection;
        peerPanel.SaveSelectedState(peerSelection);
        QCOMPARE(peerSelection.SelectedNames.Size(), 1u);
        bool submitted = false;
        QStringList errors;
        QTimer driver;
        connect(&driver, &QTimer::timeout, &window, [&] {
            auto *modal = QApplication::activeModalWidget();
            if (auto *dialog = dynamic_cast<CopyDialog *>(modal); dialog && !submitted) {
                submitted = true;
                dialog->accept();
            } else if (auto *box = qobject_cast<QMessageBox *>(modal)) {
                errors << box->text();
                box->accept();
            }
        });
        driver.start(1);
        QTest::keyClick(source, Qt::Key_F5);
        QVERIFY(submitted);
        QVERIFY(SignalWaitSupport::waitForEnabled(*action(window, "refreshAction")));
        driver.stop();
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join('\n')));
        QCOMPARE(panel.GetFolder(), provider.Interface());
        QCOMPARE(source->rowCount(), 2);
        QVERIFY(SignalWaitSupport::waitForCount(*peer->model(), largePeer ? 303 : 3));
        CSelectedState restored;
        peerPanel.SaveSelectedState(restored);
        QCOMPARE(restored.SelectedNames.Size(), peerSelection.SelectedNames.Size());
        QCOMPARE(ToQString(restored.SelectedNames[0]), ToQString(peerSelection.SelectedNames[0]));
        QVERIFY(sourceUi->isAncestorOf(window.focusWidget()));
        QVERIFY(panel.ShouldProcessNotifications() && peerPanel.ShouldProcessNotifications());
        QCOMPARE(SourceExtractFixture::readFile(archive), archiveBytes);
        QCOMPARE(SourceExtractFixture::readFile(archive + ":Zone.Identifier"), containerZone);
        QCOMPARE(SourceExtractFixture::readFile(QDir(output).filePath("peer control.txt")),
                 QByteArray("peer bytes"));
        for (const auto &name : names) {
            const QString copied = QDir(output).filePath(name);
            QCOMPARE(SourceExtractFixture::readFile(copied), name.toUtf8());
            QCOMPARE(SourceExtractFixture::readFile(copied + ":поток δ"),
                     QByteArray("stream bytes"));
            const bool useContainer =
                mode == NExtract::NZoneIdMode::kAll ||
                (mode == NExtract::NZoneIdMode::kOffice && name.endsWith(".docx"));
            QCOMPARE(SourceExtractFixture::readFile(copied + ":Zone.Identifier"),
                     useContainer ? containerZone : storedZone);
            QCOMPARE(SourceExtractFixture::readFile(fixture.filePath(name)), name.toUtf8());
        }
        QCOMPARE(peerPanel.GetFolder(), peerProvider.Interface());
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*source, window));
            QVERIFY(UiInspection::fullyVisible(*peer, window));
            QVERIFY(UiInspection::writeReport(
                window,
                QString("native-zone-copy-%1-%2").arg(QTest::currentDataTag()).arg(size.width())));
        }
    }
};
QTEST_MAIN(NativeZoneCopyTests)
#include "native_zone_copy_tests.moc"
