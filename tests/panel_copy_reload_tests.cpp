#include "CPP/7zip/UI/FileManager/FSFolder.h"
#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "copydialog.h"
#include "filesystemscanner.h"
#include "layout_inspection.h"
#include "panel_load_folder.h"
#include "window_state_support.h"
#include <new>

class PanelCopyReloadTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void copyCompletionSurvivesPeerLoadFailure_data() {
        QTest::addColumn<unsigned>("sourceIndex");
        QTest::addColumn<bool>("move");
        QTest::addColumn<QString>("phase");
        QTest::addColumn<bool>("filesystemPeer");
        for (unsigned index : {0u, 1u})
            for (bool move : {false, true})
                for (const QString phase : {"load", "cancel", "property", "type", "count"})
                    QTest::newRow(qPrintable(QString("%1-%2-%3").arg(index).arg(move).arg(phase)))
                        << index << move << phase << false;
        for (unsigned index : {0u, 1u})
            for (bool move : {false, true})
                for (const QString phase : {"cancel", "allocation", "worker"})
                    QTest::newRow(
                        qPrintable(QString("fs-%1-%2-%3").arg(index).arg(move).arg(phase)))
                        << index << move << phase << true;
    }
    void copyCompletionSurvivesPeerLoadFailure() {
        QFETCH(unsigned, sourceIndex);
        QFETCH(bool, move);
        QFETCH(QString, phase);
        QFETCH(bool, filesystemPeer);
        HRESULT expected = phase == "cancel"       ? E_ABORT
                           : phase == "allocation" ? E_OUTOFMEMORY
                           : phase == "type"       ? E_FAIL
                                                   : E_INVALIDARG;
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString input = fixture.filePath("input");
        const QString output = fixture.filePath("output");
        QVERIFY(QDir().mkpath(input) && QDir().mkpath(output));
        const QString name = QStringLiteral("payload 文件 δ.txt");
        const QByteArray bytes("copy\0control", 12);
        QFile sourceFile(QDir(input).filePath(name));
        QVERIFY(sourceFile.open(QIODevice::WriteOnly));
        QCOMPARE(sourceFile.write(bytes), qint64(bytes.size()));
        sourceFile.close();
        ControlledBackend backend;
        MainWindow window(&backend);
        window.show();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        action(window, "twoPanelsAction")->trigger();
        auto *peerUi = window.findChild<QWidget *>(QString("filePanel%1").arg(1 - sourceIndex));
        auto *peerView =
            static_cast<FileTable *>(peerUi->findChild<QTableWidget *>("archiveTable"));
        peerView->setFocus();
        QTRY_VERIFY(peerView->hasFocus());
        LoadFolder *spec = nullptr;
        CMyComPtr<IFolderFolder> folder;
        NFsFolder::CFSFolder *filesystem = nullptr;
        const int peerRows = phase == "worker" ? 301 : filesystemPeer ? 1 : 2;
        const QString peerControl = fixture.filePath(QStringLiteral("peer/seed 文件.txt"));
        if (filesystemPeer) {
            const QString peer = fixture.filePath("peer");
            QVERIFY(QDir().mkpath(peer));
            QFile seed(peerControl);
            QVERIFY(seed.open(QIODevice::WriteOnly));
            QCOMPARE(seed.write("peer control"), qint64(12));
            seed.close();
            if (phase == "worker")
                for (int i = 0; i < 300; ++i) {
                    QFile extra(QDir(peer).filePath(QString("seed-%1.txt").arg(i)));
                    QVERIFY(extra.open(QIODevice::WriteOnly));
                    QCOMPARE(extra.write("control"), qint64(7));
                }
            auto *peerAddress = peerUi->findChild<QLineEdit *>("archivePath");
            QVERIFY(peerAddress);
            peerAddress->setFocus();
            peerAddress->setText(peer);
            QTest::keyClick(peerAddress, Qt::Key_Return);
            QTRY_COMPARE(peerView->rowCount(), peerRows);
            QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
            folder = peerView->sourcePanel->GetFolder();
            filesystem = dynamic_cast<NFsFolder::CFSFolder *>((IFolderFolder *)folder);
            QVERIFY(filesystem);
        } else {
            action(window, "openRootAction")->trigger();
            spec = new LoadFolder;
            folder = spec;
            peerView->sourcePanel->SetNewFolder(folder);
            peerView->sourcePanel->OnReload();
            QCOMPARE(peerView->rowCount(), peerRows);
        }
        peerView->selectRow(0);
        auto *sourceUi = window.findChild<QWidget *>(QString("filePanel%1").arg(sourceIndex));
        auto *address = sourceUi->findChild<QLineEdit *>("archivePath");
        address->setFocus();
        address->setText(input);
        QTest::keyClick(address, Qt::Key_Return);
        auto *sourceView =
            static_cast<FileTable *>(sourceUi->findChild<QTableWidget *>("archiveTable"));
        QTRY_COMPARE(sourceView->rowCount(), 1);
        QTRY_VERIFY(action(window, "refreshAction")->isEnabled());
        sourceView->selectRow(0);
        sourceView->setFocus();
        QTRY_VERIFY(sourceView->hasFocus());
        bool submitted = false;
        QString shown;
        FilesystemScanner *peerScanner =
            window.findChildren<FilesystemScanner *>()[1 - sourceIndex];
        QSignalSpy workerFailed(peerScanner, &FilesystemScanner::failed);
        QMetaObject::Connection restart;
        bool restarted = false;
        QTimer driver;
        connect(&driver, &QTimer::timeout, &window, [&] {
            if (auto *dialog = window.findChild<CopyDialog *>("copyDialog");
                dialog && dialog->isVisible() && !submitted) {
                submitted = true;
                if (phase == "worker") {
                    restart =
                        connect(peerScanner, &FilesystemScanner::advanced, &window, [&](quint64) {
                            if (!restarted) {
                                restarted = true;
                                peerScanner->start(fixture.filePath("missing peer"), false);
                            }
                        });
                } else if (filesystem) {
                    filesystem->CheckBreak = [phase](const QString &) {
                        if (phase == "allocation")
                            throw std::bad_alloc();
                        return false;
                    };
                } else {
                    spec->loadResult = phase == "cancel" ? E_ABORT
                                       : phase == "load" ? E_INVALIDARG
                                                         : S_OK;
                    spec->directoryResult = phase == "property" ? E_INVALIDARG : S_OK;
                    spec->directoryType = phase == "type" ? VT_UI4 : VT_BOOL;
                    spec->loadedCountResult = phase == "count" ? E_INVALIDARG : S_OK;
                }
                dialog->findChild<QComboBox *>("copyDestination")->setCurrentText(output + '/');
                dialog->accept();
            }
            for (QWidget *widget : QApplication::topLevelWidgets())
                if (auto *box = qobject_cast<QMessageBox *>(widget); box && box->isVisible()) {
                    shown = box->text();
                    box->accept();
                }
        });
        driver.start(0);
        QTest::keyClick(sourceView, move ? Qt::Key_F6 : Qt::Key_F5);
        QVERIFY(submitted);
        QTRY_VERIFY(!shown.isEmpty() && action(window, "refreshAction")->isEnabled());
        driver.stop();
        if (phase == "worker") {
            QVERIFY(restarted);
            QCOMPARE(workerFailed.size(), 1);
            expected = HRESULT(workerFailed.first().first().toInt());
            QVERIFY(expected != S_OK);
            disconnect(restart);
        }
        QCOMPARE(shown, ToQString(HResultToMessage(expected)));
        QFile result(QDir(output).filePath(name));
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), bytes);
        QCOMPARE(QFileInfo::exists(sourceFile.fileName()), !move);
        if (!move) {
            QVERIFY(sourceFile.open(QIODevice::ReadOnly));
            QCOMPARE(sourceFile.readAll(), bytes);
        }
        QCOMPARE(peerView->rowCount(), 0);
        QVERIFY(sourceView->selectionModel()->selectedRows().isEmpty());
        QVERIFY(sourceView->sourcePanel->ShouldProcessNotifications());
        QVERIFY(peerView->sourcePanel->ShouldProcessNotifications());
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(&window));
        QTRY_VERIFY(sourceView->hasFocus());
        QCOMPARE(QDir::cleanPath(window.windowTitle()), input);
        QCOMPARE(peerView->sourcePanel->GetFolder(), (IFolderFolder *)folder);
        if (phase == "worker")
            for (const QSize size : {QSize(1100, 640), QSize(640, 400)}) {
                window.resize(size);
                QApplication::processEvents();
                QVERIFY(UiInspection::writeReport(window, QString("filesystem-worker-copy-%1-%2")
                                                              .arg(QTest::currentDataTag())
                                                              .arg(size.width())));
            }
        if (filesystem)
            filesystem->CheckBreak = {};
        else {
            spec->loadResult = spec->directoryResult = spec->countResult = spec->loadedCountResult =
                S_OK;
            spec->directoryType = VT_BOOL;
        }
        peerView->sourcePanel->OnReload();
        QTRY_COMPARE(peerView->rowCount(), peerRows);
        if (filesystem) {
            QFile seed(peerControl);
            QVERIFY(seed.open(QIODevice::ReadOnly));
            QCOMPARE(seed.readAll(), QByteArray("peer control"));
        }
    }
};

QTEST_MAIN(PanelCopyReloadTests)
#include "panel_copy_reload_tests.moc"
