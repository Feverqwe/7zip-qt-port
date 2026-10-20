#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLibrary>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

namespace {
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QByteArray bytes_;
    UInt64 position_ = 0;

  public:
    UInt64 failReadAt = UINT64_MAX, failSeekAt = UINT64_MAX, failAfterReadAt = UINT64_MAX;
    HRESULT failure = E_FAIL;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (position_ == failReadAt)
            return failure;
        size = UInt32(qMin<UInt64>(qMin<UInt32>(size, 64), position_ < UInt64(bytes_.size())
                                                               ? UInt64(bytes_.size()) - position_
                                                               : 0));
        if (size)
            memcpy(data, bytes_.constData() + position_, size);
        position_ += size;
        if (processed)
            *processed = size;
        return position_ == failAfterReadAt ? failure : S_OK;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        if (origin > STREAM_SEEK_END)
            return E_INVALIDARG;
        const Int64 base = origin == STREAM_SEEK_SET   ? 0
                           : origin == STREAM_SEEK_CUR ? Int64(position_)
                                                       : Int64(bytes_.size());
        if (offset < -base || (offset > 0 && base > INT64_MAX - offset))
            return E_INVALIDARG;
        const UInt64 next = UInt64(base + offset);
        if (next == failSeekAt)
            return failure;
        position_ = next;
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
    unsigned abortAtCall = UINT_MAX, calls = 0;
    Extract() { output.SetFromCls(new Output); }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *completed)) override {
        return ++calls >= abortAtCall || (cancel && completed && *completed > 0) ? E_ABORT : S_OK;
    }
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
    return QDir(QString::fromUtf8(SEVENZIP_TAR_RUNTIME)).filePath(QString::fromLatin1(name));
}
QString core() {
#ifdef Q_OS_WIN
    return runtimeFile("7z.dll");
#else
    return runtimeFile("7z.so");
#endif
}
} // namespace

class TarStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{core()};
    CMyComPtr<IInArchive> archive_;
    QByteArray image_, payload_;
    QJsonObject metadata_;

    void fixture(const QString &kind) {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath("volume.tar");
        QProcess process;
        process.start(QString::fromUtf8(SEVENZIP_TAR_PYTHON),
                      {QString::fromUtf8(SEVENZIP_TAR_FIXTURE), kind, path});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        metadata_ = QJsonDocument::fromJson(process.readAllStandardOutput()).object();
        payload_ = QByteArray::fromHex(metadata_["payload"].toString().toLatin1());
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        image_ = file.readAll();
        CMyComPtr<ISetProperties> properties;
        QCOMPARE(
            archive_->QueryInterface(IID_ISetProperties, reinterpret_cast<void **>(&properties)),
            S_OK);
        const wchar_t *names[] = {L"linkdata"};
        NWindows::NCOM::CPropVariant value;
        value = metadata_["linkdata"].toBool();
        QCOMPARE(properties->SetProperties(names, &value, 1), S_OK);
    }
    UInt32 index() const { return UInt32(metadata_["index"].toInt()); }
    void stream(CMyComPtr<IInStream> &result, UInt32 item) {
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> sequential;
        QCOMPARE(provider->GetStream(item, &sequential), S_OK);
        QVERIFY(sequential);
        QCOMPARE(sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&result)),
                 S_OK);
    }
    void checkBytes(IInStream *reader, const QByteArray &expected) {
        QCOMPARE(reader->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QByteArray actual;
        char buffer[277];
        for (;;) {
            UInt32 count = 0;
            QCOMPARE(reader->Read(buffer, sizeof(buffer), &count), S_OK);
            if (!count)
                break;
            actual.append(buffer, count);
            QVERIFY(actual.size() <= expected.size());
        }
        QCOMPARE(actual, expected);
    }
    void checkExtraction() {
        const UInt32 item = index();
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.SetFromCls(new Extract);
        QCOMPARE(archive_->Extract(&item, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(callback->output->bytes, payload_);
    }
    void checkClosed() {
        UInt32 count = 123;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> result;
        QCOMPARE(provider->GetStream(0, &result), E_INVALIDARG);
        QVERIFY(!result);
    }
    void openSequential(ISequentialInStream *input) {
        CMyComPtr<IArchiveOpenSeq> sequential;
        QCOMPARE(
            archive_->QueryInterface(IID_IArchiveOpenSeq, reinterpret_cast<void **>(&sequential)),
            S_OK);
        QCOMPARE(sequential->OpenSeq(input), S_OK);
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
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "tar")
                continue;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &id), S_OK);
            GUID guid;
            memcpy(&guid, id.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no TAR handler");
    }
    void cleanup() { archive_.Release(); }
    void retainedStreams_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"ustar", "gnu", "pax", "sparse", "sparse-extended", "symlink",
                                 "linkdata-hardlink", "linkdata-directory"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void retainedStreams() {
        QFETCH(QString, kind);
        fixture(kind);
        const QByteArray expected = payload_;
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(input->Seek(7, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        checkExtraction();
        CMyComPtr<IInStream> first, second;
        stream(first, index());
        stream(second, index());
        char buffer[19];
        for (int offset : {0, 512, 521, 2048, 4000, 1030}) {
            for (auto *reader : {first.operator->(), second.operator->()}) {
                QCOMPARE(reader->Seek(offset, STREAM_SEEK_SET, nullptr), S_OK);
                UInt32 size = 0;
                QCOMPARE(reader->Read(buffer, sizeof(buffer), &size), S_OK);
                QCOMPARE(QByteArray(buffer, size), expected.mid(offset, size));
            }
        }
        QCOMPARE(first->Seek(INT64_MAX, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(first->Seek(1, STREAM_SEEK_CUR, nullptr), E_INVALIDARG);
        QCOMPARE(first->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(first->Seek(INT64_MIN, STREAM_SEEK_CUR, nullptr),
                 HRESULT_WIN32_ERROR_NEGATIVE_SEEK);
        QCOMPARE(archive_->Close(), S_OK);
        fixture("ustar");
        CMyComPtr<IInStream> replacement = new Input(image_);
        QCOMPARE(archive_->Open(replacement, nullptr, nullptr), S_OK);
        archive_.Release();
        input.Release();
        checkBytes(first, expected);
        checkBytes(second, expected);
    }
    void sequentialExtraction_data() { retainedStreams_data(); }
    void sequentialExtraction() {
        QFETCH(QString, kind);
        fixture(kind);
        CMyComPtr<IInStream> input = new Input(image_);
        openSequential(input);
        checkExtraction();
        // Reading a following item tests physical padding and header-position accounting.
        NWindows::NCOM::CPropVariant name;
        QCOMPARE(archive_->GetProperty(index() + 1, kpidPath, &name), S_OK);
        QCOMPARE(name.vt, VARTYPE(VT_BSTR));
        QCOMPARE(QString::fromWCharArray(name.bstrVal), QString("marker.bin"));
        const UInt32 next = index() + 1;
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.SetFromCls(new Extract);
        QCOMPARE(archive_->Extract(&next, 1, 0, callback), S_OK);
        QCOMPARE(callback->output->bytes, QByteArray("following record"));
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        openSequential(input);
        callback.SetFromCls(new Extract);
        QCOMPARE(archive_->Extract(nullptr, UINT32_MAX, 0, callback), S_OK);
        QCOMPARE(callback->output->bytes, payload_ + "following record");
    }
    void sourceAndOutputErrors() {
        for (const char *kind : {"ustar", "pax", "sparse-extended"}) {
            fixture(kind);
            CMyComPtr2<IInStream, Input> input;
            input.SetFromCls(new Input(image_));
            for (HRESULT failure : {E_FAIL, E_ABORT}) {
                input->failure = failure;
                input->failReadAt = 64;
                QCOMPARE(archive_->Open(input, nullptr, nullptr), failure);
                checkClosed();
                input->failReadAt = UINT64_MAX;
                input->failSeekAt = 0;
                QCOMPARE(archive_->Open(input, nullptr, nullptr), failure);
                checkClosed();
                input->failSeekAt = UINT64_MAX;
                QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
                CMyComPtr2<IArchiveExtractCallback, Extract> callback;
                callback.SetFromCls(new Extract);
                callback->output->failure = failure;
                const UInt32 item = index();
                QCOMPARE(archive_->Extract(&item, 1, 0, callback), failure);
                checkExtraction();
                const UInt64 dataPos = kind == QString("sparse-extended")
                                           ? 1024
                                           : UInt64(image_.indexOf(payload_.left(64)));
                input->failReadAt = dataPos + 64;
                callback.SetFromCls(new Extract);
                QCOMPARE(archive_->Extract(&item, 1, 0, callback), failure);
                input->failReadAt = UINT64_MAX;
                input->failSeekAt = dataPos;
                CMyComPtr<IInStream> reader;
                stream(reader, item);
                QCOMPARE(reader->Seek(kind == QString("sparse-extended") ? 512 : 0, STREAM_SEEK_SET,
                                      nullptr),
                         S_OK);
                char bytes[20];
                UInt32 processed = 123;
                QCOMPARE(reader->Read(bytes, sizeof(bytes), &processed), failure);
                QCOMPARE(processed, UInt32(0));
                input->failSeekAt = UINT64_MAX;
                checkExtraction();
            }
        }
        fixture("longname-tail");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image_));
        for (HRESULT failure : {E_FAIL, E_ABORT}) {
            input->failure = failure;
            input->failAfterReadAt = 512 + 49152;
            QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            openSequential(input);
            NWindows::NCOM::CPropVariant name;
            QCOMPARE(archive_->GetProperty(0, kpidPath, &name), failure);
        }
    }
    void cancellation() {
        for (const char *kind : {"large-pax", "long-zero-tail"}) {
            fixture(kind);
            CMyComPtr<IInStream> input = new Input(image_);
            for (UInt64 threshold : {UInt64(0), UInt64(65536), UInt64(image_.size() - 512)}) {
                CMyComPtr2<IArchiveOpenCallback, Cancel> callback;
                callback.SetFromCls(new Cancel);
                callback->threshold = threshold;
                QCOMPARE(archive_->Open(input, nullptr, callback), E_ABORT);
                checkClosed();
            }
        }
        fixture("ustar");
        CMyComPtr<IInStream> input = new Input(image_);
        CMyComPtr2<IArchiveOpenCallback, Cancel> final;
        final.SetFromCls(new Cancel);
        final->threshold = 1;
        QCOMPARE(archive_->Open(input, nullptr, final), E_ABORT);
        checkClosed();
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const UInt32 item = index();
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.SetFromCls(new Extract);
        callback->cancel = true;
        QCOMPARE(archive_->Extract(&item, 1, 0, callback), E_ABORT);
        checkExtraction();
        fixture("large-pax");
        input = new Input(image_);
        openSequential(input);
        callback.SetFromCls(new Extract);
        callback->abortAtCall = 2;
        QCOMPARE(archive_->Extract(nullptr, UINT32_MAX, 0, callback), E_ABORT);
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        openSequential(input);
        checkExtraction();
    }
    void truncatedDataRemainsDiagnosed() {
        fixture("short-payload");
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        NWindows::NCOM::CPropVariant flags;
        QCOMPARE(archive_->GetArchiveProperty(kpidErrorFlags, &flags), S_OK);
        QCOMPARE(flags.vt, VARTYPE(VT_UI4));
        QVERIFY(flags.ulVal & kpv_ErrorFlags_UnexpectedEnd);
        const UInt32 item = 0;
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.SetFromCls(new Extract);
        QCOMPARE(archive_->Extract(&item, 1, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kDataError));
    }
};
QTEST_GUILESS_MAIN(TarStreamTests)
#include "tar_stream_tests.moc"
