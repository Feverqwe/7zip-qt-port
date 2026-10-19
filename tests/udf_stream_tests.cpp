#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

namespace {
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QByteArray bytes_;
    UInt64 position_ = 0;

  public:
    HRESULT failure = S_OK;
    UInt64 failSeekAt = UINT64_MAX;
    UInt64 lastSeek = 0;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (failure != S_OK)
            return failure;
        size = UInt32(qMin<UInt64>(
            size, position_ < UInt64(bytes_.size()) ? UInt64(bytes_.size()) - position_ : 0));
        if (size)
            memcpy(data, bytes_.constData() + position_, size);
        position_ += size;
        if (processed)
            *processed = size;
        return S_OK;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        if (failure != S_OK)
            return failure;
        if (origin == STREAM_SEEK_SET && UInt64(offset) == failSeekAt)
            return E_FAIL;
        if (origin > STREAM_SEEK_END)
            return E_INVALIDARG;
        const Int64 base = origin == STREAM_SEEK_SET   ? 0
                           : origin == STREAM_SEEK_CUR ? Int64(position_)
                                                       : bytes_.size();
        if (offset < -base || (offset > 0 && base > INT64_MAX - offset))
            return E_INVALIDARG;
        position_ = UInt64(base + offset);
        lastSeek = position_;
        if (position)
            *position = position_;
        return S_OK;
    }
};

class Extract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)
  public:
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return E_ABORT; }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32)) override {
        *stream = nullptr;
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32)) override { return S_OK; }
};
class CancelOpen final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override { return E_ABORT; }
};
class CancelAt final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
    Input *input_;
    UInt64 checkpoint_;

  public:
    CancelAt(Input *input, UInt64 checkpoint) : input_(input), checkpoint_(checkpoint) {}
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        return input_->lastSeek >= checkpoint_ ? E_ABORT : S_OK;
    }
};
} // namespace

class UdfStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_UDF_CORE)};
    CMyComPtr<IInArchive> archive_;
    QJsonObject files_;
    QByteArray bytes_;

    void createArchive() {
        archive_.Release();
        QVERIFY2(module_.load(), qPrintable(module_.errorString()));
        const auto count =
            reinterpret_cast<Func_GetNumberOfFormats>(module_.resolve("GetNumberOfFormats"));
        const auto property =
            reinterpret_cast<Func_GetHandlerProperty2>(module_.resolve("GetHandlerProperty2"));
        const auto create = reinterpret_cast<Func_CreateObject>(module_.resolve("CreateObject"));
        QVERIFY(count && property && create);
        UInt32 formats = 0;
        QCOMPARE(count(&formats), S_OK);
        for (UInt32 i = 0; i < formats; ++i) {
            NWindows::NCOM::CPropVariant name;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kName, &name), S_OK);
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "Udf")
                continue;
            NWindows::NCOM::CPropVariant clsid;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
            GUID guid;
            memcpy(&guid, clsid.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no UDF handler");
    }
    void fixture(const QString &kind) {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath("volume.udf");
        QProcess process;
        process.start(QString::fromUtf8(SEVENZIP_UDF_PYTHON),
                      {QString::fromUtf8(SEVENZIP_UDF_FIXTURE), kind, path});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        files_ = QJsonDocument::fromJson(process.readAllStandardOutput()).object();
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        bytes_ = file.readAll();
        QVERIFY(!bytes_.isEmpty());
    }
    CMyComPtr<IInStream> stream(const QString &name) {
        CMyComPtr<IInArchiveGetStream> provider;
        if (archive_->QueryInterface(IID_IInArchiveGetStream,
                                     reinterpret_cast<void **>(&provider)) != S_OK)
            return {};
        UInt32 count = 0;
        if (archive_->GetNumberOfItems(&count) != S_OK)
            return {};
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant path;
            if (archive_->GetProperty(i, kpidPath, &path) != S_OK || path.vt != VT_BSTR ||
                QString::fromWCharArray(path.bstrVal).replace('\\', '/') != name)
                continue;
            CMyComPtr<ISequentialInStream> sequential;
            if (provider->GetStream(i, &sequential) != S_OK || !sequential)
                return {};
            CMyComPtr<IInStream> result;
            sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&result));
            return result;
        }
        return {};
    }
    void check(IInStream *stream, const QByteArray &expected) {
        QVERIFY(stream);
        QCOMPARE(stream->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QByteArray actual;
        for (;;) {
            char data[37];
            UInt32 count = 0;
            QCOMPARE(stream->Read(data, sizeof(data), &count), S_OK);
            if (!count)
                break;
            actual.append(data, count);
            QVERIFY(actual.size() <= expected.size());
        }
        QCOMPARE(actual, expected);
        UInt64 end = 0;
        QCOMPARE(stream->Seek(0, STREAM_SEEK_END, &end), S_OK);
        QCOMPARE(end, UInt64(expected.size()));
    }

  private slots:
    void init() { createArchive(); }
    void allocationStreams_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"native", "sparse-short", "sparse-long", "hole-only", "allocated-hole", "eof-short",
              "eof-long", "unused-tail", "inline", "directory-padded", "directory-inline",
              "metadata-bootstrap", "implicit-tail", "implicit-hole", "eof-partition-tail",
              "terminator-short", "terminator-long"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
        for (const char *kind :
             {"ad-short", "ad-long", "ad-chain-short", "ad-chain-long", "ad-limit-16",
              "ad-directory", "ad-cross-partition", "ad-zero", "ad-implicit-tail"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
        for (const char *kind : {"icb-file",
                                 "icb-efe",
                                 "icb-inline",
                                 "icb-root",
                                 "icb-directory",
                                 "icb-chain",
                                 "icb-limit-1024",
                                 "icb-cross-partition",
                                 "icb-ad-continuation",
                                 "icb-metadata",
                                 "icb-metadata-cross",
                                 "icb-no-ie",
                                 "icb-terminal",
                                 "icb-empty-ie",
                                 "icb-bad-ie-crc",
                                 "icb-bad-ie-location",
                                 "icb-reserved",
                                 "icb-extent-short",
                                 "icb-extent-long",
                                 "icb-root-extent-zero"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void allocationStreams() {
        QFETCH(QString, kind);
        fixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto alpha = stream("alpha.bin");
        auto beta = stream("nested/tail.bin");
        auto empty = stream("empty");
        QVERIFY(alpha && beta && empty);
        const QByteArray expected = QByteArray::fromHex(files_["alpha.bin"].toString().toLatin1());
        check(alpha, expected);
        check(beta, QByteArray("directory payload"));
        char first[17];
        UInt32 count = 0;
        QCOMPARE(alpha->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(alpha->Read(first, sizeof(first), &count), S_OK);
        QCOMPARE(QByteArray(first, count), expected.left(17));
        check(beta, QByteArray("directory payload"));
        char next[17];
        QCOMPARE(alpha->Read(next, sizeof(next), &count), S_OK);
        QCOMPARE(QByteArray(next, count), expected.mid(17, 17));
        check(empty, {});
        QCOMPARE(archive_->Close(), S_OK);
        fixture("native");
        CMyComPtr<IInStream> replacement = new Input(bytes_);
        QCOMPARE(archive_->Open(replacement, nullptr, nullptr), S_OK);
        archive_.Release();
        check(alpha, expected);
        check(beta, QByteArray("directory payload"));
    }
    void failuresAndCancellation() {
        fixture("sparse-long");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto alpha = stream("alpha.bin");
        QVERIFY(alpha);
        input->failure = E_FAIL;
        char data[23];
        UInt32 count = 99;
        QCOMPARE(alpha->Read(data, sizeof(data), &count), E_FAIL);
        QCOMPARE(count, UInt32(0));
        input->failure = S_OK;
        CMyComPtr<IArchiveExtractCallback> callback = new Extract;
        QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 1, callback), E_ABORT);
    }
    void continuationOpenFailureAndCancellation() {
        fixture("ad-chain-long");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        input->failSeekAt = UInt64(257 + 21) * 2048;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), E_FAIL);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        input->failSeekAt = UINT64_MAX;
        CMyComPtr<IArchiveOpenCallback> cancel = new CancelOpen;
        QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream("alpha.bin"), QByteArray::fromHex(files_["alpha.bin"].toString().toLatin1()));
    }
    void indirectIcbFailuresAndFallback() {
        fixture("icb-file");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        input->failSeekAt = UInt64(257 + 23) * 2048;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), E_FAIL);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        input->failSeekAt = UINT64_MAX;
        CMyComPtr<IArchiveOpenCallback> cancel =
            new CancelAt(input.operator->(), UInt64(257 + 22) * 2048);
        QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        input->failSeekAt = UInt64(257 + 22) * 2048;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream("alpha.bin"), {});
        NWindows::NCOM::CPropVariant flags;
        QCOMPARE(archive_->GetArchiveProperty(kpidErrorFlags, &flags), S_OK);
        QCOMPARE(flags.vt, VARTYPE(VT_UI4));
        QCOMPARE(flags.ulVal, UInt32(0));
        input->failSeekAt = UINT64_MAX;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream("alpha.bin"), QByteArray::fromHex(files_["alpha.bin"].toString().toLatin1()));
    }
    void backendExtraction_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"sparse-long", "ad-directory", "ad-chain-long", "ad-cross-partition", "icb-efe",
              "icb-root", "icb-cross-partition", "icb-metadata-cross"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void backendExtraction() {
        QFETCH(QString, kind);
        fixture(kind);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QFile image(temporary.filePath(QString::fromUtf8("образ 空.udf")));
        QVERIFY(image.open(QIODevice::WriteOnly));
        QCOMPARE(image.write(bytes_), qint64(bytes_.size()));
        image.close();
        const QDir runtime(QString::fromUtf8(SEVENZIP_UDF_RUNTIME));
        ArchiveBackend backend(nullptr, runtime.filePath(
#ifdef Q_OS_WIN
                                            "7z.exe"
#else
                                            "7z"
#endif
                                            ));
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        const QString core = runtime.filePath(
#ifdef Q_OS_WIN
            "7z.dll"
#else
            "7z.so"
#endif
        );
        QVERIFY(backend.discoverCodecCatalog(core));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        QVERIFY(backend.codecCatalog()->formatIndex("Udf") >= 0);
        QVERIFY(!backend.codecCatalog()->canUpdate("Udf"));
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list(image.fileName());
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        finished.clear();
        const QString output = temporary.filePath("output");
        backend.extract(image.fileName(), output, {"alpha.bin"});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile file(QDir(output).filePath("alpha.bin"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray expected = QByteArray::fromHex(files_["alpha.bin"].toString().toLatin1());
        QCOMPARE(file.readAll(), expected);
        finished.clear();
        const QString saved = temporary.filePath("source.bin");
        backend.extractSourceEntry(image.fileName(), "alpha.bin", saved, "Udf", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile source(saved);
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), expected);
    }
};

QTEST_GUILESS_MAIN(UdfStreamTests)
#include "udf_stream_tests.moc"
