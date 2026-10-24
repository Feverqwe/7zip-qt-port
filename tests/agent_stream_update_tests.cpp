#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/FileManager/FileFolderPluginOpen.h"
#include "qtarchivefileresource.h"
#include "qtarchivefoldermanager.h"
#include "source_extract_fixture.h"
#include "source_metadata_update_fixture.h"
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

using namespace SourceExtractFixture;

namespace {
class CStreamUpdateWorker final : public CProgressThreadVirt {
    CMyComPtr<IFolderOperations> operations_;
    QString prefix_, name_;
    HRESULT ProcessVirt() override {
        CMyComPtr2_Create<IFolderArchiveUpdateCallback, CUpdateCallback100Imp> ui;
        ui->ProgressDialog = this;
        ui->Init();
        CMyComPtr<IFolderArchiveUpdateCallback> callback = ui.Interface();
        if (Failure != S_OK)
            callback = new CLinkUpdateFault(ui.Interface(), Failure, Observed);
        const auto prefix = prefix_.toStdWString();
        const auto name = name_.toStdWString();
        const wchar_t *names[] = {name.c_str()};
        return operations_->CopyFrom(0, prefix.c_str(), names, 1, callback);
    }

  public:
    HRESULT Failure = S_OK;
    bool Observed = false;
    CStreamUpdateWorker(IFolderOperations *operations, const QString &prefix, const QString &name)
        : operations_(operations), prefix_(prefix), name_(name) {}
};
} // namespace

class AgentStreamUpdateTests final : public QObject {
    Q_OBJECT
  private slots:
    void sourceStreamCopy_data() {
        QTest::addColumn<bool>("filesystemStream");
        QTest::addColumn<HRESULT>("failure");
        QTest::addColumn<bool>("directory");
        for (bool filesystem : {false, true})
            for (HRESULT failure : {HRESULT(S_OK), HRESULT(E_FAIL), HRESULT(E_ABORT)})
                QTest::newRow(qPrintable(QString("%1-%2").arg(filesystem).arg(quint32(failure))))
                    << filesystem << failure << false;
        QTest::newRow("archive-stream-rejects-directory") << false << HRESULT(E_NOTIMPL) << true;
    }
    void sourceStreamCopy() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; source stream updates unverified");
#else
        QFETCH(bool, filesystemStream);
        QFETCH(HRESULT, failure);
        QFETCH(bool, directory);
#ifndef Q_OS_WIN
        if (filesystemStream)
            QSKIP("Filesystem ADS enumeration requires Windows");
#endif
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString engine = QStandardPaths::findExecutable("7z");
        QVERIFY(!engine.isEmpty());
        const QString owner = QStringLiteral("файл δ with space.bin");
        const QString existing = QStringLiteral("поток δ");
        const QString name = QStringLiteral("новый поток 空");
        const QString archive = fixture.filePath("archive streams.wim");
        // Real handler fixture with one ordinary file and one archived ADS.
        QVERIFY(QFile::copy(QString::fromUtf8(ALT_STREAMS_FIXTURE), archive));
        const QByteArray original = readFile(archive);
        const QByteArray payload(1024 * 1024, 'x');
        QString prefix = fixture.filePath("source");
        QVERIFY(QDir().mkpath(prefix));
        if (filesystemStream) {
            const QString base = QDir(prefix).filePath(QStringLiteral("носитель 空.bin"));
            QVERIFY(writeFile(base, "source main bytes"));
            prefix = base + ':';
        } else
            prefix += '/';
        const QString input = prefix + name;
        if (directory)
            QVERIFY(QDir().mkpath(input));
        else
            QVERIFY(writeFile(input, payload));

        ArchiveBackend backend(nullptr, engine);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(QString::fromUtf8(SEVENZIP_CODEC_MODULE)));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY(ready[0][0].toBool());
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CFfpOpen opening(manager);
        QCOMPARE(opening.OpenFileFolderPlugin(nullptr, us2fs(archive.toStdWString().c_str()),
                                              UString(L"wim"), nullptr),
                 S_OK);
        CMyComPtr<IFolderFolder> target = opening.Folder;
        QCOMPARE(target->LoadItems(), S_OK);
        {
            CMyComPtr<IFolderAltStreams> streams;
            QCOMPARE(target.QueryInterface(IID_IFolderAltStreams, &streams), S_OK);
            CMyComPtr<IFolderFolder> streamFolder;
            QCOMPARE(streams->BindToAltStreams(owner.toStdWString().c_str(), &streamFolder), S_OK);
            QVERIFY(streamFolder);
            target = streamFolder;
            QCOMPARE(target->LoadItems(), S_OK);
        }
        auto *folder = dynamic_cast<CAgentFolder *>(target.Interface());
        QVERIFY(folder);
        auto *resource = dynamic_cast<QtArchiveFileResource *>(folder->GetEngineResource());
        QVERIFY(resource && resource->NativeItemProperties() && resource->HasUpdateService());
        CMyComPtr<IFolderOperations> operations;
        QCOMPARE(target.QueryInterface(IID_IFolderOperations, &operations), S_OK);
        CStreamUpdateWorker worker(operations, prefix, name);
        if (failure != E_NOTIMPL)
            worker.Failure = failure;
        worker.DisableUserQuestions = true;
        auto transport = resource->CreateProgressTransport(worker);
        QVERIFY(transport);
        QCOMPARE(worker.Create(L"Source stream update", nullptr), S_OK);
        QCOMPARE(worker.Result, failure);
        if (failure != S_OK) {
            if (!directory)
                QVERIFY(worker.Observed);
            QCOMPARE(readFile(archive), original);
        } else {
            QVERIFY(readFile(archive) != original);
            QCOMPARE(target->LoadItems(), S_OK);
            UInt32 count = 0;
            QCOMPARE(target->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(2));
        }
        if (!directory)
            QCOMPARE(readFile(input), payload);
        else
            QVERIFY(QFileInfo(input).isDir());
        if (filesystemStream)
            QCOMPARE(readFile(prefix.chopped(1)), QByteArray("source main bytes"));
        // Independent official extraction checks existing main/ADS bytes and
        // the exact destination namespace of the newly written payload.
        const QString output = fixture.filePath("output");
        QProcess extractor;
        extractor.start(engine, {"x", "-y", "-sns", "-o" + output, "--", archive});
        QVERIFY(extractor.waitForFinished(10000));
        QCOMPARE(extractor.exitCode(), 0);
        QCOMPARE(readFile(QDir(output).filePath(owner)), QByteArray("main bytes"));
        QCOMPARE(readFile(QDir(output).filePath(owner + ':' + existing)),
                 QByteArray("archive stream bytes"));
        if (failure == S_OK) {
            const QString extracted = QDir(output).filePath(owner + ':' + name);
            QCOMPARE(readFile(extracted), payload);
        }
        QVERIFY(QDir(fixture.path())
                    .entryList({"*.tmp", ".sevenzip-*"}, QDir::Files | QDir::Hidden)
                    .isEmpty());
#endif
    }
};
QTEST_MAIN(AgentStreamUpdateTests)
#include "agent_stream_update_tests.moc"
