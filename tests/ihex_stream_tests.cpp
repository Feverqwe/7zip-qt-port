#include "7zip/Archive/IArchive.h"
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
    UInt64 failReadAt = UINT64_MAX, failSeekAt = UINT64_MAX;
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
        return S_OK;
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
    Extract() { output.SetFromCls(new Output); }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *completed)) override {
        return cancel && completed && *completed > 0 ? E_ABORT : S_OK;
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
    return QDir(QString::fromUtf8(SEVENZIP_IHEX_RUNTIME)).filePath(QString::fromLatin1(name));
}
QString core() {
#ifdef Q_OS_WIN
    return runtimeFile("7z.dll");
#else
    return runtimeFile("7z.so");
#endif
}
} // namespace

class IhexStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{core()};
    CMyComPtr<IInArchive> archive_;
    QByteArray image_, payload_;
    QJsonObject metadata_;

    void fixture(const QString &kind) {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath("volume.ihex");
        QProcess process;
        process.start(QString::fromUtf8(SEVENZIP_IHEX_PYTHON),
                      {QString::fromUtf8(SEVENZIP_IHEX_FIXTURE), kind, path});
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
        const wchar_t *names[] = {L"iqs5xx"};
        NWindows::NCOM::CPropVariant value;
        value = metadata_["profile"].toBool();
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
        NWindows::NCOM::CPropVariant address;
        QCOMPARE(archive_->GetProperty(item, kpidVa, &address), S_OK);
        QCOMPARE(address.vt, VARTYPE(VT_UI4));
        QCOMPARE(UInt64(address.ulVal), metadata_["address"].toString().toULongLong());
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
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "IHex")
                continue;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &id), S_OK);
            GUID guid;
            memcpy(&guid, id.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no Intel HEX handler");
    }
    void cleanup() { archive_.Release(); }
    void retainedStreams_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"normal", "linear", "segment", "separate", "top-address",
                                 "iqs-settings", "iqs-overlap"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void retainedStreams() {
        QFETCH(QString, kind);
        fixture(kind);
        const QByteArray expected = payload_;
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(input->Seek(7, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        UInt32 count = 0;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(metadata_["count"].toInt()));
        checkExtraction();
        CMyComPtr<IInStream> first, second;
        stream(first, index());
        stream(second, index());
        char buffer[19];
        for (int offset : {0, 33, 18, 4000}) {
            for (auto *reader : {first.operator->(), second.operator->()}) {
                QCOMPARE(reader->Seek(offset, STREAM_SEEK_SET, nullptr), S_OK);
                UInt32 size = 0;
                QCOMPARE(reader->Read(buffer, sizeof(buffer), &size), S_OK);
                QCOMPARE(QByteArray(buffer, size), expected.mid(offset, size));
            }
        }
        QCOMPARE(archive_->Close(), S_OK);
        fixture("normal");
        CMyComPtr<IInStream> replacement = new Input(image_);
        QCOMPARE(archive_->Open(replacement, nullptr, nullptr), S_OK);
        archive_.Release();
        input.Release();
        checkBytes(first, expected);
        checkBytes(second, expected);
    }
    void sourceAndOutputErrors() {
        for (const char *kind : {"normal", "long-gap"}) {
            fixture(kind);
            CMyComPtr2<IInStream, Input> input;
            input.SetFromCls(new Input(image_));
            for (HRESULT failure : {E_FAIL, E_ABORT}) {
                input->failure = failure;
                input->failReadAt = QString::fromLatin1(kind) == "long-gap" ? 32768 : 64;
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
            }
        }
    }
    void cancellation() {
        fixture("long-gap");
        CMyComPtr<IInStream> input = new Input(image_);
        for (UInt64 threshold : {UInt64(0), UInt64(65536)}) {
            CMyComPtr2<IArchiveOpenCallback, Cancel> callback;
            callback.SetFromCls(new Cancel);
            callback->threshold = threshold;
            QCOMPARE(archive_->Open(input, nullptr, callback), E_ABORT);
            checkClosed();
        }
        fixture("normal");
        input = new Input(image_);
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
    }
    void malformedRecordClearsPartialData() {
        fixture("late-checksum");
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_FALSE);
        checkClosed();
        NWindows::NCOM::CPropVariant flags;
        QCOMPARE(archive_->GetArchiveProperty(kpidErrorFlags, &flags), S_OK);
        QCOMPARE(flags.vt, VARTYPE(VT_UI4));
        QVERIFY(flags.ulVal & kpv_ErrorFlags_DataError);
        fixture("normal");
        input = new Input(image_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        checkExtraction();
    }
    void backendExtraction() {
        fixture("separate");
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath(QString::fromUtf8("образ 空.bin"));
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
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list(path);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        finished.clear();
        const QString saved = temporary.filePath("payload.bin");
        backend.extractSourceEntry(path, metadata_["target"].toString(), saved, "IHex", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile result(saved);
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), payload_);
    }
};
QTEST_GUILESS_MAIN(IhexStreamTests)
#include "ihex_stream_tests.moc"
