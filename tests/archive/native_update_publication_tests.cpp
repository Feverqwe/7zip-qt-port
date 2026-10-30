#include "archive/codecs/archivecodeccatalog.h"
#include "archive/operations/Update.h"
#include "archive/resources/archivenativeopen.h"
#include "archive/sfx/archivesfxmodule.h"
#include "archive/support/archive_backend_test_support.h"
#include "filemanager/OpenCallback.h"
#include <QStorageInfo>

namespace {
// Exercise the official updater at its public publication callback, without
// timing a progress dialog against a small archive's compression worker.
class CPublicationCallback final : public IUpdateCallbackUI2 {
  public:
    HRESULT PublicationResult = S_OK;
    HRESULT CopyResult = S_OK;
    bool PublicationReached = false;
    std::function<HRESULT()> BeforeDeletion;
    std::function<HRESULT()> BeforePublication;
    HRESULT WriteSfx(const wchar_t *, UInt64) override { return S_OK; }
    HRESULT SetTotal(UInt64) override { return S_OK; }
    HRESULT SetCompleted(const UInt64 *) override { return S_OK; }
    HRESULT SetRatioInfo(const UInt64 *, const UInt64 *) override { return S_OK; }
    HRESULT CheckBreak() override { return S_OK; }
    HRESULT SetNumItems(const CArcToDoStat &) override { return S_OK; }
    HRESULT GetStream(const wchar_t *, bool, bool, UInt32) override { return S_OK; }
    HRESULT OpenFileError(const FString &, DWORD error) override {
        return HRESULT_FROM_WIN32(error);
    }
    HRESULT ReadingFileError(const FString &, DWORD error) override {
        return HRESULT_FROM_WIN32(error);
    }
    HRESULT SetOperationResult(Int32 result) override { return result == 0 ? S_OK : E_FAIL; }
    HRESULT ReportExtractResult(Int32 result, Int32, const wchar_t *) override {
        return result == 0 ? S_OK : E_FAIL;
    }
    HRESULT ReportUpdateOperation(UInt32, const wchar_t *, bool) override { return S_OK; }
    HRESULT CryptoGetTextPassword2(Int32 *defined, BSTR *password) override {
        *defined = 0;
        *password = nullptr;
        return S_OK;
    }
    HRESULT CryptoGetTextPassword(BSTR *password) override {
        *password = nullptr;
        return E_ABORT;
    }
    HRESULT ShowDeleteFile(const wchar_t *, bool) override { return S_OK; }
    HRESULT ScanError(const FString &, DWORD error) override { return HRESULT_FROM_WIN32(error); }
    HRESULT ScanProgress(const CDirItemsStat &, const FString &, bool) override { return S_OK; }
    HRESULT OpenResult(const CCodecs *, const CArchiveLink &, const wchar_t *,
                       HRESULT result) override {
        return result;
    }
    HRESULT StartScanning() override { return S_OK; }
    HRESULT FinishScanning(const CDirItemsStat &) override { return S_OK; }
    HRESULT StartOpenArchive(const wchar_t *) override { return S_OK; }
    HRESULT StartArchive(const wchar_t *, bool) override { return S_OK; }
    HRESULT FinishArchive(const CFinishArchiveStat &) override { return S_OK; }
    HRESULT DeletingAfterArchiving(const FString &, bool) override {
        return BeforeDeletion ? BeforeDeletion() : S_OK;
    }
    HRESULT FinishDeletingAfterArchiving() override { return S_OK; }
    HRESULT MoveArc_Start(const wchar_t *, const wchar_t *, UInt64, Int32) override {
        PublicationReached = true;
        return BeforePublication ? BeforePublication() : PublicationResult;
    }
    HRESULT MoveArc_Progress(UInt64, UInt64 current) override {
        return current ? CopyResult : S_OK;
    }
    HRESULT MoveArc_Finish() override { return S_OK; }
};
QByteArray readFile(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
} // namespace

class NativeUpdatePublicationTests final : public QObject, protected ArchiveBackendTestSupport {
    Q_OBJECT
    QString module_;
    quint32 version_ = 0;

  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; native publication unverified");
#else
        module_ = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveBackend backend(nullptr, "unused-console-engine");
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 10000);
        QVERIFY(ready[0][0].toBool());
        version_ = backend.codecCatalog()->version();
#endif
    }
    void publicationPreservesOriginalUntilCommit_data() {
        QTest::addColumn<HRESULT>("failure");
        QTest::addColumn<bool>("crossVolume");
        QTest::addColumn<bool>("duringCopy");
        QTest::newRow("cancel") << HRESULT(E_ABORT) << false << false;
        QTest::newRow("failure") << HRESULT(E_FAIL) << false << false;
        QTest::newRow("success") << HRESULT(S_OK) << false << false;
        QTest::newRow("cross-volume cancel") << HRESULT(E_ABORT) << true << false;
        QTest::newRow("cross-volume copy cancel") << HRESULT(E_ABORT) << true << true;
        QTest::newRow("cross-volume success") << HRESULT(S_OK) << true << false;
    }
    void publicationPreservesOriginalUntilCommit() {
        QFETCH(HRESULT, failure);
        QFETCH(bool, crossVolume);
        QFETCH(bool, duringCopy);
        if (crossVolume &&
            (!QFileInfo("/dev/shm").isDir() ||
             QStorageInfo("/dev/shm").device() == QStorageInfo(QDir::tempPath()).device()))
            QSKIP("No separate writable /dev/shm filesystem for cross-volume publication");
        QTemporaryDir fixture(QDir::tempPath() + "/native publication 空-XXXXXX");
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("payload 空.txt");
        const QString archive = fixture.filePath("original 空.zip");
        writeFile(source, "original bytes");
        ArchiveBackend backend(nullptr, "unused-console-engine");
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        NWildcard::CCensor censor;
        NWildcard::CCensorPathProps props;
        props.Recursive = false;
        props.WildcardMatching = false;
        censor.AddItem(NWildcard::k_RelatPath, true, source.toStdWString().c_str(), props);
        CUpdateOptions options;
        options.SetActionCommand_Add();
        CUpdateErrorInfo error;
        COpenArchiveCallback opener;
        CPublicationCallback callback;
        QCOMPARE(backend.updateSourceArchive(archive, "zip", censor, options, error, &opener,
                                             &callback, true),
                 S_OK);
        const QByteArray before = readFile(archive);
        QVERIFY(!before.isEmpty());
        writeFile(source, "changed and longer replacement bytes");
        options.DeleteAfterCompressing = failure != S_OK;
        // A real second filesystem exercises the publication copy, not a mock
        // that assumes rename and cross-device copying have identical behavior.
        QTemporaryDir working((crossVolume ? QString("/dev/shm") : fixture.path()) +
                              "/publication-work-XXXXXX");
        QVERIFY(working.isValid());
        if (crossVolume)
            options.WorkingDir = us2fs((working.path() + '/').toStdWString().c_str());
        if (duringCopy)
            callback.CopyResult = failure;
        else
            callback.PublicationResult = failure;
        QCOMPARE(backend.updateSourceArchive(archive, "zip", censor, options, error, &opener,
                                             &callback, true),
                 failure);
        QVERIFY(callback.PublicationReached);
        if (failure == S_OK) {
            QVERIFY(readFile(archive) != before);
            std::shared_ptr<ArchiveNativeOpen> reader;
            QCOMPARE(backend.openNativeArchive(archive, "zip", nullptr, reader), S_OK);
            QVERIFY(reader);
            NWindows::NCOM::CPropVariant size;
            QCOMPARE(reader->arc().Archive->GetProperty(0, kpidSize, &size), S_OK);
            QCOMPARE(size.vt, VARTYPE(VT_UI8));
            QCOMPARE(size.uhVal.QuadPart,
                     UInt64(QByteArray("changed and longer replacement bytes").size()));
        } else
            QCOMPARE(readFile(archive), before);
        QCOMPARE(readFile(source), QByteArray("changed and longer replacement bytes"));
    }
    void newArchiveAppearsOnlyAtCommit_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("success") << 0;
        QTest::newRow("cancel") << 1;
        QTest::newRow("racing destination") << 2;
    }
    void newArchiveAppearsOnlyAtCommit() {
        QFETCH(int, mode);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("source.txt");
        const QString archive = fixture.filePath("new archive.zip");
        writeFile(source, "retained input");
        ArchiveBackend backend(nullptr, "unused-console-engine");
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        NWildcard::CCensor censor;
        NWildcard::CCensorPathProps props;
        props.WildcardMatching = false;
        censor.AddItem(NWildcard::k_RelatPath, true, source.toStdWString().c_str(), props);
        CUpdateOptions options;
        options.SetActionCommand_Add();
        options.DeleteAfterCompressing = true;
        CUpdateErrorInfo error;
        COpenArchiveCallback opener;
        CPublicationCallback callback;
        bool absentBeforeCommit = false;
        callback.BeforePublication = [&] {
            absentBeforeCommit = !QFileInfo::exists(archive);
            if (mode == 2)
                writeFile(archive, "other writer's result");
            return mode == 1 ? E_ABORT : S_OK;
        };
        const HRESULT result = backend.updateSourceArchive(archive, "zip", censor, options, error,
                                                           &opener, &callback, true);
        QVERIFY(callback.PublicationReached);
        QVERIFY(absentBeforeCommit);
        if (mode == 0) {
            QCOMPARE(result, S_OK);
            QVERIFY(QFileInfo::exists(archive));
            QVERIFY(!QFileInfo::exists(source));
        } else {
            QVERIFY(result != S_OK);
            QCOMPARE(readFile(source), QByteArray("retained input"));
            if (mode == 1) {
                QCOMPARE(result, E_ABORT);
                QVERIFY(!QFileInfo::exists(archive));
            } else
                QCOMPARE(readFile(archive), QByteArray("other writer's result"));
        }
    }
    void sfxIsExecutableBeforeDeletingInputs() {
        const ArchiveSfxModule module = ArchiveSfxModule::Discover(version_);
        QVERIFY2(module.available(), qPrintable(module.error()));
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString source = fixture.filePath("source 空.txt");
        writeFile(source, "payload");
        ArchiveBackend backend(nullptr, "unused-console-engine");
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        NWildcard::CCensor censor;
        NWildcard::CCensorPathProps props;
        props.WildcardMatching = false;
        censor.AddItem(NWildcard::k_RelatPath, true, source.toStdWString().c_str(), props);
        CUpdateOptions options;
        options.SetActionCommand_Add();
        options.SfxMode = true;
        options.SfxModule = us2fs(module.path().toStdWString().c_str());
        options.DeleteAfterCompressing = true;
        CUpdateErrorInfo error;
        COpenArchiveCallback opener;
        CPublicationCallback callback;
        bool deleting = false;
        callback.BeforeDeletion = [&] {
            deleting = true;
            const QString output = QString::fromWCharArray(options.ArchivePath.GetFinalPath());
            return QFileInfo(output).isExecutable() ? S_OK : E_FAIL;
        };
        QCOMPARE(backend.updateSourceArchive(fixture.filePath("sfx.exe"), "7z", censor, options,
                                             error, &opener, &callback, true),
                 S_OK);
        QVERIFY(deleting);
        QVERIFY(!QFileInfo::exists(source));
        QVERIFY(
            QFileInfo(QString::fromWCharArray(options.ArchivePath.GetFinalPath())).isExecutable());
    }
};
QTEST_MAIN(NativeUpdatePublicationTests)
#include "native_update_publication_tests.moc"
