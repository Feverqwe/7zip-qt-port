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
    HRESULT failure = S_OK;
    HRESULT seekFailure = E_FAIL;
    UInt64 failSeekAt = UINT64_MAX;
    UInt64 failReadAt = UINT64_MAX;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (failure != S_OK)
            return failure;
        if (position_ == failReadAt)
            return seekFailure;
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
            return seekFailure;
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
class Cancel final : public IArchiveExtractCallback, public CMyUnknownImp {
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
QString runtimeFile(const char *name) {
    return QDir(QString::fromUtf8(SEVENZIP_GPT_RUNTIME)).filePath(QString::fromLatin1(name));
}
QString core() {
#ifdef Q_OS_WIN
    return runtimeFile("7z.dll");
#else
    return runtimeFile("7z.so");
#endif
}
} // namespace

class GptStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{core()};
    CMyComPtr<IInArchive> archive_;
    QByteArray image_, payload_;

    void fixture(const QString &kind) {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath("volume.gpt");
        QProcess process;
        process.start(QString::fromUtf8(SEVENZIP_GPT_PYTHON),
                      {QString::fromUtf8(SEVENZIP_GPT_FIXTURE), kind, path});
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
    CMyComPtr<IInStream> stream() {
        CMyComPtr<IInArchiveGetStream> provider;
        if (archive_->QueryInterface(IID_IInArchiveGetStream,
                                     reinterpret_cast<void **>(&provider)) != S_OK)
            return {};
        CMyComPtr<ISequentialInStream> sequential;
        if (provider->GetStream(0, &sequential) != S_OK || !sequential)
            return {};
        CMyComPtr<IInStream> result;
        sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&result));
        return result;
    }
    void check(IInStream *data, const QByteArray &expected) {
        QVERIFY(data);
        QCOMPARE(data->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QByteArray result;
        for (;;) {
            char bytes[127];
            UInt32 processed = 99;
            QCOMPARE(data->Read(bytes, sizeof(bytes), &processed), S_OK);
            if (!processed)
                break;
            result.append(bytes, processed);
            QVERIFY(result.size() <= expected.size());
        }
        QCOMPARE(result, expected);
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
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "GPT")
                continue;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &id), S_OK);
            GUID guid;
            memcpy(&guid, id.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no GPT handler");
    }
    void cleanup() { archive_.Release(); }
    void streams_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"sector-512", "sector-4096", "backup-crc", "entry-outside"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void streams() {
        QFETCH(QString, kind);
        fixture(kind);
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        UInt32 count = 0;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
        auto first = stream(), second = stream();
        QVERIFY(first && second);
        char byte;
        UInt32 processed = 0;
        QCOMPARE(first->Read(&byte, 1, &processed), S_OK);
        QCOMPARE(processed, UInt32(1));
        check(second, payload_);
        QCOMPARE(first->Read(&byte, 1, &processed), S_OK);
        QCOMPARE(processed, UInt32(1));
        QCOMPARE(byte, payload_[1]);
        const QByteArray expected = payload_;
        QCOMPARE(archive_->Close(), S_OK);
        fixture("primary-only");
        CMyComPtr<IInStream> replacement = new Input(image_);
        QCOMPARE(archive_->Open(replacement, nullptr, nullptr), S_OK);
        archive_.Release();
        input.Release();
        check(first, expected);
    }
    void failuresAndCancellation() {
        fixture("sector-512");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image_));
        UInt32 count = 99;
        for (HRESULT result : {E_FAIL, E_ABORT}) {
            input->failSeekAt = 512;
            input->seekFailure = result;
            QCOMPARE(archive_->Open(input, nullptr, nullptr), result);
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        input->failSeekAt = UINT64_MAX;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto data = stream();
        QVERIFY(data);
        input->failure = E_FAIL;
        char bytes[7];
        UInt32 processed = 99;
        QCOMPARE(data->Read(bytes, sizeof(bytes), &processed), E_FAIL);
        QCOMPARE(processed, UInt32(0));
        input->failure = S_OK;
        CMyComPtr<IArchiveExtractCallback> cancel = new Cancel;
        QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 1, cancel), E_ABORT);
        fixture("partial-failure");
        CMyComPtr<IInStream> damaged = new Input(image_);
        QCOMPARE(archive_->Open(damaged, nullptr, nullptr), S_FALSE);
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream(), payload_);
        fixture("backup-address");
        input.SetFromCls(new Input(image_));
        input->failReadAt = 20 * 512;
        for (HRESULT result : {E_FAIL, E_ABORT}) {
            input->seekFailure = result;
            QCOMPARE(archive_->Open(input, nullptr, nullptr), result);
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        input->failReadAt = UINT64_MAX;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream(), payload_);
    }
    void backendExtraction() {
        fixture("sector-2048");
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath(QString::fromUtf8("диск 空.bin"));
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
        const QString name = QString::fromUtf8("0.данные 空.img");
        const QString saved = temporary.filePath("partition.bin");
        backend.extractSourceEntry(path, name, saved, "GPT", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile result(saved);
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), payload_);
    }
};
QTEST_GUILESS_MAIN(GptStreamTests)
#include "gpt_stream_tests.moc"
