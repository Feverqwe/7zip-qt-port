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
#include <QtEndian>
#include <QtTest>

namespace {
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QByteArray bytes_;
    UInt64 position_ = 0;

  public:
    HRESULT failure = S_OK;
    HRESULT seekFailure = S_OK;
    bool eof = false;
    UInt64 failSeekAt = UINT64_MAX;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (failure != S_OK)
            return failure;
        if (eof)
            return S_OK;
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
        if (seekFailure != S_OK) {
            const HRESULT result = seekFailure;
            seekFailure = S_OK;
            return result;
        }
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
class Extract final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)
  public:
    bool cancel = false;
    Int32 result = -1;
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return cancel ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32)) override {
        *stream = nullptr;
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 value)) override {
        result = value;
        return S_OK;
    }
};
class CancelOpen final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override { return E_ABORT; }
};
} // namespace

class HfsStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_HFS_CORE)};
    CMyComPtr<IInArchive> archive_;
    QJsonObject record_;
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
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "HFS")
                continue;
            NWindows::NCOM::CPropVariant clsid;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
            GUID guid;
            memcpy(&guid, clsid.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no HFS handler");
    }
    UInt32 index(const QString &name) {
        UInt32 count = 0;
        if (archive_->GetNumberOfItems(&count) != S_OK)
            return UInt32(-1);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant path;
            if (archive_->GetProperty(i, kpidPath, &path) == S_OK && path.vt == VT_BSTR &&
                QString::fromWCharArray(path.bstrVal).replace('\\', '/') == name)
                return i;
        }
        return UInt32(-1);
    }
    CMyComPtr<IInStream> stream(const QString &name) {
        CMyComPtr<IInArchiveGetStream> provider;
        if (archive_->QueryInterface(IID_IInArchiveGetStream,
                                     reinterpret_cast<void **>(&provider)) != S_OK)
            return {};
        CMyComPtr<ISequentialInStream> sequential;
        if (provider->GetStream(index(name), &sequential) != S_OK || !sequential)
            return {};
        CMyComPtr<IInStream> result;
        sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&result));
        return result;
    }
    void check(IInStream *stream, const QString &name, int offset, int size) {
        QVERIFY2(stream, qPrintable(name));
        const auto expected =
            QByteArray::fromBase64(record_["files"].toObject()[name].toString().toLatin1())
                .mid(offset, size);
        QCOMPARE(stream->Seek(offset, STREAM_SEEK_SET, nullptr), S_OK);
        QByteArray actual;
        while (actual.size() < expected.size()) {
            char data[37];
            UInt32 count = 0;
            QCOMPARE(
                stream->Read(data,
                             UInt32(qMin(qsizetype(sizeof(data)), expected.size() - actual.size())),
                             &count),
                S_OK);
            QVERIFY(count > 0);
            actual.append(data, count);
        }
        QCOMPARE(actual, expected);
    }
    void loadFixture(const QString &codec) {
        QProcess generator;
        QStringList arguments{QString::fromUtf8(SEVENZIP_HFS_FIXTURE), codec};
        generator.start(QString::fromUtf8(SEVENZIP_HFS_PYTHON), arguments);
        QVERIFY(generator.waitForFinished(10000));
        QCOMPARE(generator.exitCode(), 0);
        record_ = QJsonDocument::fromJson(generator.readAllStandardOutput()).object();
        QVERIFY(!record_.isEmpty());
        QByteArray packed(4, '\0');
        qToBigEndian(quint32(record_["size"].toInt()), packed.data());
        packed += QByteArray::fromBase64(record_["image"].toString().toLatin1());
        bytes_ = qUncompress(packed);
        QVERIFY(!bytes_.isEmpty());
    }
  private slots:
    void initTestCase() { createArchive(); }
    void independentAndRetainedForks() {
        loadFixture("overflow-leaves");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto alpha = stream("HfsFixture/alpha");
        auto beta = stream(QString::fromUtf8("HfsFixture/данные 空"));
        auto resource = stream("HfsFixture/alpha:rsrc");
        auto attribute = stream("HfsFixture/alpha:com.sevenzip.fixture");
        QVERIFY(alpha && beta && resource && attribute);
        check(alpha, "HfsFixture/alpha", 0, 37);
        check(beta, QString::fromUtf8("HfsFixture/данные 空"), 0, 74);
        check(alpha, "HfsFixture/alpha", 37, 37);
        check(resource, "HfsFixture/alpha:rsrc", 0, 37);
        check(alpha, "HfsFixture/alpha", 74, 37);
        QCOMPARE(archive_->Close(), S_OK);
        loadFixture("base");
        CMyComPtr2<IInStream, Input> replacement;
        replacement.SetFromCls(new Input(bytes_));
        QCOMPARE(archive_->Open(replacement, nullptr, nullptr), S_OK);
        archive_.Release();
        input.SetFromCls(nullptr);
        check(alpha, "HfsFixture/alpha", 32760, 32780);
        check(resource, "HfsFixture/alpha:rsrc", 0, 2422);
        check(attribute, "HfsFixture/alpha:com.sevenzip.fixture", 0, 24);
        createArchive();
    }
    void errorsAndSeekBoundaries() {
        loadFixture("base");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto alpha = stream("HfsFixture/alpha");
        QVERIFY(alpha);
        char data[37];
        UInt32 count = 99;
        input->eof = true;
        QCOMPARE(alpha->Read(data, sizeof(data), &count), S_FALSE);
        QCOMPARE(count, UInt32(0));
        input->eof = false;
        input->seekFailure = E_NOTIMPL;
        QCOMPARE(alpha->Read(data, sizeof(data), &count), E_NOTIMPL);
        QCOMPARE(count, UInt32(0));
        check(alpha, "HfsFixture/alpha", 0, 37);
        input->failure = E_FAIL;
        QCOMPARE(alpha->Read(data, sizeof(data), &count), E_FAIL);
        input->failure = S_OK;
        check(alpha, "HfsFixture/alpha", 37, 37);
        UInt64 position = 0;
        QVERIFY(alpha->Seek(INT64_MIN, STREAM_SEEK_CUR, nullptr) != S_OK);
        QCOMPARE(alpha->Seek(0, STREAM_SEEK_CUR, &position), S_OK);
        QCOMPARE(position, UInt64(74));
        QCOMPARE(alpha->Seek(INT64_MAX, STREAM_SEEK_SET, nullptr), S_OK);
        QVERIFY(alpha->Seek(1, STREAM_SEEK_CUR, nullptr) != S_OK);
        check(alpha, "HfsFixture/alpha", 0, 128);
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.SetFromCls(new Extract);
        callback->cancel = true;
        const UInt32 item = index("HfsFixture/alpha");
        QCOMPARE(archive_->Extract(&item, 1, 1, callback), E_ABORT);
        callback->cancel = false;
        input->eof = true;
        QCOMPARE(archive_->Extract(&item, 1, 1, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kDataError));
        input->eof = false;
        QCOMPARE(archive_->Close(), S_OK);
    }
    void failedOpenClearsCatalog() {
        for (const char *kind :
             {"leaf-height", "metadata-short-map", "parent-cycle", "wrapped-short"}) {
            loadFixture(kind);
            CMyComPtr2<IInStream, Input> input;
            input.SetFromCls(new Input(bytes_));
            QVERIFY(archive_->Open(input, nullptr, nullptr) != S_OK);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        loadFixture("base");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        CMyComPtr2<IArchiveOpenCallback, CancelOpen> callback;
        callback.SetFromCls(new CancelOpen);
        QCOMPARE(archive_->Open(input, nullptr, callback), E_ABORT);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        input.SetFromCls(new Input(bytes_));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream("HfsFixture/alpha"), "HfsFixture/alpha", 0, 4096);
    }
    void backendExtraction_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"preallocated", "overflow-leaves", "extent-over-volume"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void backendExtraction() {
        QFETCH(QString, kind);
        loadFixture(kind);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath(QString::fromUtf8("образ 空.hfs"));
        QFile image(path);
        QVERIFY(image.open(QIODevice::WriteOnly));
        QCOMPARE(image.write(bytes_), bytes_.size());
        image.close();
        const QDir runtime(QFileInfo(QString::fromUtf8(SEVENZIP_HFS_CORE)).absolutePath());
        ArchiveBackend backend(nullptr, runtime.filePath(
#ifdef Q_OS_WIN
                                            "7z.exe"
#else
                                            "7z"
#endif
                                            ));
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(QString::fromUtf8(SEVENZIP_HFS_CORE)));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list(path);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QVERIFY(!backend.codecCatalog()->canUpdate("HFS"));
        finished.clear();
        const QString name = "HfsFixture/alpha";
        const QString output = temporary.filePath("output");
        backend.extract(path, output, {name});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        const bool corrupt = kind == "extent-over-volume";
        QCOMPARE(finished[0][0].toBool(), !corrupt);
        if (!corrupt) {
            QFile file(QDir(output).filePath(name));
            QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(file.readAll(), QByteArray::fromBase64(
                                         record_["files"].toObject()[name].toString().toLatin1()));
            for (const QString &entry : {QString("HfsFixture/alpha:rsrc"),
                                         QString("HfsFixture/alpha:com.sevenzip.fixture"),
                                         QString::fromUtf8("HfsFixture/данные 空")}) {
                finished.clear();
                const QString saved = temporary.filePath("stream.bin");
                QFile::remove(saved);
                backend.extractSourceEntry(path, entry, saved, "HFS", {}, entry.contains(':'));
                QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
                QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
                QFile source(saved);
                QVERIFY(source.open(QIODevice::ReadOnly));
                QCOMPARE(source.readAll(),
                         QByteArray::fromBase64(
                             record_["files"].toObject()[entry].toString().toLatin1()));
            }
        }
        QVERIFY(image.open(QIODevice::ReadOnly));
        QCOMPARE(image.readAll(), bytes_);
    }
    void cleanupTestCase() { archive_.Release(); }
};

QTEST_GUILESS_MAIN(HfsStreamTests)
#include "hfs_stream_tests.moc"
