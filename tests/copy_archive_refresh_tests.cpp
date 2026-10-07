#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "archivecodeccatalog.h"
#include "copydialog.h"
#include "fixtures/signal_wait_support.h"
#include "layout_inspection.h"
#include "qtfilemanagerstartup.h"
#include "source_extract_fixture.h"
#include "window_state_support.h"

class CopyArchiveBackend final : public ArchiveBackend {
  public:
    bool Fail = false, Started = false;
    void updateSourcePlan(const QString &archive, const SourceUpdatePlan &plan,
                          const QString &output, const QString &format, const QString &password,
                          const std::optional<CompressionOptions> &options) override {
        Started = true;
        if (Fail)
            emit finished(false, QStringLiteral("Controlled Copy update failure."));
        else
            ArchiveBackend::updateSourcePlan(archive, plan, output, format, password, options);
    }
    void updateSourceAdd(const QString &archive, const QStringList &entries, const QString &root,
                         const QString &output, const QString &format,
                         const QString &password) override {
        Started = true;
        if (Fail)
            emit finished(false, QStringLiteral("Controlled Copy update failure."));
        else
            ArchiveBackend::updateSourceAdd(archive, entries, root, output, format, password);
    }
};

class CopyArchiveRefreshTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    QString engine_;
    std::unique_ptr<ArchiveBackend> catalogOwner_;
    QTemporaryDir seeds_;

  private slots:
    void initTestCase() {
        engine_ = QStandardPaths::findExecutable("7zz");
        QVERIFY(!engine_.isEmpty());
        QVERIFY(seeds_.isValid());
        const QStringList initial{QStringLiteral("m marked δ.txt"),
                                  QStringLiteral("z focused 文件.bin")};
        for (const QString &name : initial)
            QVERIFY(SourceExtractFixture::writeFile(seeds_.filePath(name), name.toUtf8()));
        QStringList large = initial;
        for (int i = 2; i < 301; ++i) {
            const QString name = QString("seed-%1.txt").arg(i);
            QVERIFY(SourceExtractFixture::writeFile(seeds_.filePath(name), "seed control"));
            large << name;
        }
        // Copy each immutable engine-produced seed into a private destination;
        // all actual update/move/failure/cancel workflows still run per-case.
        for (const QString &seed : {QString("2.zip"), QString("301.zip"), QString("2.7z")}) {
            QProcess create;
            create.setWorkingDirectory(seeds_.path());
            create.start(engine_, QStringList{"a", "--", seeds_.filePath(seed)} +
                                      (seed.startsWith("301") ? large : initial));
            QVERIFY(create.waitForFinished(10000));
            QCOMPARE(create.exitCode(), 0);
        }
#ifdef SEVENZIP_CODEC_MODULE
        ArchiveCodecCatalog catalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE), 0);
        QCOMPARE(catalog.result(), S_OK);
        catalogOwner_ = std::make_unique<ArchiveBackend>();
        QCOMPARE(catalogOwner_->loadCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE),
                                                 catalog.version()),
                 S_OK);
#endif
    }
    void filesystemCopyPreservesDestination_data() {
        QTest::addColumn<unsigned>("pane");
        QTest::addColumn<bool>("russian");
        QTest::addColumn<bool>("icons");
        QTest::addColumn<bool>("alternative");
        QTest::addColumn<bool>("move");
        QTest::addColumn<QString>("route");
        QStringList routes{"transport", "failure", "cancel"};
#ifdef SEVENZIP_CODEC_MODULE
        routes << "zip" << "7z" << "large-zip";
#endif
        for (unsigned pane : {0u, 1u})
            for (bool russian : {false, true})
                for (bool icons : {false, true})
                    for (bool alternative : {false, true})
                        for (bool move : {false, true})
                            for (const QString &route : routes)
                                QTest::newRow(qPrintable(QString("%1-%2-%3-%4-%5-%6")
                                                             .arg(pane)
                                                             .arg(russian)
                                                             .arg(icons)
                                                             .arg(alternative)
                                                             .arg(move)
                                                             .arg(route)))
                                    << pane << russian << icons << alternative << move << route;
    }
    void filesystemCopyPreservesDestination() {
        QFETCH(unsigned, pane);
        QFETCH(bool, russian);
        QFETCH(bool, icons);
        QFETCH(bool, alternative);
        QFETCH(bool, move);
        QFETCH(QString, route);
        const bool success = route != "failure" && route != "cancel";
        const bool native = route == "zip" || route == "7z" || route == "large-zip";
        const int originalCount = route == "large-zip" ? 301 : 2;
        const QString settingsPath = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settingsPath);
        SaveRegLang(UString(russian ? L"ru" : L"-"), settingsPath);
        ReloadLang(settingsPath);
        QSettings settings(settingsPath, QSettings::IniFormat);
        settings.setValue("FM/AlternativeSelection", alternative);
        settings.setValue("FM/ShowDots", false);
        settings.sync();
        CWindowInfo windowInfo{};
        windowInfo.rect.right = 980;
        windowInfo.rect.bottom = 620;
        windowInfo.numPanels = 2;
        windowInfo.currentPanel = 1 - pane;
        windowInfo.splitterPos = 490;
        windowInfo.Save(settingsPath);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString input = fixture.filePath("input folder");
        QVERIFY(QDir().mkpath(input));
        SavePanelPath(pane, ToUString(input), settingsPath);
        const QString added = QStringLiteral("a copied 文件.txt");
        const QString marked = QStringLiteral("m marked δ.txt");
        const QString focused = QStringLiteral("z focused 文件.bin");
        QVERIFY(SourceExtractFixture::writeFile(QDir(input).filePath(added), "copied payload"));
        const QString format = route == "7z" ? route : "zip";
        const QString archive = fixture.filePath("target archive." + format);
        QVERIFY(
            QFile::copy(seeds_.filePath(QString::number(originalCount) + '.' + format), archive));
        const QByteArray before = SourceExtractFixture::readFile(archive);
        CopyArchiveBackend controlled;
        controlled.Fail = route == "failure";
        std::unique_ptr<ArchiveBackend> nativeBackend;
#ifdef SEVENZIP_CODEC_MODULE
        // Keep per-case process/operation owners, sharing only the immutable
        // module lease through the same public factory as independent windows.
        if (native)
            nativeBackend.reset(catalogOwner_->createIndependentBackend());
#endif
        ArchiveBackend &backend = nativeBackend ? *nativeBackend : controlled;
        QtFileManagerStartup startup;
        startup.FullPath = ToUString(archive);
        startup.ArcFormat = ToUString(format);
        startup.NeedOpenArc = true;
        MainWindow window(&backend, {}, nullptr, &startup);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        auto *sourceUi = window.findChild<QWidget *>(QString("filePanel%1").arg(pane));
        auto *targetUi = window.findChild<QWidget *>(QString("filePanel%1").arg(1 - pane));
        auto *target =
            static_cast<FileTable *>(targetUi->findChild<QTableWidget *>("archiveTable"));
        auto &destination = *target->sourcePanel;
        destination.Set_AutoRefresh_Mode(false);
        QVERIFY(SignalWaitSupport::waitForCount(*target->model(), originalCount));
        QCOMPARE(target->rowCount(), originalCount);
        QVERIFY(SignalWaitSupport::waitForEnabled(*action(window, "refreshAction")));
        QVERIFY(action(window, "refreshAction")->isEnabled());
        CMyComPtr<IFolderFolder> provider = destination.GetFolder();
        QVERIFY(dynamic_cast<CAgentFolder *>(provider.Interface()));
        destination.SelectAll(false);
        destination.SelectSpec(ToUString(marked), true);
        target->setCurrentCell(originalCount - 1, 0, QItemSelectionModel::NoUpdate);
        CSelectedState selection;
        destination.SaveSelectedState(selection);
        QCOMPARE(selection.SelectedNames.Size(), 1u);
        auto *address = sourceUi->findChild<QLineEdit *>("archivePath");
        address->setFocus();
        auto *source =
            static_cast<FileTable *>(sourceUi->findChild<QTableWidget *>("archiveTable"));
        QVERIFY(SignalWaitSupport::waitForCount(*source->model(), 1));
        QCOMPARE(source->rowCount(), 1);
        QVERIFY(SignalWaitSupport::waitForEnabled(*action(window, "refreshAction")));
        QVERIFY(action(window, "refreshAction")->isEnabled());
        auto &sourcePanel = *source->sourcePanel;
        sourcePanel.Set_AutoRefresh_Mode(false);
        sourcePanel.SelectAll(true);
        if (route == "cancel")
            connect(&backend, &ArchiveBackend::engineStarted, &backend, [&] { backend.cancel(); });
        if (icons)
            action(window, "viewMode0Action")->trigger();
        QAbstractItemView *view =
            icons ? static_cast<QAbstractItemView *>(sourceUi->findChild<QListView *>("iconList"))
                  : source;
        view->setFocus();
        QVERIFY(sourceUi->isAncestorOf(window.focusWidget()));
        QSignalSpy publication(target->model(), &QAbstractItemModel::rowsRemoved);
        QStringList errors;
        bool submitted = false;
        QTimer driver;
        connect(&driver, &QTimer::timeout, &window, [&] {
            auto *modal = QApplication::activeModalWidget();
            if (auto *dialog = dynamic_cast<CopyDialog *>(modal); dialog && !submitted) {
                submitted = true;
                dialog->accept();
            } else if (auto *box = qobject_cast<QMessageBox *>(modal)) {
                errors << box->text();
                box->accept();
            } else if (auto *progress = dynamic_cast<CProgressDialog *>(modal)) {
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (close && close->text() == ToQString(LangString(IDS_CLOSE)))
                    close->click();
            }
        });
        driver.start(1);
        QTest::keyClick(view, move ? Qt::Key_F6 : Qt::Key_F5);
        QVERIFY(submitted);
        QVERIFY(SignalWaitSupport::waitForEnabled(*action(window, "refreshAction")));
        QVERIFY(action(window, "refreshAction")->isEnabled());
        driver.stop();
        QCOMPARE(controlled.Started, !native);
        QCOMPARE(!errors.isEmpty(), route == "failure");
        QCOMPARE(destination.GetFolder(), provider.Interface());
        QVERIFY(!publication.isEmpty());
        QVERIFY(
            SignalWaitSupport::waitForCount(*target->model(), originalCount + (success ? 1 : 0)));
        QCOMPARE(target->rowCount(), originalCount + (success ? 1 : 0));
        CSelectedState restored;
        destination.SaveSelectedState(restored);
        QCOMPARE(restored.SelectedNames.Size(), 1u);
        QCOMPARE(ToQString(restored.SelectedNames[0]), marked);
        QCOMPARE(ToQString(restored.FocusedName), ToQString(selection.FocusedName));
        QVERIFY(sourcePanel.ShouldProcessNotifications() &&
                destination.ShouldProcessNotifications());
        QVERIFY(source->updatesEnabled() && target->updatesEnabled() && view->isEnabled());
        QVERIFY(sourceUi->isAncestorOf(window.focusWidget()));
        CSelectedState sourceSelection;
        sourcePanel.SaveSelectedState(sourceSelection);
        QVERIFY(sourceSelection.SelectedNames.IsEmpty());
        QCOMPARE(QFileInfo::exists(QDir(input).filePath(added)), !(success && move));
        if (!success)
            QCOMPARE(SourceExtractFixture::readFile(archive), before);
        QProcess extract;
        const QString output = fixture.filePath("verification");
        QStringList verified{"x", "-y", "-o" + output, "--", archive, marked, focused, added};
        if (originalCount > 2)
            verified << "seed-300.txt";
        extract.start(engine_, verified);
        QVERIFY(extract.waitForFinished(10000));
        QCOMPARE(extract.exitCode(), 0);
        for (const QString &name : {marked, focused})
            QCOMPARE(SourceExtractFixture::readFile(QDir(output).filePath(name)), name.toUtf8());
        if (originalCount > 2)
            QCOMPARE(SourceExtractFixture::readFile(QDir(output).filePath("seed-300.txt")),
                     QByteArray("seed control"));
        QCOMPARE(SourceExtractFixture::readFile(QDir(output).filePath(added)),
                 success ? QByteArray("copied payload") : QByteArray());
        for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
            window.resize(size);
            QApplication::processEvents();
            QVERIFY(UiInspection::fullyVisible(*view, window));
            QVERIFY(UiInspection::fullyVisible(*target, window));
            QVERIFY(UiInspection::writeReport(window, QString("copy-archive-refresh-%1-%2")
                                                          .arg(QTest::currentDataTag())
                                                          .arg(size.width())));
        }
    }
};
QTEST_MAIN(CopyArchiveRefreshTests)
#include "copy_archive_refresh_tests.moc"
