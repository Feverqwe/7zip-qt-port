#include "archivecodeccatalog.h"
#include "filetable.h"
#include "fixtures/drag_event_position.h"
#include "fixtures/operation_temp_cleanup.h"
#include "fixtures/scoped_environment.h"
#include "native_archive_fixture.h"
#include "qtpaneldragtransport.h"
#include "source_extract_fixture.h"
#include "window_state_support.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace {
class DropBackend final : public ArchiveBackend {
  public:
    QList<QtOperationResult> Results;
    QString Barrier;
    bool Cancel = false;
    bool Fail = false;
    unsigned Accepted = 0;
    QtOperationLauncher *startGuiCompress(const QString &archive, const QString &type,
                                          bool addExtension, const QStringList &paths, bool email,
                                          bool showDialog, QString &error) override {
        QtOperationMessage job;
        job.Type = QtOperationMessage::Kind::Job;
        job.Operation = QtOperationMessage::Command::Compress;
        job.ArchivePath = archive;
        job.ArchiveType = Fail ? QStringLiteral("unsupported-drag-format") : type;
        job.AddExtension = addExtension;
        job.Paths = paths;
        job.EMail = email;
        job.ShowDialog = showDialog;
        ScopedTestEnvironment decision("SEVENZIP_OPERATION_CREATE_DECISION",
                                       Cancel ? "cancel" : "accept");
        ScopedTestEnvironment barrier("SEVENZIP_OPERATION_BARRIER", Barrier);
        auto *operation =
            QtOperationLauncher::Launch(QStringLiteral(SEVENZIP_OPERATION_HOST), job, error);
        if (operation) {
            connect(operation, &QtOperationLauncher::accepted, this, [this] { ++Accepted; });
            connect(operation, &QtOperationLauncher::finished, this,
                    [this](const QtOperationResult &result) { Results.append(result); });
        }
        return operation;
    }
};
} // namespace

class DragDropAddUiTests final : public QObject {
    Q_OBJECT
  private slots:
    void init() { QFile::remove(qEnvironmentVariable("SEVENZIP_SETTINGS_PATH")); }
    void cleanup() { QVERIFY(CleanupOperationTestTemps()); }
    void externalArchiveDrop_data() {
        QTest::addColumn<bool>("valid");
        QTest::addColumn<bool>("multiple");
        QTest::addColumn<bool>("peer");
        QTest::newRow("open-archive") << true << false << false;
        QTest::newRow("open-in-other-panel") << true << false << true;
        QTest::newRow("damaged-archive") << false << false << false;
        QTest::newRow("multiple-files-add") << true << true << false;
    }
    void externalArchiveDrop() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Requires the official native module");
#endif
        QFETCH(bool, valid);
        QFETCH(bool, multiple);
        QFETCH(bool, peer);
        QTemporaryDir root;
        QFile input(root.filePath(QStringLiteral("содержимое.txt")));
        QVERIFY(input.open(QIODevice::WriteOnly));
        input.write("archive drop content");
        input.close();
        const QString archive = root.filePath(QStringLiteral("мой архив.ZIP"));
        DropBackend backend;
        backend.Cancel = multiple;
#ifdef SEVENZIP_CODEC_MODULE
        ArchiveCodecCatalog probe(QString::fromUtf8(SEVENZIP_CODEC_MODULE), 0);
        QCOMPARE(probe.result(), S_OK);
        QCOMPARE(
            backend.loadCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE), probe.version()),
            S_OK);
#endif
        QSignalSpy created(&backend, &ArchiveBackend::finished);
        QVERIFY(NativeArchiveFixture::WriteFiles(archive, {input.fileName()}));
        if (!valid) {
            QFile damaged(archive);
            QVERIFY(damaged.open(QIODevice::WriteOnly | QIODevice::Truncate));
            damaged.write("damaged archive");
        }
        MainWindow window(&backend);
        window.show();
        window.activateWindow();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(root.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        QTRY_VERIFY(table->isEnabled());
        QLineEdit *otherAddress = nullptr;
        if (peer) {
            auto *two = window.findChild<QAction *>("twoPanelsAction");
            if (!two->isChecked())
                two->trigger();
            auto *right = window.findChild<QWidget *>("filePanel1");
            QVERIFY(right && right->isVisible());
            otherAddress = address;
            address = right->findChild<QLineEdit *>("archivePath");
            table = right->findChild<QTableWidget *>("archiveTable");
            address->setFocus();
            address->setText(root.path());
            QTest::keyClick(address, Qt::Key_Return);
            QTRY_VERIFY(table->isEnabled());
            QTRY_VERIFY(table->rowCount() >= 2);
            QTRY_VERIFY((otherAddress->setFocus(), otherAddress->hasFocus()));
        }
        QMimeData mime;
        QList<QUrl> urls{QUrl::fromLocalFile(archive)};
        if (multiple)
            urls.append(QUrl::fromLocalFile(input.fileName()));
        mime.setUrls(urls);
        QDragEnterEvent enter(DragEventPosition(QPoint(5, 5)), Qt::CopyAction | Qt::MoveAction,
                              &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(table->viewport(), &enter);
        QVERIFY(enter.isAccepted());
        bool errorShown = false;
        QTimer respond;
        connect(&respond, &QTimer::timeout, &window, [&] {
            if (auto *error = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                errorShown = true;
                error->accept();
            }
        });
        respond.start(5);
        QDropEvent drop(QPointF(5, 5), Qt::CopyAction | Qt::MoveAction, &mime, Qt::NoButton,
                        Qt::NoModifier);
        QCoreApplication::sendEvent(table->viewport(), &drop);
        respond.stop();
        if (multiple) {
            QTRY_COMPARE(backend.Results.size(), 1);
            QCOMPARE(backend.Results.first().Status, QtOperationResult::Outcome::Cancelled);
        }
        QCOMPARE(errorShown, !valid);
        QVERIFY(QFileInfo::exists(archive));
        if (valid && !multiple) {
            QVERIFY(drop.isAccepted());
            QCOMPARE(drop.dropAction(), Qt::CopyAction);
            QVERIFY(address->text().contains(QFileInfo(archive).fileName()));
            bool found = false;
            for (int row = 0; row < table->rowCount(); ++row)
                found |= table->item(row, 0) &&
                         table->item(row, 0)->text() == QFileInfo(input.fileName()).fileName();
            QVERIFY(found);
            if (otherAddress)
                QCOMPARE(QDir::cleanPath(otherAddress->text()), QDir::cleanPath(root.path()));
        } else {
            QCOMPARE(QDir::cleanPath(address->text()), QDir::cleanPath(root.path()));
        }
    }
    void externalFilesystemDropCreatesArchiveWithNativeName() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Source drop Add requires the official native module");
#endif
        QTemporaryDir root;
        QVERIFY(QDir().mkpath(root.filePath("input")));
        QVERIFY(QDir().mkpath(root.filePath("output")));
        const QString name = QStringLiteral("добавленный файл.bin");
        QFile input(QDir(root.filePath("input")).filePath(name));
        QVERIFY(input.open(QIODevice::WriteOnly));
        const QByteArray bytes("Add\0native bytes", 16);
        QCOMPARE(input.write(bytes), qint64(bytes.size()));
        input.close();
        DropBackend backend;
#ifdef SEVENZIP_CODEC_MODULE
        ArchiveCodecCatalog probe(QString::fromUtf8(SEVENZIP_CODEC_MODULE), 0);
        QCOMPARE(probe.result(), S_OK);
        QCOMPARE(
            backend.loadCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE), probe.version()),
            S_OK);
#endif
        MainWindow window(&backend);
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(root.filePath("output"));
        QTest::keyClick(address, Qt::Key_Return);
        auto *table = window.findChild<QTableWidget *>("archiveTable");
        QTRY_VERIFY(table->isEnabled());
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(input.fileName())});
        QDragEnterEvent enter(DragEventPosition(QPoint(5, 5)), Qt::CopyAction, &mime,
                              Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(table->viewport(), &enter);
        QVERIFY(enter.isAccepted());
        const QString nameReport = root.filePath("dialog-name.json");
        ScopedTestEnvironment report("SEVENZIP_OPERATION_CREATE_NAME_REPORT", nameReport);
        QDropEvent drop(QPointF(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(table->viewport(), &drop);
        QTRY_COMPARE_WITH_TIMEOUT(backend.Results.size(), 1, 30000);
        QCOMPARE(backend.Results.first().Status, QtOperationResult::Outcome::Success);
        const auto dialog =
            QJsonDocument::fromJson(SourceExtractFixture::readFile(nameReport)).object();
        const QString archiveName = dialog.value("name").toString();
        const QString format = dialog.value("format").toString();
        QVERIFY(!format.isEmpty());
        QCOMPARE(archiveName, QFileInfo(name).completeBaseName() + "." + format);
        QVERIFY(drop.isAccepted());
        const QString archive = QDir(root.filePath("output")).filePath(archiveName);
        QVERIFY(QFileInfo::exists(archive));
        QVERIFY(!QFileInfo::exists(QDir(root.filePath("output")).filePath(name)));
        QVERIFY(QFileInfo::exists(input.fileName()));
        QVERIFY(!backend.busy());
        ArchiveBackend verifier;
        QSignalSpy finished(&verifier, &ArchiveBackend::finished);
        verifier.list(archive);
        QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        const QString output = root.filePath("verify");
        verifier.extract(archive, output);
        if (finished.isEmpty())
            QVERIFY(finished.wait(30000));
        QVERIFY(finished.takeFirst().first().toBool());
        QFile result(QDir(output).filePath(name));
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), bytes);
    }

    void sourceLifetimeAndIndependentCompletion_data() {
        QTest::addColumn<bool>("wait");
        QTest::addColumn<QString>("outcome");
        for (bool wait : {false, true})
            for (const QString &outcome :
                 {QString("create"), QString("cancel"), QString("failure")})
                QTest::newRow(
                    qPrintable(QString("%1-%2").arg(wait ? "temporary" : "ordinary").arg(outcome)))
                    << wait << outcome;
    }
    void sourceLifetimeAndIndependentCompletion() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Requires the official native module");
#else
        QFETCH(bool, wait);
        QFETCH(QString, outcome);
        const bool cancel = outcome == "cancel", failure = outcome == "failure";
        QTemporaryDir input, output;
        QVERIFY(input.isValid() && output.isValid());
        const QString source = input.filePath(QStringLiteral("drop 空 с пробелом.bin"));
        const QByteArray bytes("temporary drop bytes");
        QVERIFY(SourceExtractFixture::writeFile(source, bytes));
        DropBackend backend;
        backend.Barrier = output.filePath("continue");
        backend.Cancel = cancel;
        backend.Fail = failure;
        ArchiveCodecCatalog catalog(QStringLiteral(SEVENZIP_CODEC_MODULE), 0);
        QCOMPARE(backend.loadCodecCatalog(QStringLiteral(SEVENZIP_CODEC_MODULE), catalog.version()),
                 S_OK);
        MainWindow window(&backend);
        window.show();
        auto *address = window.findChild<QLineEdit *>("archivePath");
        address->setText(output.path());
        QTest::keyClick(address, Qt::Key_Return);
        auto *table = dynamic_cast<FileTable *>(window.findChild<QTableWidget *>("archiveTable"));
        QVERIFY(table);
        QTRY_VERIFY(table->isEnabled());
        bool returned = false, checked = false;
        QTimer release;
        connect(&release, &QTimer::timeout, &window, [&] {
            if (!backend.Accepted)
                return;
            release.stop();
            checked = true;
            QCOMPARE(returned, !wait);
            QVERIFY(backend.Results.isEmpty());
            QCOMPARE(SourceExtractFixture::readFile(source), bytes);
            // Waiting protects the source stack from teardown; ordinary drops
            // can finish after the manager closes.
            QCOMPARE(window.close(), !wait);
            QVERIFY(SourceExtractFixture::writeFile(backend.Barrier, "continue"));
        });
        release.start(10);
        UStringVector paths;
        paths.Add(ToUString(source));
        UInt32 flags = 0;
        const HRESULT result = table->sourcePanel->CompressDropFiles(
            paths, ToUString(output.path() + '/'), true, false,
            wait ? k_SourceFlags_TempFiles : k_SourceFlags_DoNotWaitFinish, flags);
        returned = true;
        if (wait) {
            QVERIFY(checked);
            QCOMPARE(result, cancel ? E_ABORT : failure ? E_FAIL : S_OK);
        } else {
            QCOMPARE(result, S_OK);
        }
        QTRY_COMPARE_WITH_TIMEOUT(backend.Results.size(), 1, 30000);
        QVERIFY(checked);
        QCOMPARE(backend.Results.first().Status, cancel    ? QtOperationResult::Outcome::Cancelled
                                                 : failure ? QtOperationResult::Outcome::Failure
                                                           : QtOperationResult::Outcome::Success);
        QCOMPARE(SourceExtractFixture::readFile(source), bytes);
        if (!cancel && !failure) {
            QVERIFY(!backend.Results.first().ChangedArchives.isEmpty());
            const QString archive = backend.Results.first().ChangedArchives.first();
            // The drag source may release its temporary bytes immediately
            // after completion. The independent output must retain them.
            QVERIFY(QFile::remove(source));
            ArchiveBackend verifier;
            QSignalSpy finished(&verifier, &ArchiveBackend::finished);
            verifier.list(archive);
            QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 30000);
            QVERIFY(finished.takeFirst().first().toBool());
            verifier.extract(archive, output.filePath("verify"));
            QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 30000);
            QVERIFY(finished.first().first().toBool());
            QCOMPARE(SourceExtractFixture::readFile(
                         QDir(output.filePath("verify")).filePath(QFileInfo(source).fileName())),
                     bytes);
        } else {
            QVERIFY(backend.Results.first().ChangedArchives.isEmpty());
        }
#endif
    }
};
QTEST_MAIN(DragDropAddUiTests)
#include "drag_drop_add_ui_tests.moc"
