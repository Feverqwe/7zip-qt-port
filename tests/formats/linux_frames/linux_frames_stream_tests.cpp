#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "archive/backend/archivebackend.h"
#include "formats/initramfs/linuxframestream.h"
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
    UInt64 cancelAt = UINT64_MAX;
    unsigned abortAtCall = UINT_MAX, calls = 0;
    Extract() { output.SetFromCls(new Output); }
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *completed)) override {
        return ++calls >= abortAtCall ||
                       (completed && ((cancel && *completed > 0) || *completed >= cancelAt))
                   ? E_ABORT
                   : S_OK;
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
    return QDir(QString::fromUtf8(SEVENZIP_FRAME_RUNTIME)).filePath(QString::fromLatin1(name));
}
QString core() {
#ifdef Q_OS_WIN
    return QString::fromUtf8(SEVENZIP_FRAME_MODULE);
#else
    return QString::fromUtf8(SEVENZIP_FRAME_MODULE);
#endif
}
} // namespace

class LinuxFramesStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{core()};
    CMyComPtr<IInArchive> archive_;
    QByteArray image_, payload_;
    QString format_;

    void create(const QString &format) {
        if (archive_ && format_ == format)
            return;
        archive_.Release();
        format_ = format;
        const auto property =
            reinterpret_cast<Func_GetHandlerProperty2>(module_.resolve("GetHandlerProperty2"));
        const auto count =
            reinterpret_cast<Func_GetNumberOfFormats>(module_.resolve("GetNumberOfFormats"));
        const auto factory = reinterpret_cast<Func_CreateObject>(module_.resolve("CreateObject"));
        QVERIFY(property && count && factory);
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
            QCOMPARE(factory(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared module has no requested frame reader");
    }
    void fixture(const QString &kind) {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath("sample.bin");
        QProcess process;
        process.start(QString::fromUtf8(SEVENZIP_FRAME_PYTHON),
                      {QString::fromUtf8(SEVENZIP_FRAME_FIXTURE), kind, path});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        const QJsonObject metadata =
            QJsonDocument::fromJson(process.readAllStandardOutput()).object();
        payload_ = QByteArray::fromHex(metadata["payload"].toString().toLatin1());
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        image_ = file.readAll();
        create(metadata["format"].toString());
    }
    void checkExtraction() {
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.SetFromCls(new Extract);
        QCOMPARE(archive_->Extract(nullptr, UINT32_MAX, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(callback->output->bytes, payload_);
        NWindows::NCOM::CPropVariant size;
        QCOMPARE(archive_->GetProperty(0, kpidSize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(VT_UI8));
        QCOMPARE(size.uhVal.QuadPart, UInt64(payload_.size()));
    }
    void checkClosed() {
        UInt32 count = 123;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        NWindows::NCOM::CPropVariant property;
        QCOMPARE(archive_->GetProperty(0, kpidSize, &property), E_INVALIDARG);
    }

  private slots:
    void init() { QVERIFY2(module_.load(), qPrintable(module_.errorString())); }
    void cleanup() { archive_.Release(); }
    void openAndExtract_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"lz4-literal", "lz4-match", "lz4-concat", "lz4-empty",
                                 "lzo-stored", "lzo-rle", "lzo-compressed-checksums", "lzo-concat",
                                 "lzo-empty", "lzo-max-name", "lzo-no-checksum"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void openAndExtract() {
        QFETCH(QString, kind);
        fixture(kind);
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(input->Seek(7, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        input.Release();
        checkExtraction();
        checkExtraction();
        QCOMPARE(archive_->Close(), S_OK);
        checkClosed();
    }
    void sourceErrorsAndCancellation() {
        for (const char *kind : {"lz4-literal", "lzo-stored"}) {
            fixture(kind);
            CMyComPtr2<IInStream, Input> input;
            input.SetFromCls(new Input(image_));
            for (HRESULT failure : {E_FAIL, E_ABORT}) {
                input->failure = failure;
                input->failSeekAt = 0;
                QCOMPARE(archive_->Open(input, nullptr, nullptr), failure);
                checkClosed();
                input->failSeekAt = UINT64_MAX;
                input->failReadAt = 0;
                QCOMPARE(archive_->Open(input, nullptr, nullptr), failure);
                checkClosed();
                input->failReadAt = UINT64_MAX;
                QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
                input->failReadAt = format_ == "Lzop" ? 64 : 72;
                CMyComPtr2<IArchiveExtractCallback, Extract> callback;
                callback.SetFromCls(new Extract);
                QCOMPARE(archive_->Extract(nullptr, UINT32_MAX, 0, callback), failure);
                input->failReadAt = UINT64_MAX;
                input->failAfterReadAt = format_ == "Lzop" ? 64 : 72;
                callback.SetFromCls(new Extract);
                QCOMPARE(archive_->Extract(nullptr, UINT32_MAX, 0, callback), failure);
                input->failAfterReadAt = UINT64_MAX;
                callback.SetFromCls(new Extract);
                callback->output->failure = failure;
                QCOMPARE(archive_->Extract(nullptr, UINT32_MAX, 0, callback), failure);
                checkExtraction();
            }
            for (UInt64 threshold : {UInt64(0), UInt64(1)}) {
                CMyComPtr2<IArchiveOpenCallback, Cancel> callback;
                callback.SetFromCls(new Cancel);
                callback->threshold = threshold;
                QCOMPARE(archive_->Open(input, nullptr, callback), E_ABORT);
                checkClosed();
            }
            QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
            CMyComPtr2<IArchiveExtractCallback, Extract> callback;
            callback.SetFromCls(new Extract);
            callback->cancel = true;
            QCOMPARE(archive_->Extract(nullptr, UINT32_MAX, 0, callback), E_ABORT);
            checkExtraction();
        }
    }
    void exactFrameBoundaryWithReadAhead_data() {
        QTest::addColumn<QString>("kind");
        QTest::newRow("lzop") << QString("lzo-rle");
        QTest::newRow("lz4") << QString("lz4-zero-end");
    }
    void exactFrameBoundaryWithReadAhead() {
        QFETCH(QString, kind);
        fixture(kind);
        const UInt64 frameSize = image_.size() - (kind.startsWith("lz4") ? 4 : 0);
        const QByteArray following = QByteArray("070701") + QByteArray(300, 'x');
        const QByteArray inputBytes = image_ + following;
        CMyComPtr<IInStream> input = new Input(inputBytes);
        CMyComPtr2<ISequentialOutStream, Output> output;
        output.SetFromCls(new Output);
        UInt64 consumed = UINT64_MAX, produced = 0;
        QCOMPARE(SevenzipDecodeLinuxFrame(format_ == "Lzop", input, 0, inputBytes.size(), output,
                                          nullptr, consumed, produced),
                 S_OK);
        QCOMPARE(consumed, frameSize);
        QCOMPARE(produced, UInt64(payload_.size()));
        QCOMPARE(output->bytes, payload_);
        UInt64 position = 0;
        QCOMPARE(input->Seek(0, STREAM_SEEK_CUR, &position), S_OK);
        QCOMPARE(position, frameSize);
        char next[40];
        UInt32 count = 0;
        QCOMPARE(input->Read(next, sizeof(next), &count), S_OK);
        QCOMPARE(QByteArray(next, count), inputBytes.mid(frameSize, count));
    }
    void lateCorruptionDoesNotPublishSuccessfulSize() {
        fixture("lzo-late-checksum");
        CMyComPtr<IInStream> input = new Input(image_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.SetFromCls(new Extract);
        QCOMPARE(archive_->Extract(nullptr, UINT32_MAX, 0, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kDataError));
        NWindows::NCOM::CPropVariant size, flags;
        QCOMPARE(archive_->GetProperty(0, kpidSize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(VT_EMPTY));
        QCOMPARE(archive_->GetArchiveProperty(kpidErrorFlags, &flags), S_OK);
        QVERIFY(flags.ulVal & kpv_ErrorFlags_DataError);
    }
    void cancellationInZeroTail() {
        for (const char *kind : {"lz4-zero-tail", "lzo-zero-tail"}) {
            fixture(kind);
            CMyComPtr<IInStream> input = new Input(image_);
            QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
            CMyComPtr2<IArchiveExtractCallback, Extract> callback;
            callback.SetFromCls(new Extract);
            callback->cancelAt = image_.size() - (192 << 10) + (1 << 16);
            QCOMPARE(archive_->Extract(nullptr, UINT32_MAX, 0, callback), E_ABORT);
            checkExtraction();
        }
    }
    void backendNestedExtraction() {
        for (const char *kind : {"lz4-cpio", "lzo-cpio"}) {
            fixture(kind);
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
            const QString catalog = runtimeFile(
#ifdef Q_OS_WIN
                "7z.dll"
#else
                "7z.so"
#endif
            );
            QVERIFY(backend.discoverCodecCatalog(catalog));
            QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 10000);
            QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
            QSignalSpy finished(&backend, &ArchiveBackend::finished);
            QList<ArchiveEntry> entries;
            connect(&backend, &ArchiveBackend::entriesReady, &backend,
                    [&](const QList<ArchiveEntry> &listed) { entries = listed; });
            backend.list(path);
            QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
            QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
            QCOMPARE(entries.size(), 1);
            finished.clear();
            const QString inner = temporary.filePath("inner.cpio");
            backend.extractSourceEntry(path, entries[0].path, inner, format_, {});
            QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
            QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
            finished.clear();
            const QString saved = temporary.filePath("payload.bin");
            backend.extractSourceEntry(inner, QString::fromUtf8("данные 空.txt"), saved, "Cpio",
                                       {});
            QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
            QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
            QFile result(saved);
            QVERIFY(result.open(QIODevice::ReadOnly));
            QCOMPARE(result.readAll(), QByteArray("nested cpio payload"));
        }
    }
};
QTEST_GUILESS_MAIN(LinuxFramesStreamTests)
#include "linux_frames_stream_tests.moc"
