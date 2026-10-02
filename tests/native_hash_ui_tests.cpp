#include "CPP/7zip/UI/FileManager/ProgressDialog2.h"
#include "archivecodeccatalog.h"
#include "window_state_support.h"

class NativeHashUiTests final : public QObject, protected MainWindowTestSupport {
    Q_OBJECT
    QString engine_, module_;
    quint32 version_ = 0;
  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; native hash-file UI remains unverified");
#else
        engine_ = QStandardPaths::findExecutable("7zz");
        QVERIFY(!engine_.isEmpty());
        module_ = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY(ready[0][0].toBool());
        version_ = backend.codecCatalog()->version();
#endif
    }
    void hashFileTestUsesSourceReader_data() {
        QTest::addColumn<QString>("outcome");
        for (const char *outcome : {"success", "crc", "missing", "unsafe"})
            QTest::newRow(outcome) << QString::fromLatin1(outcome);
    }
    void hashFileTestUsesSourceReader() {
        QFETCH(QString, outcome);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString name = QStringLiteral("имя с пробелом.txt");
        QFile input(temporary.filePath(name));
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("123456789"), qint64(9));
        input.close();
        const QByteArray digest =
            QCryptographicHash::hash(outcome == "crc" ? QByteArray("wrong")
                                                      : QByteArray("123456789"),
                                     QCryptographicHash::Sha256)
                .toHex();
        const QString entry = outcome == "unsafe"    ? "../outside.txt"
                              : outcome == "missing" ? "missing.txt"
                                                     : name;
        const QString manifest = temporary.filePath("checksums.sha256");
        QFile hashes(manifest);
        QVERIFY(hashes.open(QIODevice::WriteOnly));
        const QByteArray data = digest + "  " + entry.toUtf8() + "\n";
        QCOMPARE(hashes.write(data), qint64(data.size()));
        hashes.close();
        ArchiveBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        MainWindow window(&backend);
        window.show();
        window.openArchive(manifest);
        QTRY_COMPARE(table(window)->rowCount(), 1);
        table(window)->selectAll();
        QSignalSpy engines(&backend, &ArchiveBackend::engineStarted);
        QStringList errors;
        QTimer responder;
        connect(&responder, &QTimer::timeout, this, [&] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                if (box->icon() == QMessageBox::Warning || box->icon() == QMessageBox::Critical)
                    errors.append(box->text());
                box->accept();
            } else if (auto *dialog =
                           dynamic_cast<CProgressDialog *>(QApplication::activeModalWidget())) {
                auto *pause = dialog->findChild<QPushButton *>("IDB_PAUSE");
                if (pause && !pause->isVisible()) {
                    FOR_VECTOR(i, dialog->Sync.Messages)
                    errors.append(ToQString(dialog->Sync.Messages[i]));
                    const UString &final = dialog->Sync.FinalMessage.ErrorMessage.Message;
                    if (!final.IsEmpty())
                        errors.append(ToQString(final));
                    dialog->findChild<QPushButton *>("IDCANCEL")->click();
                }
            }
        });
        responder.start(1);
        window.testArchive();
        responder.stop();
        QCOMPARE(engines.size(), 0); // Native Hash handler and original Test callbacks.
        QCOMPARE(errors.isEmpty(), outcome == "success");
        QVERIFY(input.open(QIODevice::ReadOnly));
        QCOMPARE(input.readAll(), QByteArray("123456789"));
        QVERIFY(hashes.open(QIODevice::ReadOnly));
        QCOMPARE(hashes.readAll(), data);
    }
};
QTEST_MAIN(NativeHashUiTests)
#include "native_hash_ui_tests.moc"
