#include "7zip/Archive/IArchive.h"
#include "7zip/Archive/Zip/ZipHandler.h"
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
    UInt64 failReadAt = UINT64_MAX, failSeekAt = UINT64_MAX;
    HRESULT failure = E_FAIL;
    bool partialFailure = false;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        const bool failed = position_ == failReadAt;
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
    return QDir(QString::fromUtf8(SEVENZIP_XZ_RUNTIME)).filePath(QString::fromLatin1(name));
}
QString core() {
#ifdef Q_OS_WIN
    return runtimeFile("7z.dll");
#else
    return runtimeFile("7z.so");
#endif
}
} // namespace

class XzStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{core()};
    CMyComPtr<IInArchive> archive_;
    QByteArray image_, payload_;

    void fixture(const QString &kind) {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath("volume.xz");
        QProcess process;
        process.start(QString::fromUtf8(SEVENZIP_XZ_PYTHON),
                      {QString::fromUtf8(SEVENZIP_XZ_FIXTURE), kind, path,
                       runtimeFile(
#ifdef Q_OS_WIN
                           "7z.exe"
#else
                           "7z"
#endif
                           )});
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
    void stream(CMyComPtr<IInStream> &result) {
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> sequential;
        QCOMPARE(provider->GetStream(0, &sequential), S_OK);
        QVERIFY(sequential);
        QCOMPARE(sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&result)),
                 S_OK);
    }
    void checkBytes(IInStream *stream, const QByteArray &expected) {
        QCOMPARE(stream->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QByteArray actual;
        char buffer[5000];
        for (;;) {
            UInt32 count = 0;
            QCOMPARE(stream->Read(buffer, sizeof(buffer), &count), S_OK);
            if (!count)
                break;
            actual.append(buffer, count);
            QVERIFY(actual.size() <= expected.size());
        }
        QCOMPARE(actual, expected);
        UInt64 position = 0;
        QCOMPARE(stream->Seek(0, STREAM_SEEK_END, &position), S_OK);
        QCOMPARE(position, UInt64(expected.size()));
    }
    void checkClosed() {
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> result;
        QCOMPARE(provider->GetStream(0, &result), S_FALSE);
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
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "xz")
                continue;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &id), S_OK);
            GUID guid;
            memcpy(&guid, id.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no XZ handler");
    }
    void cleanup() { archive_.Release(); }
    void retainedStreams_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"crc32", "empty", "multiple-blocks", "zero-blocks", "long-padding",
                                 "concatenated", "empty-first", "arm64", "riscv"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void retainedStreams() {
        QFETCH(QString, kind);
        fixture(kind);
        const QByteArray expected = payload_;
        CMyComPtr<IInStream> input = new Input(image_);
        // Open must not depend on the caller's current physical position.
        QCOMPARE(input->Seek(7, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        CMyComPtr<IInStream> first, second;
        stream(first);
        stream(second);
        QVERIFY(first && second);
        char buffer[23];
        for (int offset : {8200, 0, 16400, 8190}) {
            if (offset >= expected.size())
                continue;
            for (auto *reader : {first.operator->(), second.operator->()}) {
                QCOMPARE(reader->Seek(offset, STREAM_SEEK_SET, nullptr), S_OK);
                UInt32 count = 0;
                QCOMPARE(reader->Read(buffer, sizeof(buffer), &count), S_OK);
                QVERIFY(count > 0);
                QCOMPARE(QByteArray(buffer, count), expected.mid(offset, count));
            }
        }
        QCOMPARE(archive_->Close(), S_OK);
        fixture(expected.isEmpty() ? "crc32" : "empty");
        CMyComPtr<IInStream> replacement = new Input(image_);
        QCOMPARE(archive_->Open(replacement, nullptr, nullptr), S_OK);
        archive_.Release();
        input.Release();
        checkBytes(first, expected);
        checkBytes(second, expected);
    }
    void sourceErrors() {
        fixture("crc32");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(image_));
        for (HRESULT failure : {E_FAIL, E_ABORT}) {
            input->failure = failure;
            for (bool seek : {false, true}) {
                input->failReadAt = seek ? UINT64_MAX : UInt64(image_.size() - 12);
                input->failSeekAt = seek ? UInt64(image_.size() - 12) : UINT64_MAX;
                QCOMPARE(archive_->Open(input, nullptr, nullptr), failure);
                checkClosed();
            }
            input->failReadAt = input->failSeekAt = UINT64_MAX;
            QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
            CMyComPtr<IInStream> reader;
            stream(reader);
            input->failReadAt = 12;
            char byte;
            UInt32 count = 123;
            QCOMPARE(reader->Read(&byte, 1, &count), failure);
            QCOMPARE(count, UInt32(0));
            input->failReadAt = UINT64_MAX;
            checkBytes(reader, payload_);
            CMyComPtr2<IArchiveExtractCallback, Extract> extract;
            for (bool partial : {false, true}) {
                input->failReadAt = 16;
                input->partialFailure = partial;
                extract.SetFromCls(new Extract);
                QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 0, extract), failure);
            }
            input->failReadAt = UINT64_MAX;
            input->partialFailure = false;
            extract.SetFromCls(new Extract);
            extract->output->failure = failure;
            QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 0, extract), failure);
        }
    }
    void cancellation() {
        for (const char *kind : {"crc32", "long-padding"}) {
            fixture(kind);
            CMyComPtr<IInStream> input = new Input(image_);
            CMyComPtr2<IArchiveOpenCallback, Cancel> cancel;
            cancel.SetFromCls(new Cancel);
            cancel->threshold = QString::fromLatin1(kind) == "long-padding" ? 65536 : 0;
            QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
            checkClosed();
            QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
            CMyComPtr2<IArchiveExtractCallback, Extract> extract;
            extract.SetFromCls(new Extract);
            extract->cancel = true;
            QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 0, extract), E_ABORT);
            extract.SetFromCls(new Extract);
            QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 0, extract), S_OK);
            QCOMPARE(extract->result, Int32(NArchive::NExtract::NOperationResult::kOK));
            QCOMPARE(extract->output->bytes, payload_);
        }
    }
    void integrity_data() {
        QTest::addColumn<QString>("kind");
        QTest::newRow("CRC") << QString("block-crc");
        QTest::newRow("CRC64") << QString("block-crc64");
        QTest::newRow("Index size") << QString("index-unpack");
    }
    void nativeZipConsumer() {
        fixture("zip-xz");
        CMyComPtr<IInArchive> zip = new NArchive::NZip::CHandler;
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(zip->Open(input, nullptr, nullptr), S_OK);
        CMyComPtr2<IArchiveExtractCallback, Extract> extract;
        extract.SetFromCls(new Extract);
        QCOMPARE(zip->Extract(nullptr, UInt32(-1), 0, extract), S_OK);
        QCOMPARE(extract->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(extract->output->bytes, payload_);
    }
    void integrity() {
        QFETCH(QString, kind);
        fixture(kind);
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        CMyComPtr<IInStream> reader;
        stream(reader);
        char byte;
        for (int attempt = 0; attempt < 2; ++attempt) {
            UInt32 count = 123;
            QVERIFY(reader->Read(&byte, 1, &count) != S_OK);
            QCOMPARE(count, UInt32(0));
        }
    }
    void backendExtraction() {
        fixture("long-padding");
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
        backend.extractSourceEntry(path, entries[0].path, saved, "xz", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile result(saved);
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), payload_);
    }
};
QTEST_GUILESS_MAIN(XzStreamTests)
#include "xz_stream_tests.moc"
