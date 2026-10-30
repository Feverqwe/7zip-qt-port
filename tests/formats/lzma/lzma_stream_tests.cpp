#include "7zip/Archive/IArchive.h"
#include "7zip/Archive/Zip/ZipHandler.h"
#include "7zip/ICoder.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "archive/backend/archivebackend.h"
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
        const bool fail = position_ == failAt;
        if (fail && !partialFailure)
            return failure;
        size = UInt32(qMin<UInt64>(qMin<UInt32>(size, 8), position_ < UInt64(bytes_.size())
                                                              ? UInt64(bytes_.size()) - position_
                                                              : 0));
        if (size)
            memcpy(data, bytes_.constData() + position_, size);
        position_ += size;
        if (processed)
            *processed = size;
        return fail ? failure : S_OK;
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
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override { return E_ABORT; }
};
class Output final : public ISequentialOutStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialOutStream)
  public:
    QByteArray bytes;
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *processed)) override {
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
    Extract() { output.SetFromCls(new Output); }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return cancel ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32)) override {
        *stream = output;
        (*stream)->AddRef();
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 value)) override {
        result = value;
        return S_OK;
    }
};
QString runtimeFile(const char *name) {
    return QDir(QString::fromUtf8(SEVENZIP_LZMA_RUNTIME)).filePath(QString::fromLatin1(name));
}
QString core() {
#ifdef Q_OS_WIN
    return runtimeFile("7z.dll");
#else
    return runtimeFile("7z.so");
#endif
}
} // namespace

class LzmaStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{core()};
    CMyComPtr<IInArchive> archive_;
    Func_IsArc probe_ = nullptr;
    QByteArray image_, payload_;
    QString format_;

    void fixture(const QString &kind) {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath("volume.lzma");
        QProcess process;
        process.start(QString::fromUtf8(SEVENZIP_LZMA_PYTHON),
                      {QString::fromUtf8(SEVENZIP_LZMA_FIXTURE), kind, path});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        const auto object = QJsonDocument::fromJson(process.readAllStandardOutput()).object();
        payload_ = QByteArray::fromHex(object["payload"].toString().toLatin1());
        format_ = object["format"].toString();
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        image_ = file.readAll();
    }
    void handler(const QString &format) {
        const auto property =
            reinterpret_cast<Func_GetHandlerProperty2>(module_.resolve("GetHandlerProperty2"));
        const auto count =
            reinterpret_cast<Func_GetNumberOfFormats>(module_.resolve("GetNumberOfFormats"));
        const auto create = reinterpret_cast<Func_CreateObject>(module_.resolve("CreateObject"));
        using GetIsArc = HRESULT(Z7_STDCALL *)(UInt32, Func_IsArc *);
        const auto getProbe = reinterpret_cast<GetIsArc>(module_.resolve("GetIsArc"));
        QVERIFY(property && count && create && getProbe);
        archive_.Release();
        UInt32 formats = 0;
        QCOMPARE(count(&formats), S_OK);
        for (UInt32 i = 0; i < formats; ++i) {
            NWindows::NCOM::CPropVariant name, id;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kName, &name), S_OK);
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != format)
                continue;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &id), S_OK);
            GUID guid;
            memcpy(&guid, id.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            QCOMPARE(getProbe(i, &probe_), S_OK);
            QVERIFY(probe_);
            return;
        }
        QFAIL("Prepared engine has no selected LZMA handler");
    }
    void checkExtraction() {
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.SetFromCls(new Extract);
        QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(callback->output->bytes, payload_);
    }
    void checkClosed() {
        NWindows::NCOM::CPropVariant method, flags;
        QCOMPARE(archive_->GetArchiveProperty(kpidMethod, &method), S_OK);
        QCOMPARE(method.vt, VARTYPE(VT_EMPTY));
        QCOMPARE(archive_->GetArchiveProperty(kpidErrorFlags, &flags), S_OK);
        QCOMPARE(flags.vt, VARTYPE(VT_UI4));
        QVERIFY(flags.ulVal & kpv_ErrorFlags_IsNotArc);
    }
    CMyComPtr<ICompressCoder> decoder() {
        const auto count =
            reinterpret_cast<Func_GetNumberOfMethods>(module_.resolve("GetNumberOfMethods"));
        const auto property =
            reinterpret_cast<Func_GetMethodProperty>(module_.resolve("GetMethodProperty"));
        const auto create = reinterpret_cast<Func_CreateObject>(module_.resolve("CreateObject"));
        if (!count || !property || !create)
            return {};
        UInt32 methods = 0;
        if (count(&methods) != S_OK)
            return {};
        for (UInt32 i = 0; i < methods; ++i) {
            NWindows::NCOM::CPropVariant name, id;
            if (property(i, NMethodPropID::kName, &name) != S_OK || name.vt != VT_BSTR ||
                QString::fromWCharArray(name.bstrVal) != "LZMA")
                continue;
            if (property(i, NMethodPropID::kDecoder, &id) != S_OK || id.vt != VT_BSTR)
                return {};
            GUID guid;
            memcpy(&guid, id.bstrVal, sizeof(guid));
            CMyComPtr<ICompressCoder> result;
            create(&guid, &IID_ICompressCoder, reinterpret_cast<void **>(&result));
            return result;
        }
        return {};
    }

  private slots:
    void init() {
        QVERIFY2(module_.load(), qPrintable(module_.errorString()));
        handler("lzma");
    }
    void cleanup() { archive_.Release(); }
    void read_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"dict-zero", "dict-three", "dict-5001", "props-max", "known", "empty-known",
              "empty-first", "concat-odd", "lzma86", "lzma86-bcj"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void read() {
        QFETCH(QString, kind);
        fixture(kind);
        handler(format_);
        QCOMPARE(probe_(reinterpret_cast<const Byte *>(image_.constData()), image_.size()),
                 UInt32(k_IsArc_Res_YES));
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        checkExtraction();
        CMyComPtr<IArchiveOpenSeq> sequential;
        QCOMPARE(
            archive_->QueryInterface(IID_IArchiveOpenSeq, reinterpret_cast<void **>(&sequential)),
            S_OK);
        CMyComPtr<IInStream> fresh = new Input(image_);
        QCOMPARE(sequential->OpenSeq(fresh), S_OK);
        checkExtraction();
    }
    void failuresAndCancellation() {
        fixture("unknown");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image_));
        input->failAt = 8;
        for (HRESULT result : {E_FAIL, E_ABORT}) {
            input->failure = result;
            QCOMPARE(archive_->Open(input, nullptr, nullptr), result);
            checkClosed();
        }
        input->failAt = UINT64_MAX;
        CMyComPtr<IArchiveOpenCallback> cancel = new Cancel;
        QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
        checkClosed();
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        for (HRESULT result : {E_FAIL, E_ABORT}) {
            input->failAt = 16;
            input->failure = result;
            CMyComPtr2<IArchiveExtractCallback, Extract> extract;
            extract.SetFromCls(new Extract);
            QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 0, extract), result);
        }
        input->failAt = UINT64_MAX;
        CMyComPtr2<IArchiveExtractCallback, Extract> extract;
        extract.SetFromCls(new Extract);
        extract->cancel = true;
        QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 0, extract), E_ABORT);
        checkExtraction();
    }
    void declaredSize() {
        fixture("size-56bit");
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        NWindows::NCOM::CPropVariant size;
        QCOMPARE(archive_->GetProperty(0, kpidSize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(VT_UI8));
        QCOMPARE(size.uhVal.QuadPart, UInt64(1) << 56);
        CMyComPtr2<IArchiveExtractCallback, Extract> extract;
        extract.SetFromCls(new Extract);
        QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 0, extract), S_OK);
        QCOMPARE(extract->result, Int32(NArchive::NExtract::NOperationResult::kDataError));
    }
    void nativeZipConsumer() {
        fixture("zip-lzma");
        CMyComPtr<IInArchive> zip = new NArchive::NZip::CHandler;
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(zip->Open(input, nullptr, nullptr), S_OK);
        CMyComPtr2<IArchiveExtractCallback, Extract> extract;
        extract.SetFromCls(new Extract);
        QCOMPARE(zip->Extract(nullptr, UInt32(-1), 0, extract), S_OK);
        QCOMPARE(extract->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(extract->output->bytes, payload_);
    }
    void coderReadFailures() {
        fixture("unknown");
        for (HRESULT failure : {E_FAIL, E_ABORT}) {
            for (bool partial : {false, true}) {
                for (bool readable : {false, true}) {
                    auto coder = decoder();
                    QVERIFY(coder);
                    CMyComPtr<ICompressSetDecoderProperties2> props;
                    QCOMPARE(coder->QueryInterface(IID_ICompressSetDecoderProperties2,
                                                   reinterpret_cast<void **>(&props)),
                             S_OK);
                    QCOMPARE(props->SetDecoderProperties2(
                                 reinterpret_cast<const Byte *>(image_.constData()), 5),
                             S_OK);
                    QByteArray bytes = image_.mid(13);
                    if (partial)
                        bytes[0] = 1; // Malformed partial data must not hide a source HRESULT.
                    CMyComPtr2<IInStream, Input> input;
                    input.SetFromCls(new Input(bytes));
                    input->failure = failure;
                    input->failAt = partial ? 0 : 8;
                    input->partialFailure = partial;
                    if (!readable) {
                        CMyComPtr<ISequentialOutStream> output = new Output;
                        QCOMPARE(coder->Code(input, output, nullptr, nullptr, nullptr), failure);
                    } else {
                        CMyComPtr<ICompressSetInStream> setter;
                        CMyComPtr<ICompressSetOutStreamSize> size;
                        CMyComPtr<ISequentialInStream> reader;
                        QCOMPARE(coder->QueryInterface(IID_ICompressSetInStream,
                                                       reinterpret_cast<void **>(&setter)),
                                 S_OK);
                        QCOMPARE(coder->QueryInterface(IID_ICompressSetOutStreamSize,
                                                       reinterpret_cast<void **>(&size)),
                                 S_OK);
                        QCOMPARE(coder->QueryInterface(IID_ISequentialInStream,
                                                       reinterpret_cast<void **>(&reader)),
                                 S_OK);
                        QCOMPARE(setter->SetInStream(input), S_OK);
                        QCOMPARE(size->SetOutStreamSize(nullptr), S_OK);
                        char output[64];
                        UInt32 processed = 0;
                        QCOMPARE(reader->Read(output, sizeof(output), &processed), failure);
                    }
                }
            }
        }
    }
    void backendExtraction() {
        fixture("dict-5001");
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
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 10000);
        QVERIFY(ready[0][0].toBool());
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
        backend.extractSourceEntry(path, entries[0].path, saved, "lzma", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile result(saved);
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), payload_);
    }
};
QTEST_GUILESS_MAIN(LzmaStreamTests)
#include "lzma_stream_tests.moc"
