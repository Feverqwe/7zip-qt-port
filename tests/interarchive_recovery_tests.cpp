#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "archivecodeccatalog.h"
#include "copydialog.h"
#include "layout_inspection.h"
#include "source_extract_fixture.h"
#include "window_state_support.h"

class ReopenFailureBackend final : public ArchiveBackend {
  public:
    QString Target;
    bool Armed = false, Fail = false, ReopenStarted = false;
    void openFolder(const QString &archive, const QString &format,
                    const QString &password) override {
        if (Armed && archive == Target) {
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
        for (const QString &name : {added, control, marked, focused})
            QVERIFY(SourceExtractFixture::writeFile(fixture.filePath(name), name.toUtf8()));
        const QString home = fixture.filePath("fallback directory");
        QVERIFY(QDir().mkdir(home));
        for (int i = 0; i < fallbackCount; ++i)
            QVERIFY(SourceExtractFixture::writeFile(QDir(home).filePath(QString::number(i)),
                                                    QByteArray("fallback")));
        const QString sourceArchive = fixture.filePath("source archive.zip");
        const QString targetArchive = fixture.filePath("target archive.zip");
        createArchive(sourceArchive, fixture.path(), {added, control});
        createArchive(targetArchive, fixture.path(), {marked, focused});
        const QByteArray sourceBefore = SourceExtractFixture::readFile(sourceArchive);
        const QByteArray targetBefore = SourceExtractFixture::readFile(targetArchive);
        ReopenFailureBackend backend;
        backend.Target = targetArchive;
        backend.Fail = route == "reopen-failure";
#ifdef SEVENZIP_CODEC_MODULE
        if (route == "native-restore")
            QCOMPARE(backend.loadCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE), version_),
                     S_OK);
#endif
        bool cancelled = false;
        connect(&backend, &ArchiveBackend::engineStarted, &backend, [&] {
            if (route == "reopen-cancel" && backend.ReopenStarted && !cancelled) {
                cancelled = true;
                backend.cancel();
            }
        });
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *sourceUi = window.findChild<QWidget *>(QString("filePanel%1").arg(pane));
        auto *targetUi = window.findChild<QWidget *>(QString("filePanel%1").arg(1 - pane));
        for (QWidget *ui : {targetUi, sourceUi}) {
            auto *address = ui->findChild<QLineEdit *>("archivePath");
            address->setFocus();
            address->setText(home);
            QTest::keyClick(address, Qt::Key_Return);
            auto *table = ui->findChild<QTableWidget *>("archiveTable");
            QTRY_COMPARE(table->rowCount(), fallbackCount);
            QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
            window.openArchive(ui == targetUi ? targetArchive : sourceArchive);
            QTRY_COMPARE(table->rowCount(), 2);
            QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
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
                    if (restoreFailure)
                        replaced = QFile::remove(targetArchive) && QDir().mkdir(targetArchive) &&
                                   SourceExtractFixture::writeFile(sentinel, QByteArray("keep"));
                    else
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
        QCOMPARE(cancelled, route == "reopen-cancel");
        QTRY_COMPARE(target->rowCount(), fallbackCount);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
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
                QTRY_COMPARE(target->rowCount(), 2);
                QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
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
                QTRY_COMPARE(target->rowCount(), 3);
                QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
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
