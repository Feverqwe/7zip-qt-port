#include "7zip/UI/FileManager/ProgressDialog2Res.h"
#include "CPP/7zip/UI/FileManager/LangUtils.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "archivecodeccatalog.h"
#include "window_state_support.h"
#include <QStandardPaths>
#include <QTreeWidget>

class PanelListKeyTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    static QByteArray read(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }
  private slots:
    void internalAndParent_data() {
        QTest::addColumn<int>("pane");
        QTest::addColumn<bool>("icons");
        for (int pane : {0, 1})
            for (bool icons : {false, true})
                QTest::newRow(qPrintable(QString("%1-%2").arg(pane).arg(icons))) << pane << icons;
    }
    void internalAndParent() {
        QFETCH(int, pane);
        QFETCH(bool, icons);
        const QString settings = qEnvironmentVariable("SEVENZIP_SETTINGS_PATH");
        QFile::remove(settings);
        SaveRegLang(UString(pane ? L"ru" : L"-"), settings);
        ReloadLang(settings);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString folder = fixture.filePath(QStringLiteral("folder 空"));
        QVERIFY(QDir().mkpath(folder));
        QFile input(QDir(folder).filePath(QStringLiteral("child δ.txt")));
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("protected bytes"), qint64(15));
        input.close();
        const QString engine = QStandardPaths::findExecutable("7zz");
        QVERIFY(!engine.isEmpty());
        const QString archive = fixture.filePath(QStringLiteral("archive 空.zip"));
        QProcess create;
        create.setWorkingDirectory(folder);
        create.start(engine, {"a", "-tzip", "--", archive, "child δ.txt"});
        QVERIFY(create.waitForFinished(10000));
        QCOMPARE(create.exitCode(), 0);
        const QByteArray original = read(archive);
        QVERIFY(!original.isEmpty());
        const QString broken = fixture.filePath("broken.zip");
        QFile bad(broken);
        QVERIFY(bad.open(QIODevice::WriteOnly));
        QCOMPARE(bad.write("not an archive"), qint64(14));
        bad.close();
        ArchiveBackend backend(nullptr, engine);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE)));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY(ready[0][0].toBool());
        MainWindow window(&backend);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        auto *right = window.findChild<QWidget *>("filePanel1");
        if (!right || !right->isVisible())
            action(window, "twoPanelsAction")->trigger();
        auto *owner = window.findChild<QWidget *>(pane ? "filePanel1" : "filePanel0");
        auto *peer = window.findChild<QWidget *>(pane ? "filePanel0" : "filePanel1");
        auto *path = owner->findChild<QLineEdit *>("archivePath");
        auto *list = static_cast<FileTable *>(owner->findChild<QTableWidget *>("archiveTable"));
        auto *peerList = static_cast<FileTable *>(peer->findChild<QTableWidget *>("archiveTable"));
        path->setFocus();
        path->setText(fixture.path());
        QTest::keyClick(path, Qt::Key_Return);
        QTRY_COMPARE(list->rowCount(), 3);
        auto &panel = *list->sourcePanel;
        panel.Set_AutoRefresh_Mode(false);
        CMyComPtr<IFolderFolder> peerProvider = peerList->sourcePanel->GetFolder();
        const QString peerPath = peer->findChild<QLineEdit *>("archivePath")->text();
        const int peerRows = peerList->rowCount();
        action(window, icons ? "viewMode0Action" : "viewMode3Action")->trigger();
        auto *view =
            icons ? static_cast<QAbstractItemView *>(owner->findChild<QListView *>("iconList"))
                  : static_cast<QAbstractItemView *>(list);
        const auto focus = [&](const QString &name) {
            window.activateWindow();
            QVERIFY(QTest::qWaitForWindowActive(&window));
            int row = -1;
            for (int i = 0; i < list->rowCount(); ++i)
                if (list->item(i, 0)->data(Qt::EditRole).toString() == name)
                    row = i;
            QVERIFY(row >= 0);
            list->clearSelection();
            list->setCurrentCell(row, 0);
            view->setCurrentIndex(view->model()->index(row, 0));
            view->setFocus();
            QTRY_VERIFY(view->hasFocus());
        };
        focus(QFileInfo(folder).fileName());
        CMyComPtr<IFolderFolder> before = panel.GetFolder();
        {
            CPanel::CDisableNotify guard(panel);
            QTest::keyClick(view, Qt::Key_PageDown, Qt::ControlModifier);
            QCOMPARE(panel.GetFolder(), before.Interface());
        }
        QTest::keyClick(view, Qt::Key_PageDown, Qt::ControlModifier | Qt::ShiftModifier);
        QCOMPARE(panel.GetFolder(), before.Interface());
        focus(QFileInfo(folder).fileName());
        QTest::keyClick(view, Qt::Key_PageDown, Qt::ControlModifier);
        QTRY_COMPARE(QFileInfo(path->text()).canonicalFilePath(), folder);
        QTRY_COMPARE(list->rowCount(), 1);
        view->setFocus();
        QTest::keyClick(view, Qt::Key_PageUp, Qt::ControlModifier);
        QTRY_COMPARE(QFileInfo(path->text()).canonicalFilePath(), fixture.path());
        QTRY_COMPARE(list->rowCount(), 3);
        focus(QFileInfo(archive).fileName());
        QTest::keyClick(view, Qt::Key_PageDown, Qt::ControlModifier);
        QTRY_VERIFY(panel.IsArcFolder());
        QTRY_COMPARE(list->rowCount(), 1);
        view->setFocus();
        QTest::keyClick(view, Qt::Key_PageUp, Qt::ControlModifier);
        QTRY_VERIFY(panel.IsFSFolder());
        QTRY_COMPARE(list->rowCount(), 3);
        QStringList errors;
        QTimer dismiss;
        connect(&dismiss, &QTimer::timeout, &window, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                errors << box->text();
                box->accept();
            } else if (auto *progress =
                           dynamic_cast<CProgressDialog *>(QApplication::activeModalWidget())) {
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (close && close->property("languageID").toUInt() == IDS_CLOSE)
                    close->click();
            }
        });
        dismiss.start(5);
        focus(QFileInfo(broken).fileName());
        before = panel.GetFolder();
        QTest::keyClick(view, Qt::Key_PageDown, Qt::ControlModifier);
        // Original OpenItem treats S_FALSE (not an archive) as a silent return
        // when tryExternal is false. No invented error or external fallback.
        QVERIFY(errors.isEmpty());
        QCOMPARE(panel.GetFolder(), before.Interface());
        QCOMPARE(list->rowCount(), 3);
        QCOMPARE(peerList->sourcePanel->GetFolder(), peerProvider.Interface());
        QCOMPARE(peerList->rowCount(), peerRows);
        QCOMPARE(peer->findChild<QLineEdit *>("archivePath")->text(), peerPath);
        QCOMPARE(read(archive), original);
        QCOMPARE(read(input.fileName()), QByteArray("protected bytes"));
        QCOMPARE(read(broken), QByteArray("not an archive"));
    }
    void nestedCancellationKeepsOwner() {
        QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH"));
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "viewMode3Action")->trigger();
        openListedArchive(window, backend, fixture.filePath(QStringLiteral("outer 空.zip")),
                          {{QStringLiteral("inner δ.zip"), 1, 1}});
        auto *list = table(window);
        QTRY_COMPARE(list->rowCount(), 1);
        auto &panel = *static_cast<FileTable *>(list)->sourcePanel;
        CMyComPtr<IFolderFolder> provider = panel.GetFolder();
        const UString prefix = panel.GetCurrentFolderPrefix();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        list->setFocus();
        list->setCurrentCell(0, 0);
        QTest::keyClick(list, Qt::Key_PageDown, Qt::ControlModifier);
        QCOMPARE(backend.extractCalls, 1);
        QCOMPARE(backend.cancelCalls, 1);
        QCOMPARE(backend.lastSelected, QStringList{QStringLiteral("inner δ.zip")});
        QCOMPARE(panel.GetFolder(), provider.Interface());
        QCOMPARE(panel.GetCurrentFolderPrefix(), prefix);
        QCOMPARE(list->rowCount(), 1);
        QVERIFY(panel.ShouldProcessNotifications());
        QVERIFY(list->isEnabled());
        QStringList errors;
        QTimer dismiss;
        connect(&dismiss, &QTimer::timeout, &window, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                errors << box->text();
                box->accept();
            } else if (auto *progress =
                           dynamic_cast<CProgressDialog *>(QApplication::activeModalWidget())) {
                if (auto *messages = progress->findChild<QTreeWidget *>("IDL_PROGRESS_MESSAGES"))
                    for (int row = 0; row < messages->topLevelItemCount(); ++row)
                        errors << messages->topLevelItem(row)->text(1);
                auto *close = progress->findChild<QPushButton *>("IDCANCEL");
                if (close && close->property("languageID").toUInt() == IDS_CLOSE)
                    close->click();
            }
        });
        dismiss.start(5);
        backend.queueSourceExtractAction([&] { backend.fail("controlled extraction failure"); });
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        list->setFocus();
        QTest::keyClick(list, Qt::Key_PageDown, Qt::ControlModifier);
        QCOMPARE(backend.extractCalls, 2);
        QVERIFY(!errors.isEmpty());
        QCOMPARE(panel.GetFolder(), provider.Interface());
        QCOMPARE(panel.GetCurrentFolderPrefix(), prefix);
        QCOMPARE(list->rowCount(), 1);
        QVERIFY(panel.ShouldProcessNotifications());
    }
};
QTEST_MAIN(PanelListKeyTests)
#include "panel_list_key_tests.moc"
