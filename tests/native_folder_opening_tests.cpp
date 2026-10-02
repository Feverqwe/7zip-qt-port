#include "7zip/Common/StreamObjects.h"
#include "CPP/7zip/UI/Agent/Agent.h"
#include "CPP/7zip/UI/FileManager/FileFolderPluginOpen.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include "qtarchivefileresource.h"
#include "qtarchivefoldermanager.h"
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>
#include <future>

namespace {
class CObservedBackend final : public ArchiveBackend {
  public:
    unsigned Starts = 0;
    using ArchiveBackend::ArchiveBackend;
    void openFolder(const QString &path, const QString &type, const QString &password) override {
        ++Starts;
        ArchiveBackend::openFolder(path, type, password);
    }
};
class COpenProgress final : public IProgress, public ICryptoGetTextPassword, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IProgress, ICryptoGetTextPassword)

  public:
    bool Cancel = false;
    unsigned PasswordCalls = 0;
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return Cancel ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return Cancel ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(CryptoGetTextPassword(BSTR *password)) override {
        ++PasswordCalls;
        return StringToBstr(UString(L"local fixture password"), password);
    }
};
class CExtractToMemory final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)

  public:
    CMyComPtr2<ISequentialOutStream, CDynBufSeqOutStream> Stream;
    CExtractToMemory() { Stream.Create_if_Empty(); }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32)) override {
        *stream = Stream.Interface();
        (*stream)->AddRef();
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 result)) override {
        return result == NArchive::NExtract::NOperationResult::kOK ? S_OK : E_FAIL;
    }
};
} // namespace

class NativeFolderOpeningTests final : public QObject {
    Q_OBJECT
    QString engine_, module_;
    quint32 version_ = 0;
    bool createFixture(const QString &path, const QString &input, bool encrypted = false) {
        QProcess creator;
        QStringList arguments{"a", encrypted ? "-t7z" : "-tzip"};
        if (encrypted)
            arguments << "-plocal fixture password" << "-mhe=on";
        arguments << "--" << path << input;
        creator.start(engine_, arguments);
        return creator.waitForFinished(10000) && creator.exitCode() == 0;
    }

    void extractToMemory(IInArchive *archive, const QByteArray &expected) {
        auto *callback = new CExtractToMemory;
        CMyComPtr<IArchiveExtractCallback> owner = callback;
        auto result = std::async(std::launch::async,
                                 [&] { return archive->Extract(nullptr, UInt32(-1), 0, owner); });
        QTRY_VERIFY_WITH_TIMEOUT(
            result.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready, 10000);
        QCOMPARE(result.get(), S_OK);
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(callback->Stream->GetBuffer()),
                            qsizetype(callback->Stream->GetSize())),
                 expected);
    }

  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; native application opening unverified");
#else
        engine_ = QStandardPaths::findExecutable("7zz");
        if (engine_.isEmpty())
            QSKIP("Official engine unavailable; native application opening unverified");
        module_ = QString::fromUtf8(SEVENZIP_CODEC_MODULE);
        ArchiveBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        version_ = backend.codecCatalog()->version();
#endif
    }
    void sourceFolderRetainsNativePropertiesAndRefusesWrongType() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QFile input(fixture.filePath(QStringLiteral("имя 空 with spaces.bin")));
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("native folder"), qint64(13));
        input.close();
        const QString path = fixture.filePath("archive.unknown");
        QVERIFY(createFixture(path, input.fileName()));
        CObservedBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        std::weak_ptr<const ArchiveCodecCatalog> old = backend.codecCatalog();
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CFfpOpen opening(manager);
        QCOMPARE(opening.OpenFileFolderPlugin(nullptr, us2fs(path.toStdWString().c_str()),
                                              UString(L"*"), nullptr),
                 S_OK);
        QCOMPARE(backend.Starts, 1u);
        auto *folder = dynamic_cast<CAgentFolder *>(opening.Folder.Interface());
        QVERIFY(folder);
        IInArchive *archive = folder->GetArchiveProperties();
        QVERIFY(archive);
        UInt32 count = 0;
        QCOMPARE(archive->GetNumberOfArchiveProperties(&count), S_OK);
        QVERIFY(count > 0);
        for (UInt32 i = 0; i < count; ++i) {
            CMyComBSTR name;
            PROPID id = 0;
            VARTYPE type = VT_EMPTY;
            QCOMPARE(archive->GetArchivePropertyInfo(i, &name, &id, &type), S_OK);
            QVERIFY(id < 0x10000); // Native property IDs, not CLI string-key projection.
        }
        QCOMPARE(backend.loadCodecCatalog(fixture.filePath("missing module"), version_), E_FAIL);
        QVERIFY(!old.expired());
        NWindows::NCOM::CPropVariant size;
        QCOMPARE(archive->GetArchiveProperty(kpidPhySize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(VT_UI8));
        QCOMPARE(size.uhVal.QuadPart, UInt64(QFileInfo(path).size()));
        QCOMPARE(archive->Close(), S_OK);
        opening.Folder.Release();
        QVERIFY(old.expired());
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        CFfpOpen wrong(manager);
        QCOMPARE(wrong.OpenFileFolderPlugin(nullptr, us2fs(path.toStdWString().c_str()),
                                            UString(L"7z"), nullptr),
                 S_FALSE);
        QVERIFY(!wrong.Folder);
        QCOMPARE(backend.Starts, 1u);
    }
    void workerCancelAndNativePassword_data() {
        QTest::addColumn<bool>("cancel");
        QTest::newRow("native cancel before process") << true;
        QTest::newRow("native encrypted headers password") << false;
    }
    void compressedTarRetainsSourceGzipLevel() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QFile input(fixture.filePath(QStringLiteral("вложенный 空.bin")));
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("nested tar bytes"), qint64(16));
        input.close();
        const QString tar = fixture.filePath("inner.tar");
        const QString path = fixture.filePath("outer.tgz");
        for (const auto &command : {QStringList{"a", "-ttar", "--", tar, input.fileName()},
                                    QStringList{"a", "-tgzip", "--", path, tar}}) {
            QProcess creator;
            creator.start(engine_, command);
            QVERIFY(creator.waitForFinished(10000));
            QCOMPARE(creator.exitCode(), 0);
        }
        CObservedBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CFfpOpen opening(manager);
        QCOMPARE(opening.OpenFileFolderPlugin(nullptr, us2fs(path.toStdWString().c_str()),
                                              UString(), nullptr),
                 S_OK);
        auto *folder = dynamic_cast<CAgentFolder *>(opening.Folder.Interface());
        QVERIFY(folder);
        auto *resource = dynamic_cast<QtArchiveFileResource *>(folder->GetEngineResource());
        QVERIFY(resource);
        // Upstream only recurses when IInArchiveGetStream supplies IInStream.
        // This gzip module exposes its TAR item, not a seekable decoded stream.
        QCOMPARE(resource->Info.format, QString("gzip"));
        QVERIFY(!resource->RequiresOuterOpen());
        QVERIFY(resource->HasUpdateService());
        NWindows::NCOM::CPropVariant readOnly;
        QCOMPARE(folder->GetFolderProperty(kpidReadOnly, &readOnly), S_OK);
        QCOMPARE(readOnly.vt, VARTYPE(VT_BOOL));
        QCOMPARE(readOnly.boolVal, VARIANT_FALSE);
        QFile tarBytes(tar);
        QVERIFY(tarBytes.open(QIODevice::ReadOnly));
        extractToMemory(folder->GetArchiveProperties(), tarBytes.readAll());
        QCOMPARE(folder->GetArchiveProperties()->Close(), S_OK);
        QVERIFY(!resource->RequiresOuterOpen());
        QVERIFY(resource->HasUpdateService());
    }
    void nativeSplitChainKeepsReadOnlyStateAfterClose() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QFile input(fixture.filePath(QStringLiteral("тома 空.bin")));
        QVERIFY(input.open(QIODevice::WriteOnly));
        const QByteArray payload(4096, 'v');
        QCOMPARE(input.write(payload), qint64(payload.size()));
        input.close();
        const QString base = fixture.filePath("volumes.7z");
        const QString path = base + ".001";
        QProcess creator;
        creator.start(engine_, {"a", "-t7z", "-mx0", "-v1024b", "--", base, input.fileName()});
        QVERIFY(creator.waitForFinished(10000));
        QCOMPARE(creator.exitCode(), 0);
        QVERIFY(QFileInfo::exists(base + ".003"));
        CObservedBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CFfpOpen opening(manager);
        QCOMPARE(opening.OpenFileFolderPlugin(nullptr, us2fs(path.toStdWString().c_str()),
                                              UString(), nullptr),
                 S_OK);
        auto *folder = dynamic_cast<CAgentFolder *>(opening.Folder.Interface());
        QVERIFY(folder);
        auto *resource = dynamic_cast<QtArchiveFileResource *>(folder->GetEngineResource());
        QVERIFY(resource);
        QCOMPARE(resource->Info.format, QString("7z"));
        QVERIFY(resource->RequiresOuterOpen());
        QVERIFY(!resource->HasUpdateService());
        NWindows::NCOM::CPropVariant readOnly;
        QCOMPARE(folder->GetFolderProperty(kpidReadOnly, &readOnly), S_OK);
        QCOMPARE(readOnly.vt, VARTYPE(VT_BOOL));
        QCOMPARE(readOnly.boolVal, VARIANT_TRUE);
        extractToMemory(folder->GetArchiveProperties(), payload);
        QCOMPARE(folder->GetArchiveProperties()->Close(), S_OK);
        QVERIFY(resource->RequiresOuterOpen());
        QVERIFY(!resource->HasUpdateService());
    }
    void workerCancelAndNativePassword() {
        QFETCH(bool, cancel);
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QFile input(fixture.filePath(QStringLiteral("пароль 空.bin")));
        QVERIFY(input.open(QIODevice::WriteOnly));
        QCOMPARE(input.write("protected fixture"), qint64(17));
        input.close();
        const QString path = fixture.filePath("encrypted.unknown");
        QVERIFY(createFixture(path, input.fileName(), true));
        QFile bytes(path);
        QVERIFY(bytes.open(QIODevice::ReadOnly));
        const QByteArray original = bytes.readAll();
        bytes.close();
        CObservedBackend backend(nullptr, engine_);
        QCOMPARE(backend.loadCodecCatalog(module_, version_), S_OK);
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        auto *progress = new COpenProgress;
        CMyComPtr<IProgress> progressOwner = progress;
        progress->Cancel = cancel;
        CMyComPtr<IFolderFolder> folder;
        auto result = std::async(std::launch::async, [&] {
            return manager->OpenFolderFile(nullptr, path.toStdWString().c_str(), L"7z", &folder,
                                           progressOwner);
        });
        QTRY_VERIFY_WITH_TIMEOUT(
            result.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready, 10000);
        QCOMPARE(result.get(), cancel ? E_ABORT : S_OK);
        QCOMPARE(backend.Starts, cancel ? 0u : 1u);
        QCOMPARE(bool(folder), !cancel);
        QCOMPARE(progress->PasswordCalls, cancel ? 0u : 1u);
        QVERIFY(bytes.open(QIODevice::ReadOnly));
        QCOMPARE(bytes.readAll(), original);
    }
};
QTEST_MAIN(NativeFolderOpeningTests)
#include "native_folder_opening_tests.moc"
