#include "7zip/Archive/IArchive.h"
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
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (position_ == failAt)
            return failure;
        size = UInt32(qMin<UInt64>(qMin<UInt32>(size, 4096), position_ < UInt64(bytes_.size())
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
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *bytes)) override {
        return bytes && *bytes >= 65536 ? E_ABORT : S_OK;
    }
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
    Z7_COM7F_IMF(SetCompleted(const UInt64 *bytes)) override {
        return cancel && bytes && *bytes >= 65536 ? E_ABORT : S_OK;
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
    return QDir(QString::fromUtf8(SEVENZIP_GZIP_RUNTIME)).filePath(QString::fromLatin1(name));
}
QString core() {
#ifdef Q_OS_WIN
    return runtimeFile("7z.dll");
#else
    return runtimeFile("7z.so");
#endif
}
} // namespace

class GzipStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{core()};
    CMyComPtr<IInArchive> archive_;
    Func_IsArc probe_ = nullptr;
    QByteArray image_, payload_;
    QString name_;

    void fixture(const QString &kind) {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath("volume.gz");
        QProcess process;
        process.start(QString::fromUtf8(SEVENZIP_GZIP_PYTHON),
                      {QString::fromUtf8(SEVENZIP_GZIP_FIXTURE), kind, path});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        const auto object = QJsonDocument::fromJson(process.readAllStandardOutput()).object();
        payload_ = QByteArray::fromHex(object["payload"].toString().toLatin1());
        name_ = object["name"].toString();
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
        NWindows::NCOM::CPropVariant name, flags;
        QCOMPARE(archive_->GetProperty(0, kpidPath, &name), S_OK);
        QCOMPARE(name.vt, VARTYPE(VT_EMPTY));
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
        using GetIsArc = HRESULT(Z7_STDCALL *)(UInt32, Func_IsArc *);
        const auto getProbe = reinterpret_cast<GetIsArc>(module_.resolve("GetIsArc"));
        QVERIFY(property && count && create && getProbe);
        UInt32 formats = 0;
        QCOMPARE(count(&formats), S_OK);
        for (UInt32 i = 0; i < formats; ++i) {
            NWindows::NCOM::CPropVariant name, id;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kName, &name), S_OK);
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "gzip")
                continue;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &id), S_OK);
            GUID guid;
            memcpy(&guid, id.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            QCOMPARE(getProbe(i, &probe_), S_OK);
            QVERIFY(probe_);
            return;
        }
        QFAIL("Prepared engine has no gzip handler");
    }
    void cleanup() { archive_.Release(); }
    void read_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"xfl-255", "stored-padding", "name-long", "comment-long",
                                 "optional-fields", "concatenated", "empty"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void read() {
        QFETCH(QString, kind);
        fixture(kind);
        QCOMPARE(probe_(reinterpret_cast<const Byte *>(image_.constData()), image_.size()),
                 UInt32(k_IsArc_Res_YES));
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        NWindows::NCOM::CPropVariant name;
        QCOMPARE(archive_->GetProperty(0, kpidPath, &name), S_OK);
        QCOMPARE(name.vt, VARTYPE(VT_BSTR));
        QCOMPARE(QString::fromWCharArray(name.bstrVal), name_);
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
        fixture("name-long");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image_));
        input->failAt = 4096;
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
        CMyComPtr2<IArchiveExtractCallback, Extract> extract;
        extract.SetFromCls(new Extract);
        extract->cancel = true;
        QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 0, extract), E_ABORT);
        checkExtraction();
        QCOMPARE(archive_->Close(), S_OK);
        checkClosed();
        fixture("name-unterminated");
        CMyComPtr<IInStream> invalid = new Input(image_);
        QCOMPARE(archive_->Open(invalid, nullptr, nullptr), S_FALSE);
        checkClosed();
    }
    void backendExtraction() {
        fixture("xfl-255");
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
        backend.list(path);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        finished.clear();
        const QString saved = temporary.filePath("payload.bin");
        backend.extractSourceEntry(path, name_, saved, "gzip", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile result(saved);
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), payload_);
    }
};
QTEST_GUILESS_MAIN(GzipStreamTests)
#include "gzip_stream_tests.moc"
