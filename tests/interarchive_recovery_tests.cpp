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

class ReopenFailureBackend final : public ArchiveBackend {
  public:
    QString Target;
    bool Armed = false, Fail = false, ReopenStarted = false;
    void openFolder(const QString &archive, const QString &format,
                    const QString &password) override {
        if (Armed && QDir::cleanPath(QDir::fromNativeSeparators(archive)) ==
                         QDir::cleanPath(QDir::fromNativeSeparators(Target))) {
            ReopenStarted = true;
            if (Fail) {
                emit finished(false, QStringLiteral("Controlled rollback reopen failure."));
                return;
            }
        }
        ArchiveBackend::openFolder(archive, format, password);
    }
};

class InterarchiveRecoveryTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    QString engine_;
    quint32 version_ = 0;
    QTemporaryDir seeds_;

    void createArchive(const QString &path, const QString &root, const QStringList &names) {
        QProcess process;
        process.setWorkingDirectory(root);
        process.start(engine_, QStringList{"a", "--", path} + names);
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
    }
    void verifyArchive(const QString &path, const QString &output, const QStringList &names) {
        QProcess process;
        process.start(engine_, {"x", "-y", "-o" + output, "--", path});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        QCOMPARE(QDir(output).entryList(QDir::Files).size(), names.size());
        for (const QString &name : names)
            QCOMPARE(SourceExtractFixture::readFile(QDir(output).filePath(name)), name.toUtf8());
    }

  private slots:
    void initTestCase() {
        engine_ = QStandardPaths::findExecutable("7zz");
        QVERIFY(!engine_.isEmpty());
        QVERIFY(seeds_.isValid());
        const QString added = QStringLiteral("a copied 文件.txt");
        const QString control = QStringLiteral("zz source control δ.bin");
        const QString marked = QStringLiteral("m marked δ.txt");
        const QString focused = QStringLiteral("z focused 文件.bin");
        for (const QString &name : {added, control, marked, focused})
            QVERIFY(SourceExtractFixture::writeFile(seeds_.filePath(name), name.toUtf8()));
        createArchive(seeds_.filePath("source.zip"), seeds_.path(), {added, control});
        createArchive(seeds_.filePath("target.zip"), seeds_.path(), {marked, focused});
        for (int count : {1, 301}) {
            const QString home =
                seeds_.filePath(QStringLiteral("fallback directory %1 文件").arg(count));
            QVERIFY(QDir().mkdir(home));
            for (int i = 0; i < count; ++i)
                QVERIFY(SourceExtractFixture::writeFile(QDir(home).filePath(QString::number(i)),
                                                        QByteArray("fallback")));
        }
#ifdef SEVENZIP_CODEC_MODULE
        ArchiveCodecCatalog catalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE), 0);
        QCOMPARE(catalog.result(), S_OK);
        version_ = catalog.version();
#endif
    }
    void rollbackFailureAndRetry_data() {
        QTest::addColumn<unsigned>("pane");
        QTest::addColumn<bool>("russian");
        QTest::addColumn<bool>("icons");
        QTest::addColumn<bool>("alternative");
        QTest::addColumn<QString>("route");
        QStringList routes{"restore", "restore-large", "reopen-failure", "reopen-cancel"};
#ifdef SEVENZIP_CODEC_MODULE
        routes << "native-restore";
#endif
        for (unsigned pane : {0u, 1u})
            for (bool russian : {false, true})
                for (bool icons : {false, true})
                    for (bool alternative : {false, true})
                        for (const QString &route : routes)
                            QTest::newRow(qPrintable(QString("%1-%2-%3-%4-%5")
                                                         .arg(pane)
                                                         .arg(russian)
                                                         .arg(icons)
                                                         .arg(alternative)
                                                         .arg(route)))
                                << pane << russian << icons << alternative << route;
#ifdef SEVENZIP_CODEC_MODULE
        QTest::newRow("native-reopen-failure")
            << 0u << false << false << false << QString("native-reopen-failure");
        QTest::newRow("native-reopen-cancel")
            << 1u << true << true << true << QString("native-reopen-cancel");
#endif
    }
    void rollbackFailureAndRetry() {
        QFETCH(unsigned, pane);
        QFETCH(bool, russian);
        QFETCH(bool, icons);
        QFETCH(bool, alternative);
        QFETCH(QString, route);
        const bool restoreFailure = route.contains("restore");
        const int fallbackCount = route == "restore-large" ? 301 : 1;
        const QString settingsPath = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settingsPath);
        SaveRegLang(UString(russian ? L"ru" : L"-"), settingsPath);
        ReloadLang(settingsPath);
        QSettings settings(settingsPath, QSettings::IniFormat);
        settings.setValue("FM/AlternativeSelection", alternative);
        settings.setValue("FM/ShowDots", false);
        settings.sync();
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString added = QStringLiteral("a copied 文件.txt");
        const QString control = QStringLiteral("zz source control δ.bin");
        const QString marked = QStringLiteral("m marked δ.txt");
        const QString focused = QStringLiteral("z focused 文件.bin");
        const QString home =
            seeds_.filePath(QStringLiteral("fallback directory %1 文件").arg(fallbackCount));
        const QString sourceArchive = fixture.filePath("source archive.zip");
        const QString targetArchive = fixture.filePath("target archive.zip");
        QVERIFY(QFile::copy(seeds_.filePath("source.zip"), sourceArchive));
        QVERIFY(QFile::copy(seeds_.filePath("target.zip"), targetArchive));
        const QByteArray sourceBefore = SourceExtractFixture::readFile(sourceArchive);
        const QByteArray targetBefore = SourceExtractFixture::readFile(targetArchive);
        ReopenFailureBackend backend;
        backend.Target = targetArchive;
        backend.Fail = route.endsWith("reopen-failure");
#ifdef SEVENZIP_CODEC_MODULE
        if (route.startsWith("native-"))
            QCOMPARE(backend.loadCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE), version_),
                     S_OK);
#endif
        bool cancelled = false;
        connect(&backend, &ArchiveBackend::engineStarted, &backend, [&] {
            if (route.endsWith("reopen-cancel") && backend.ReopenStarted && !cancelled) {
                cancelled = true;
                backend.cancel();
            }
        });
        CWindowInfo windowInfo{};
        windowInfo.rect.right = 980;
        windowInfo.rect.bottom = 620;
        windowInfo.numPanels = 2;
        windowInfo.currentPanel = pane;
        windowInfo.splitterPos = 490;
        windowInfo.Save(settingsPath);
        for (unsigned index : {0u, 1u})
            SavePanelPath(index, ToUString(home), settingsPath);
        QtFileManagerStartup startup;
        startup.FullPath = ToUString(sourceArchive);
        startup.NeedOpenArc = true;
        MainWindow window(&backend, {}, nullptr, &startup);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        auto *sourceUi = window.findChild<QWidget *>(QString("filePanel%1").arg(pane));
        auto *targetUi = window.findChild<QWidget *>(QString("filePanel%1").arg(1 - pane));
        for (QWidget *ui : {targetUi, sourceUi}) {
            auto *table = ui->findChild<QTableWidget *>("archiveTable");
            const int initialCount = ui == targetUi ? fallbackCount : 2;
            QVERIFY(SignalWaitSupport::waitForCount(*table->model(), initialCount));
            QCOMPARE(table->rowCount(), initialCount);
            QTRY_VERIFY(table->isEnabled() && table->updatesEnabled());
        }
        // Finish both initial publications before choosing an operation owner;
        // the source initial-focus request can otherwise replace this focus.
        QVERIFY(SignalWaitSupport::waitForEnabled(*action(window, "refreshAction")));
        QVERIFY(action(window, "refreshAction")->isEnabled());
        // The source is opened by the ordinary startup path. Only the target
        // needs a filesystem-to-archive transition to retain its fallback folder.
        for (QWidget *ui : {targetUi}) {
            auto *address = ui->findChild<QLineEdit *>("archivePath");
            QCOMPARE(QFileInfo(address->text()).canonicalFilePath(),
                     QFileInfo(home).canonicalFilePath());
            QVERIFY(SignalWaitSupport::waitForEnabled(*action(window, "refreshAction")));
            QVERIFY(action(window, "refreshAction")->isEnabled());
            window.activateWindow();
            QVERIFY(QTest::qWaitForWindowActive(&window));
            address->setFocus();
            QVERIFY(ui->isAncestorOf(window.focusWidget()));
            auto *table = ui->findChild<QTableWidget *>("archiveTable");
            window.openArchive(targetArchive);
            QVERIFY2(SignalWaitSupport::waitForCount(*table->model(), 2),
                     qPrintable(QString("owner=%1 rows=%2 source=%3 target=%4 status=%5")
                                    .arg(ui->objectName())
                                    .arg(table->rowCount())
                                    .arg(sourceUi->findChild<QLineEdit *>("archivePath")->text(),
                                         targetUi->findChild<QLineEdit *>("archivePath")->text(),
                                         window.statusBar()->currentMessage())));
            QCOMPARE(table->rowCount(), 2);
            QTRY_VERIFY(table->isEnabled() && table->updatesEnabled() &&
                        static_cast<FileTable *>(table)->sourcePanel->ShouldProcessNotifications());
            QVERIFY(SignalWaitSupport::waitForEnabled(*action(window, "refreshAction")));
            QVERIFY(action(window, "refreshAction")->isEnabled());
            if (icons)
                action(window, "viewMode0Action")->trigger();
        }
        auto *source =
            static_cast<FileTable *>(sourceUi->findChild<QTableWidget *>("archiveTable"));
        auto *target =
            static_cast<FileTable *>(targetUi->findChild<QTableWidget *>("archiveTable"));
        CPanel &sourcePanel = *source->sourcePanel;
        CPanel &destination = *target->sourcePanel;
        for (CPanel *panel : {&sourcePanel, &destination})
            panel->Set_AutoRefresh_Mode(false);
        CMyComPtr<IFolderFolder> sourceProvider = sourcePanel.GetFolder();
        CMyComPtr<IFolderFolder> targetProvider = destination.GetFolder();
        QVERIFY(dynamic_cast<CAgentFolder *>(sourceProvider.Interface()));
        auto view = [icons](QWidget *ui) -> QAbstractItemView * {
            return icons ? static_cast<QAbstractItemView *>(ui->findChild<QListView *>("iconList"))
                         : ui->findChild<QTableWidget *>("archiveTable");
        };
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        sourceUi->findChild<QLineEdit *>("archivePath")->setFocus();
        if (icons)
            action(window, "viewMode0Action")->trigger();
        sourcePanel.SelectAll(false);
        sourcePanel.SelectSpec(ToUString(added), true);
        view(sourceUi)->setFocus();
        QVERIFY(sourceUi->isAncestorOf(window.focusWidget()));
        QStringList errors;
        bool submitted = false, refused = false, committed = false, replaced = false;
        const QString sentinel = QDir(targetArchive).filePath("keep sentinel δ.txt");
        QTimer driver;
        connect(&driver, &QTimer::timeout, &window, [&] {
            auto *modal = QApplication::activeModalWidget();
            if (auto *dialog = dynamic_cast<CopyDialog *>(modal); dialog && !submitted) {
                submitted = true;
                dialog->accept();
            } else if (auto *box = qobject_cast<QMessageBox *>(modal)) {
                if (box->button(QMessageBox::Yes)) {
                    refused = true;
                    committed = SourceExtractFixture::readFile(targetArchive) != targetBefore;
                    if (restoreFailure) {
                        // Quiesce this private native fixture before replacing
                        // its read-shared Windows file with a sentinel directory.
                        auto *folder = dynamic_cast<CAgentFolder *>(destination.GetFolder());
                        const bool released =
                            route != "native-restore" || (folder && folder->CloseArchive() == S_OK);
                        replaced = released && QFile::remove(targetArchive) &&
                                   QDir().mkdir(targetArchive) &&
                                   SourceExtractFixture::writeFile(sentinel, QByteArray("keep"));
                    } else
                        backend.Armed = true;
                    box->button(QMessageBox::No)->click();
                } else {
                    errors << box->text();
                    box->accept();
                }
            } else if (auto *progress = dynamic_cast<CProgressDialog *>(modal)) {
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (close && close->text() == ToQString(LangString(IDS_CLOSE)))
                    close->click();
            }
        });
        driver.start(1);
        QTest::keyClick(view(sourceUi), Qt::Key_F6);
        QVERIFY(submitted);
        QVERIFY(refused);
        QVERIFY(committed);
        QCOMPARE(replaced, restoreFailure);
        QCOMPARE(cancelled, route.endsWith("reopen-cancel"));
        QVERIFY(SignalWaitSupport::waitForCount(*target->model(), fallbackCount));
        QCOMPARE(target->rowCount(), fallbackCount);
        QVERIFY(SignalWaitSupport::waitForEnabled(*action(window, "refreshAction")));
        QVERIFY(action(window, "refreshAction")->isEnabled());
        QCOMPARE(errors.size(), restoreFailure ? 2 : (cancelled ? 0 : 1));
        if (restoreFailure) {
            QVERIFY(errors.last().contains(russian ? QStringLiteral("откатить") : "roll back"));
            QCOMPARE(SourceExtractFixture::readFile(sentinel), QByteArray("keep"));
        } else {
            QVERIFY(backend.ReopenStarted);
            QCOMPARE(SourceExtractFixture::readFile(targetArchive), targetBefore);
        }
        QVERIFY(destination.GetFolder() != targetProvider.Interface());
        QVERIFY(destination.IsFSFolder());
        QCOMPARE(
            QFileInfo(targetUi->findChild<QLineEdit *>("archivePath")->text()).canonicalFilePath(),
            QFileInfo(home).canonicalFilePath());
        QCOMPARE(sourcePanel.GetFolder(), sourceProvider.Interface());
        QCOMPARE(SourceExtractFixture::readFile(sourceArchive), sourceBefore);
        QCOMPARE(source->rowCount(), 2);
        QVERIFY(sourceUi->isAncestorOf(window.focusWidget()));
        for (const QString &phase : {QString("failed"), QString("recovered")}) {
            if (phase == "recovered") {
                backend.Armed = false;
                if (restoreFailure) {
                    QVERIFY(QDir().rename(targetArchive, fixture.filePath("preserved directory")));
                    QVERIFY(SourceExtractFixture::writeFile(targetArchive, targetBefore));
                }
                window.activateWindow();
                QVERIFY(QTest::qWaitForWindowActive(&window));
                targetUi->findChild<QLineEdit *>("archivePath")->setFocus();
                QVERIFY(targetUi->isAncestorOf(window.focusWidget()));
                window.openArchive(targetArchive);
                QVERIFY(SignalWaitSupport::waitForCount(*target->model(), 2));
                QCOMPARE(target->rowCount(), 2);
                QVERIFY(SignalWaitSupport::waitForEnabled(*action(window, "refreshAction")));
                QVERIFY(action(window, "refreshAction")->isEnabled());
                window.activateWindow();
                QVERIFY(QTest::qWaitForWindowActive(&window));
                targetProvider = destination.GetFolder();
                destination.SelectAll(false);
                destination.SelectSpec(ToUString(marked), true);
                target->setCurrentCell(1, 0, QItemSelectionModel::NoUpdate);
                sourcePanel.SelectSpec(ToUString(added), true);
                view(sourceUi)->setFocus();
                submitted = false;
                errors.clear();
                QTest::keyClick(view(sourceUi), Qt::Key_F5);
                QVERIFY(submitted);
                QVERIFY(SignalWaitSupport::waitForCount(*target->model(), 3));
                QCOMPARE(target->rowCount(), 3);
                QVERIFY(SignalWaitSupport::waitForEnabled(*action(window, "refreshAction")));
                QVERIFY(action(window, "refreshAction")->isEnabled());
                QVERIFY(errors.isEmpty());
                QCOMPARE(destination.GetFolder(), targetProvider.Interface());
                CSelectedState selection;
                destination.SaveSelectedState(selection);
                QCOMPARE(selection.SelectedNames.Size(), 1u);
                QCOMPARE(ToQString(selection.SelectedNames[0]), marked);
                QCOMPARE(ToQString(selection.FocusedName), focused);
                verifyArchive(sourceArchive, fixture.filePath("verify source"), {added, control});
                verifyArchive(targetArchive, fixture.filePath("verify target"),
                              {added, marked, focused});
                if (restoreFailure)
                    QCOMPARE(SourceExtractFixture::readFile(
                                 fixture.filePath("preserved directory/keep sentinel δ.txt")),
                             QByteArray("keep"));
            }
            QCOMPARE(sourcePanel.GetFolder(), sourceProvider.Interface());
            QCOMPARE(SourceExtractFixture::readFile(sourceArchive), sourceBefore);
            CSelectedState selection;
            sourcePanel.SaveSelectedState(selection);
            QVERIFY(selection.SelectedNames.IsEmpty());
            QVERIFY(sourcePanel.ShouldProcessNotifications() &&
                    destination.ShouldProcessNotifications());
            QVERIFY(source->updatesEnabled() && target->updatesEnabled());
            QVERIFY(sourceUi->isAncestorOf(window.focusWidget()));
            for (const QSize size : {QSize(980, 620), QSize(640, 400)}) {
                window.resize(size);
                QApplication::processEvents();
                for (QWidget *ui : {sourceUi, targetUi}) {
                    QVERIFY(view(ui)->isEnabled());
                    QVERIFY(UiInspection::fullyVisible(*view(ui), window));
                }
                QVERIFY(UiInspection::writeReport(window, QString("interarchive-recovery-%1-%2-%3")
                                                              .arg(QTest::currentDataTag(), phase)
                                                              .arg(size.width())));
            }
        }
        driver.stop();
    }
};
QTEST_MAIN(InterarchiveRecoveryTests)
#include "interarchive_recovery_tests.moc"
