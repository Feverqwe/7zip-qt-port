#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "archivecodeccatalog.h"
#include "copydialog.h"
#include "layout_inspection.h"
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
    quint32 version_ = 0;

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
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString input = fixture.filePath("input folder");
        QVERIFY(QDir().mkpath(input));
        const QString added = QStringLiteral("a copied 文件.txt");
        const QString marked = QStringLiteral("m marked δ.txt");
        const QString focused = QStringLiteral("z focused 文件.bin");
        QVERIFY(SourceExtractFixture::writeFile(QDir(input).filePath(added), "copied payload"));
        for (const QString &name : {marked, focused})
            QVERIFY(SourceExtractFixture::writeFile(fixture.filePath(name), name.toUtf8()));
        QStringList archiveNames{marked, focused};
        for (int i = 2; i < originalCount; ++i) {
            const QString name = QString("seed-%1.txt").arg(i);
            QVERIFY(SourceExtractFixture::writeFile(fixture.filePath(name), "seed control"));
            archiveNames << name;
        }
        const QString archive =
            fixture.filePath("target archive." + (route == "7z" ? route : "zip"));
        QProcess create;
        create.setWorkingDirectory(fixture.path());
        create.start(engine_, QStringList{"a", "--", archive} + archiveNames);
        QVERIFY(create.waitForFinished(10000));
        QCOMPARE(create.exitCode(), 0);
        const QByteArray before = SourceExtractFixture::readFile(archive);
        CopyArchiveBackend backend;
        backend.Fail = route == "failure";
#ifdef SEVENZIP_CODEC_MODULE
        if (native)
            QCOMPARE(backend.loadCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE), version_),
                     S_OK);
#endif
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *sourceUi = window.findChild<QWidget *>(QString("filePanel%1").arg(pane));
        auto *targetUi = window.findChild<QWidget *>(QString("filePanel%1").arg(1 - pane));
        targetUi->findChild<QLineEdit *>("archivePath")->setFocus();
        window.openArchive(archive);
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        auto *target =
            static_cast<FileTable *>(targetUi->findChild<QTableWidget *>("archiveTable"));
        auto &destination = *target->sourcePanel;
        destination.Set_AutoRefresh_Mode(false);
        QTRY_COMPARE(target->rowCount(), originalCount);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
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
        address->setText(input);
        QTest::keyClick(address, Qt::Key_Return);
        auto *source =
            static_cast<FileTable *>(sourceUi->findChild<QTableWidget *>("archiveTable"));
        QTRY_COMPARE(source->rowCount(), 1);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
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
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        driver.stop();
        QCOMPARE(backend.Started, !native);
        QCOMPARE(!errors.isEmpty(), route == "failure");
        QCOMPARE(destination.GetFolder(), provider.Interface());
        QVERIFY(!publication.isEmpty());
        QTRY_COMPARE(target->rowCount(), originalCount + (success ? 1 : 0));
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
        extract.start(engine_, {"x", "-y", "-o" + output, "--", archive});
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
