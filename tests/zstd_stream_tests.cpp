#include "7zip/Archive/IArchive.h"
#include "7zip/Archive/Zip/ZipHandler.h"
#include "7zip/Compress/ZstdDecoder.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "archivebackend.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
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
    UInt64 failAt = UINT64_MAX;
    HRESULT failure = E_FAIL;
    bool partialFailure = false;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        const bool failed = position_ == failAt;
        if (failed && !partialFailure)
            return failure;
        size = UInt32(qMin<UInt64>(qMin<UInt32>(size, 8), position_ < UInt64(bytes_.size())
                                                              ? UInt64(bytes_.size()) - position_
                                                              : 0));
        if (size)
            memcpy(data, bytes_.constData() + position_, size);
        position_ += size;
        if (processed)
            *processed = size;
        return failed ? failure : S_OK;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        if (origin > STREAM_SEEK_END)
            return E_INVALIDARG;
        const Int64 base = origin == STREAM_SEEK_SET   ? 0
                           : origin == STREAM_SEEK_CUR ? Int64(position_)
                                                       : bytes_.size();
        if (offset < -base || (offset > 0 && base > INT64_MAX - offset))
            return E_INVALIDARG;
        position_ = UInt64(base + offset);
        if (position)
            *position = position_;
        return S_OK;
    }
};
class Cancel final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    UInt64 threshold = 0;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *bytes)) override {
        return bytes && *bytes >= threshold ? E_ABORT : S_OK;
    }
};
class Output final : public ISequentialOutStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialOutStream)
  public:
    QByteArray bytes;
    HRESULT failure = S_OK;
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (failure != S_OK)
            return failure;
        bytes.append(static_cast<const char *>(data), size);
        if (processed)
            *processed = size;
        return S_OK;
    }
};
class Extract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)
  public:
    CMyComPtr2<ISequentialOutStream, Output> output;
    Int32 result = -1;
    bool cancel = false;
    HRESULT prepareResult = S_OK;
    Extract() { output.SetFromCls(new Output); }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return cancel ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32)) override {
        *stream = output;
        (*stream)->AddRef();
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return prepareResult; }
    Z7_COM7F_IMF(SetOperationResult(Int32 value)) override {
        result = value;
        return S_OK;
    }
};
QString runtimeFile(const char *name) {
    return QDir(QString::fromUtf8(SEVENZIP_ZSTD_RUNTIME)).filePath(QString::fromLatin1(name));
}
QString core() {
#ifdef Q_OS_WIN
    return runtimeFile("7z.dll");
#else
    return runtimeFile("7z.so");
#endif
}
} // namespace

class ZstdStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{core()};
    CMyComPtr<IInArchive> archive_;
    QByteArray image_, payload_;

    void fixture(const QString &kind) {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath("volume.zst");
        QProcess process;
        process.start(QString::fromUtf8(SEVENZIP_ZSTD_PYTHON),
                      {QString::fromUtf8(SEVENZIP_ZSTD_FIXTURE), kind, path});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        payload_ = QByteArray::fromHex(QJsonDocument::fromJson(process.readAllStandardOutput())
                                           .object()["payload"]
                                           .toString()
                                           .toLatin1());
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        image_ = file.readAll();
    }
    void checkExtraction() {
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.SetFromCls(new Extract);
        QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(callback->output->bytes, payload_);
    }
    void checkClosed() {
        NWindows::NCOM::CPropVariant size, flags;
        QCOMPARE(archive_->GetProperty(0, kpidSize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(VT_EMPTY));
        QCOMPARE(archive_->GetArchiveProperty(kpidErrorFlags, &flags), S_OK);
        QCOMPARE(flags.vt, VARTYPE(VT_UI4));
        QVERIFY(flags.ulVal & kpv_ErrorFlags_IsNotArc);
    }

  private slots:
    void init() {
        QVERIFY2(module_.load(), qPrintable(module_.errorString()));
        const auto property =
            reinterpret_cast<Func_GetHandlerProperty2>(module_.resolve("GetHandlerProperty2"));
        const auto count =
            reinterpret_cast<Func_GetNumberOfFormats>(module_.resolve("GetNumberOfFormats"));
        const auto create = reinterpret_cast<Func_CreateObject>(module_.resolve("CreateObject"));
        QVERIFY(property && count && create);
        UInt32 formats = 0;
        QCOMPARE(count(&formats), S_OK);
        for (UInt32 i = 0; i < formats; ++i) {
            NWindows::NCOM::CPropVariant name, id;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kName, &name), S_OK);
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "zstd")
                continue;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &id), S_OK);
            GUID guid;
            memcpy(&guid, id.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no zstd handler");
    }
    void cleanup() { archive_.Release(); }
    void read_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"raw", "empty", "rle", "checksum", "huffman", "skip-first", "skip-only",
              "concatenated", "multi-block", "dict-zero-4", "unknown-size"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void read() {
        QFETCH(QString, kind);
        fixture(kind);
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        checkExtraction();
        NWindows::NCOM::CPropVariant size, packed, streams;
        QCOMPARE(archive_->GetProperty(0, kpidSize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(VT_UI8));
        QCOMPARE(size.uhVal.QuadPart, UInt64(payload_.size()));
        QCOMPARE(archive_->GetProperty(0, kpidPackSize, &packed), S_OK);
        QCOMPARE(packed.vt, VARTYPE(VT_UI8));
        QCOMPARE(packed.uhVal.QuadPart, UInt64(image_.size()));
        QCOMPARE(archive_->GetArchiveProperty(kpidNumStreams, &streams), S_OK);
        QCOMPARE(streams.vt, VARTYPE(VT_UI8));
        QCOMPARE(streams.uhVal.QuadPart, UInt64(kind == "skip-only"      ? 0
                                                : kind == "concatenated" ? 2
                                                                         : 1));
        CMyComPtr<IArchiveOpenSeq> sequential;
        QCOMPARE(
            archive_->QueryInterface(IID_IArchiveOpenSeq, reinterpret_cast<void **>(&sequential)),
            S_OK);
        CMyComPtr<IInStream> fresh = new Input(image_);
        QCOMPARE(sequential->OpenSeq(fresh), S_OK);
        checkExtraction();
    }
    void failuresAndCancellation_data() {
        QTest::addColumn<QString>("kind");
        QTest::newRow("short") << QString("raw");
        QTest::newRow("multi-block") << QString("multi-block");
    }
    void failuresAndCancellation() {
        QFETCH(QString, kind);
        fixture(kind);
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image_));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        CMyComPtr2<IArchiveExtractCallback, Extract> cancelled;
        cancelled.SetFromCls(new Extract);
        cancelled->cancel = true;
        QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 0, cancelled), E_ABORT);
        input->failAt = 8;
        for (HRESULT result : {E_FAIL, E_ABORT}) {
            QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            input->failure = result;
            QCOMPARE(archive_->Open(input, nullptr, nullptr), result);
            checkClosed();
        }
        input->failAt = UINT64_MAX;
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        CMyComPtr<IArchiveOpenCallback> cancel = new Cancel;
        QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
        checkClosed();
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        for (HRESULT result : {E_FAIL, E_ABORT}) {
            input->failAt = kind == "multi-block" ? 131072 : 16;
            input->failure = result;
            CMyComPtr2<IArchiveExtractCallback, Extract> extract;
            extract.SetFromCls(new Extract);
            QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 0, extract), result);
            input->failAt = UINT64_MAX;
            extract.SetFromCls(new Extract);
            extract->output->failure = result;
            QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 0, extract), result);
        }
        checkExtraction();
        cancelled.SetFromCls(new Extract);
        cancelled->prepareResult = E_ABORT;
        QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 0, cancelled), E_ABORT);
        QVERIFY(cancelled->output->bytes.isEmpty());
    }
    void backendExtraction() {
        fixture("skip-first");
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath(QString::fromUtf8("архив 空.bin"));
        QFile image(path);
        QVERIFY(image.open(QIODevice::WriteOnly));
        QCOMPARE(image.write(image_), image_.size());
        image.close();
        ArchiveBackend backend(nullptr, runtimeFile(
#ifdef Q_OS_WIN
                                            "7z.exe"
#else
                                            "7z"
#endif
                                            ));
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(core()));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 35000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        QList<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, this,
                [&entries](const QList<ArchiveEntry> &value) { entries = value; });
        backend.list(path);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        finished.clear();
        QCOMPARE(entries.size(), 1);
        const QString saved = temporary.filePath("payload.bin");
        backend.extractSourceEntry(path, entries[0].path, saved, "zstd", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile result(saved);
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), payload_);
    }
    void nativeZipConsumer() {
        fixture("zip-zstd");
        CMyComPtr<IInArchive> zip = new NArchive::NZip::CHandler;
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(zip->Open(input, nullptr, nullptr), S_OK);
        CMyComPtr2<IArchiveExtractCallback, Extract> extract;
        extract.SetFromCls(new Extract);
        QCOMPARE(zip->Extract(nullptr, UInt32(-1), 0, extract), S_OK);
        QCOMPARE(extract->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(extract->output->bytes, payload_);
    }
    void parsedMetadataAndCancellation() {
        CMyComPtr<ISetProperties> properties;
        QCOMPARE(
            archive_->QueryInterface(IID_ISetProperties, reinterpret_cast<void **>(&properties)),
            S_OK);
        const wchar_t *name = L"parse";
        NWindows::NCOM::CPropVariant value(true);
        QCOMPARE(properties->SetProperties(&name, &value, 1), S_OK);
        fixture("concatenated");
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(input->Seek(7, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        NWindows::NCOM::CPropVariant size, frames;
        QCOMPARE(archive_->GetProperty(0, kpidSize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(VT_UI8));
        QCOMPARE(size.uhVal.QuadPart, UInt64(payload_.size()));
        QCOMPARE(archive_->GetArchiveProperty(kpidNumStreams, &frames), S_OK);
        QCOMPARE(frames.vt, VARTYPE(VT_UI8));
        QCOMPARE(frames.uhVal.QuadPart, UInt64(2));
        checkExtraction();
        fixture("size-overflow");
        input = new Input(image_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_FALSE);
        checkClosed();
        fixture("multi-block");
        input = new Input(image_);
        CMyComPtr2<IArchiveOpenCallback, Cancel> cancel;
        cancel.SetFromCls(new Cancel);
        cancel->threshold = 65536;
        QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
        checkClosed();
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        checkExtraction();
    }
    void checksumErrorPreservesOutput() {
        fixture("checksum");
        QByteArray damaged = image_;
        damaged[damaged.size() - 1] = char(damaged.back() ^ 1);
        for (UInt32 chunk : {1u, 13u, 65536u}) {
            CMyComPtr2<ICompressCoder, NCompress::NZstd::CDecoder> coder;
            coder.SetFromCls(new NCompress::NZstd::CDecoder);
            coder->FinishMode = true;
            CMyComPtr<ICompressSetBufSize> buffers;
            QCOMPARE(coder.Interface()->QueryInterface(IID_ICompressSetBufSize,
                                                       reinterpret_cast<void **>(&buffers)),
                     S_OK);
            QCOMPARE(buffers->SetInBufSize(0, chunk), S_OK);
            CMyComPtr2<IInStream, Input> input;
            input.SetFromCls(new Input(damaged));
            CMyComPtr2<ISequentialOutStream, Output> output;
            output.SetFromCls(new Output);
            QVERIFY(coder.Interface()->Code(input, output, nullptr, nullptr, nullptr) != S_OK);
            QCOMPARE(coder->ResInfo.decode_SRes, SZ_ERROR_CRC);
            QCOMPARE(output->bytes, payload_);

            input.SetFromCls(new Input(image_));
            output.SetFromCls(new Output);
            QCOMPARE(coder.Interface()->Code(input, output, nullptr, nullptr, nullptr), S_OK);
            QCOMPARE(output->bytes, payload_);
        }
    }
    void coderSourceErrors() {
        fixture("raw");
        for (HRESULT failure : {E_FAIL, E_ABORT}) {
            for (bool partial : {false, true}) {
                for (bool readable : {false, true}) {
                    CMyComPtr2<ICompressCoder, NCompress::NZstd::CDecoder> coder;
                    coder.SetFromCls(new NCompress::NZstd::CDecoder);
                    coder->FinishMode = true;
                    CMyComPtr2<IInStream, Input> input;
                    QByteArray bytes = image_;
                    if (partial)
                        bytes[0] = 0; // Invalid partial bytes must not hide the source failure.
                    input.SetFromCls(new Input(bytes));
                    input->failure = failure;
                    input->failAt = partial ? 0 : 8;
                    input->partialFailure = partial;
                    if (readable) {
                        CMyComPtr<ICompressSetInStream> setter;
                        CMyComPtr<ICompressSetOutStreamSize> size;
                        CMyComPtr<ISequentialInStream> reader;
                        QCOMPARE(coder.Interface()->QueryInterface(
                                     IID_ICompressSetInStream, reinterpret_cast<void **>(&setter)),
                                 S_OK);
                        QCOMPARE(
                            coder.Interface()->QueryInterface(IID_ICompressSetOutStreamSize,
                                                              reinterpret_cast<void **>(&size)),
                            S_OK);
                        QCOMPARE(coder.Interface()->QueryInterface(
                                     IID_ISequentialInStream, reinterpret_cast<void **>(&reader)),
                                 S_OK);
                        QCOMPARE(setter->SetInStream(input), S_OK);
                        QCOMPARE(size->SetOutStreamSize(nullptr), S_OK);
                        char byte;
                        UInt32 count = 123;
                        QCOMPARE(reader->Read(&byte, 1, &count), failure);
                        QCOMPARE(count, UInt32(0));
                        QCOMPARE(coder->GetFinishResult(), failure);
                    } else {
                        CMyComPtr<ISequentialOutStream> output = new Output;
                        QCOMPARE(coder.Interface()->Code(input, output, nullptr, nullptr, nullptr),
                                 failure);
                    }
                    CMyComPtr<IInStream> fresh = new Input(image_);
                    CMyComPtr2<ISequentialOutStream, Output> output;
                    output.SetFromCls(new Output);
                    QCOMPARE(coder.Interface()->Code(fresh, output, nullptr, nullptr, nullptr),
                             S_OK);
                    QCOMPARE(output->bytes, payload_);
                }
            }
        }
    }
};
QTEST_GUILESS_MAIN(ZstdStreamTests)
#include "zstd_stream_tests.moc"
