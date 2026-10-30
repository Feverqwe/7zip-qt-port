#include "archive/backend/archivebackend.h"
#include "archive/codecs/archivecodeccatalog.h"
#include "archive/support/source_extract_fixture.h"
#include "gui/UpdateCallbackGUI.h"
#include <QTemporaryDir>
#include <QtTest>

class SourceShellUpdateTests final : public QObject {
    Q_OBJECT
  private slots:
    void sourceWriter_data() {
        QTest::addColumn<QString>("type");
        QTest::addColumn<bool>("cancel");
        QTest::newRow("hash") << QString("hash") << false;
        QTest::newRow("7z") << QString("7z") << false;
        QTest::newRow("hash-cancel") << QString("hash") << true;
        QTest::newRow("7z-cancel") << QString("7z") << true;
    }
    void sourceWriter() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official native runtime unavailable");
#else
        QFETCH(QString, type);
        QFETCH(bool, cancel);
        QTemporaryDir fixture;
        const QString name = QStringLiteral("данные 空 with spaces.bin");
        const QString input = fixture.filePath(name);
        const QString output = fixture.filePath(type == "hash" ? "result.sha256" : "result.7z");
        QVERIFY(SourceExtractFixture::writeFile(input, "abc"));
        ArchiveBackend backend;
        const QString module = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveCodecCatalog probe(module, 0);
        QCOMPARE(probe.result(), S_OK);
        QCOMPARE(backend.loadCodecCatalog(module, probe.version()), S_OK);
        CProgressDialog progress;
        CUpdateCallbackGUI callback;
        callback.ProgressDialog = &progress;
        callback.Init();
        progress.Sync.Set_Stopped(cancel);
        CUpdateOptions options;
        options.ArcNameMode = k_ArcNameMode_Exact;
        options.SetActionCommand_Add();
        NWildcard::CCensor censor;
        censor.AddPreItem_NoWildcard(UString(input.toStdWString().c_str()));
        CUpdateErrorInfo error;
        const HRESULT result = backend.updateSourceArchive(output, type, censor, options, error,
                                                           &callback, &callback, true);
        QCOMPARE(result, cancel ? E_ABORT : S_OK);
        QCOMPARE(SourceExtractFixture::readFile(input), QByteArray("abc"));
        QVERIFY(!backend.busy());
        if (cancel) {
            QVERIFY(!QFileInfo::exists(output));
            return;
        }
        QVERIFY(QFileInfo::exists(output));
        QProcess verify;
        verify.setWorkingDirectory(fixture.path());
        verify.start(QStandardPaths::findExecutable("7z"), {"t", "--", output});
        QVERIFY(verify.waitForFinished(10000));
        QCOMPARE(verify.exitCode(), 0);
        if (type == "hash") {
            const QByteArray bytes = SourceExtractFixture::readFile(output);
            QVERIFY(
                bytes.contains("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
            QVERIFY(bytes.contains(name.toUtf8()));
        }
#endif
    }
    void existingInvalidArchiveIsPreserved() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official native runtime unavailable");
#else
        QTemporaryDir fixture;
        const QString input = fixture.filePath(QStringLiteral("данные 空 with spaces.bin"));
        const QString output = fixture.filePath("occupied.7z");
        QVERIFY(SourceExtractFixture::writeFile(input, "abc"));
        QVERIFY(SourceExtractFixture::writeFile(output, "broken archive sentinel"));
        ArchiveBackend backend;
        const QString module = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveCodecCatalog probe(module, 0);
        QCOMPARE(probe.result(), S_OK);
        QCOMPARE(backend.loadCodecCatalog(module, probe.version()), S_OK);
        CProgressDialog progress;
        CUpdateCallbackGUI callback;
        callback.ProgressDialog = &progress;
        callback.Init();
        CUpdateOptions options;
        options.ArcNameMode = k_ArcNameMode_Exact;
        options.SetActionCommand_Add();
        NWildcard::CCensor censor;
        censor.AddPreItem_NoWildcard(UString(input.toStdWString().c_str()));
        CUpdateErrorInfo error;
        QCOMPARE(backend.updateSourceArchive(output, "7z", censor, options, error, &callback,
                                             &callback, true),
                 S_FALSE);
        QCOMPARE(SourceExtractFixture::readFile(output), QByteArray("broken archive sentinel"));
        QCOMPARE(SourceExtractFixture::readFile(input), QByteArray("abc"));
        QCOMPARE(QDir(fixture.path()).entryList({"*.tmp*"}, QDir::Files).size(), 0);
        QVERIFY(!backend.busy());
#endif
    }
};
QTEST_MAIN(SourceShellUpdateTests)
#include "source_shell_update_tests.moc"
