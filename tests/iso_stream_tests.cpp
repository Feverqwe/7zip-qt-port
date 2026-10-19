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
    UInt64 total = 0;
    Int32 result = -1;
    Z7_COM7F_IMF(SetTotal(UInt64 value)) override {
        total = value;
        return S_OK;
    }
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

class IsoStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_ISO_CORE)};
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
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "Iso")
                continue;
            NWindows::NCOM::CPropVariant clsid;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
            GUID guid;
            memcpy(&guid, clsid.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no ISO handler");
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
        const UInt32 item = index(name);
        if (item == UInt32(-1))
            return {};
        if (provider->GetStream(item, &sequential) != S_OK || !sequential)
            return {};
        CMyComPtr<IInStream> result;
        sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&result));
        return result;
    }
    void check(IInStream *stream, const QString &name, int offset, int size) {
        QVERIFY2(stream, qPrintable(name));
        QVERIFY2(record_["files"].toObject().contains(name), qPrintable(name));
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
        QStringList arguments{QString::fromUtf8(SEVENZIP_ISO_FIXTURE), codec};
        generator.start(QString::fromUtf8(SEVENZIP_ISO_PYTHON), arguments);
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
    void independentAndRetainedStreams_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"plain", "multi", "multi-directory", "xar", "xar-directory", "block-512",
              "block-1024", "block-512-tail", "sierra", "sierra-xar", "sierra-multi"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void independentAndRetainedStreams() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto alpha = stream("ALPHA.BIN");
        auto beta = stream("NESTED/BETA.BIN");
        auto boot = stream("[BOOT]/Boot-NoEmul.img");
        const bool hasBoot = !kind.startsWith("sierra");
        QVERIFY(alpha && beta);
        if (hasBoot)
            QVERIFY(boot);
        else
            QVERIFY(!boot);
        check(alpha, "ALPHA.BIN", 0, 37);
        check(beta, "NESTED/BETA.BIN", 0, 74);
        check(alpha, "ALPHA.BIN", 37, 37);
        if (hasBoot)
            check(boot, "[BOOT]/Boot-NoEmul.img", 0, 37);
        char buffer[37];
        UInt32 count = 0;
        QCOMPARE(alpha->Read(buffer, sizeof(buffer), &count), S_OK);
        QCOMPARE(
            QByteArray(buffer, count),
            QByteArray::fromBase64(record_["files"].toObject()["ALPHA.BIN"].toString().toLatin1())
                .mid(74, 37));
        QCOMPARE(archive_->Close(), S_OK);
        const QJsonObject retained = record_;
        loadFixture("joliet");
        CMyComPtr<IInStream> replacement = new Input(bytes_);
        QCOMPARE(archive_->Open(replacement, nullptr, nullptr), S_OK);
        archive_.Release();
        input.SetFromCls(nullptr);
        record_ = retained;
        check(alpha, "ALPHA.BIN", 2040, 8192);
        check(beta, "NESTED/BETA.BIN", 0, 100000);
        if (hasBoot)
            check(boot, "[BOOT]/Boot-NoEmul.img", 0, 2048);
        createArchive();
    }
    void errorsAndSeekBoundaries() {
        loadFixture("plain");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto alpha = stream("ALPHA.BIN");
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
        check(alpha, "ALPHA.BIN", 0, 37);
        input->failure = E_FAIL;
        QCOMPARE(alpha->Read(data, sizeof(data), &count), E_FAIL);
        input->failure = S_OK;
        QVERIFY(alpha->Seek(INT64_MIN, STREAM_SEEK_CUR, nullptr) != S_OK);
        QCOMPARE(alpha->Seek(INT64_MAX, STREAM_SEEK_SET, nullptr), S_OK);
        QVERIFY(alpha->Seek(1, STREAM_SEEK_CUR, nullptr) != S_OK);
        check(alpha, "ALPHA.BIN", 37, 37);
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.SetFromCls(new Extract);
        const UInt32 item = index("ALPHA.BIN");
        QVERIFY(item != UInt32(-1));
        callback->cancel = true;
        QCOMPARE(archive_->Extract(&item, 1, 1, callback), E_ABORT);
        callback->cancel = false;
        input->eof = true;
        QCOMPARE(archive_->Extract(&item, 1, 1, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kDataError));
        input->eof = false;
        input->failure = E_FAIL;
        QCOMPARE(archive_->Extract(&item, 1, 1, callback), E_FAIL);
    }
    void invalidMaps_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"outside-volume", "xar-outside-volume", "short-source",
                                 "missing-final", "misaligned-section", "interleave"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void invalidMaps() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const UInt32 item = index("ALPHA.BIN");
        QVERIFY(item != UInt32(-1));
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> file;
        QCOMPARE(provider->GetStream(item, &file), S_FALSE);
        QVERIFY(!file);
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.SetFromCls(new Extract);
        QCOMPARE(archive_->Extract(&item, 1, 1, callback), S_OK);
        QCOMPARE(callback->result,
                 Int32(kind == "interleave"
                           ? NArchive::NExtract::NOperationResult::kUnsupportedMethod
                           : NArchive::NExtract::NOperationResult::kDataError));
    }
    void failedOpenClearsCatalog() {
        for (const char *kind :
             {"bad-directory", "cycle", "bad-name-length", "short-record", "sierra-invalid-type",
              "sierra-root-file", "sierra-short-root", "sierra-bad-name", "sierra-short-source"}) {
            loadFixture("plain");
            CMyComPtr<IInStream> input = new Input(bytes_);
            QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
            loadFixture(kind);
            input = new Input(bytes_);
            QVERIFY(archive_->Open(input, nullptr, nullptr) != S_OK);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        loadFixture("plain");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        CMyComPtr<IArchiveOpenCallback> callback = new CancelOpen;
        QCOMPARE(archive_->Open(input, nullptr, callback), E_ABORT);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        input->failSeekAt = 0x8000;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), E_FAIL);
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        input->failSeekAt = UINT64_MAX;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream("ALPHA.BIN"), "ALPHA.BIN", 0, 4096);
    }
    void backendExtraction_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"plain", "joliet", "block-512-tail", "xar", "multi", "multi-directory", "rock",
              "rock-joliet", "zisofs", "zisofs-multi", "sierra", "sierra-multi"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void backendExtraction() {
        QFETCH(QString, kind);
        loadFixture(kind);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QFile image(temporary.filePath(QString::fromUtf8("образ 空.iso")));
        QVERIFY(image.open(QIODevice::WriteOnly));
        QCOMPARE(image.write(bytes_), bytes_.size());
        image.close();
        const QDir runtime(QString::fromUtf8(SEVENZIP_ISO_RUNTIME));
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
        QVERIFY(!backend.codecCatalog()->canUpdate("Iso"));
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list(image.fileName());
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        const QString name =
            kind == "joliet" || kind.startsWith("rock") || kind.startsWith("zisofs")
                ? QString::fromUtf8("nested/данные 空.txt")
            : kind == "multi-directory" ? "NESTED/LATE.BIN"
                                        : "ALPHA.BIN";
        const QByteArray expected =
            QByteArray::fromBase64(record_["files"].toObject()[name].toString().toLatin1());
        QVERIFY(!expected.isEmpty());
        finished.clear();
        const QString output = temporary.filePath("output");
        backend.extract(image.fileName(), output, {name});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile file(QDir(output).filePath(name));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), expected);
        finished.clear();
        const QString saved = temporary.filePath("source.bin");
        backend.extractSourceEntry(image.fileName(), name, saved, "Iso", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile source(saved);
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), expected);
        QVERIFY(image.open(QIODevice::ReadOnly));
        QCOMPARE(image.readAll(), bytes_);
    }
    void linuxRecordingDates_data() {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<int>("offset");
        QTest::addColumn<bool>("normalize");
        QTest::newRow("sierra-no-zone") << QString("sierra-date") << 0 << false;
        QTest::newRow("east") << QString("date-east") << 52 << false;
        QTest::newRow("west") << QString("date-west") << -52 << false;
        QTest::newRow("invalid-east") << QString("date-invalid-east") << 0 << false;
        QTest::newRow("invalid-west") << QString("date-invalid-west") << 0 << false;
        QTest::newRow("normalize") << QString("date-normalize") << 0 << true;
    }
    void linuxRecordingDates() {
        QFETCH(QString, kind);
        QFETCH(int, offset);
        QFETCH(bool, normalize);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        NWindows::NCOM::CPropVariant value;
        QCOMPARE(archive_->GetProperty(index("ALPHA.BIN"), kpidMTime, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_FILETIME));
        const qint64 expected = QDateTime(normalize ? QDate(2041, 2, 1) : QDate(2041, 1, 2),
                                          QTime(3, 4, 5), QTimeZone::UTC)
                                    .toSecsSinceEpoch() -
                                offset * 900;
        const UInt64 actual =
            (UInt64(value.filetime.dwHighDateTime) << 32) | value.filetime.dwLowDateTime;
        QCOMPARE(actual, UInt64(expected + 11644473600LL) * 10000000);
        check(stream("ALPHA.BIN"), "ALPHA.BIN", 32760, 32780);
    }
    void compressedStreamsAndLifetime() {
        loadFixture("zisofs");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto alpha = stream("alpha.bin");
        auto noise = stream("noise.bin");
        auto mixed = stream("mixed.bin");
        QVERIFY(alpha && noise && mixed);
        check(alpha, "alpha.bin", 32760, 32780);
        check(noise, "noise.bin", 65530, 40000);
        check(mixed, "mixed.bin", 32760, 32780);
        check(alpha, "alpha.bin", 4080, 8200);
        const UInt32 item = index("alpha.bin");
        NWindows::NCOM::CPropVariant size, packed, method;
        QCOMPARE(archive_->GetProperty(item, kpidSize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(VT_UI8));
        QCOMPARE(size.uhVal.QuadPart, UInt64(131328));
        QCOMPARE(archive_->GetProperty(item, kpidPackSize, &packed), S_OK);
        QCOMPARE(packed.vt, VARTYPE(VT_UI8));
        QVERIFY(packed.uhVal.QuadPart < size.uhVal.QuadPart);
        QCOMPARE(archive_->GetProperty(item, kpidMethod, &method), S_OK);
        QCOMPARE(method.vt, VARTYPE(VT_BSTR));
        QCOMPARE(QString::fromWCharArray(method.bstrVal), QString("zisofs"));
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.SetFromCls(new Extract);
        QCOMPARE(archive_->Extract(&item, 1, 1, callback), S_OK);
        QCOMPARE(callback->total, UInt64(131328));
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        callback->cancel = true;
        QCOMPARE(archive_->Extract(&item, 1, 1, callback), E_ABORT);
        QCOMPARE(archive_->Close(), S_OK);
        const auto retained = record_;
        loadFixture("plain");
        CMyComPtr<IInStream> replacement = new Input(bytes_);
        QCOMPARE(archive_->Open(replacement, nullptr, nullptr), S_OK);
        archive_.Release();
        input.SetFromCls(nullptr);
        record_ = retained;
        check(noise, "noise.bin", 0, 150321);
        check(alpha, "alpha.bin", 131000, 1000);
        check(mixed, "mixed.bin", 65530, 36306);
        char data[37];
        UInt32 count = 99;
        QCOMPARE(alpha->Seek(0, STREAM_SEEK_END, nullptr), S_OK);
        QCOMPARE(alpha->Read(data, sizeof(data), &count), S_OK);
        QCOMPARE(count, UInt32(0));
        QVERIFY(alpha->Seek(INT64_MIN, STREAM_SEEK_CUR, nullptr) != S_OK);
        QCOMPARE(alpha->Seek(INT64_MAX, STREAM_SEEK_SET, nullptr), S_OK);
        QVERIFY(alpha->Seek(1, STREAM_SEEK_CUR, nullptr) != S_OK);
        createArchive();
    }
    void compressedIoFailures() {
        loadFixture("zisofs");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto alpha = stream("alpha.bin");
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
        input->failure = E_ABORT;
        QCOMPARE(alpha->Read(data, sizeof(data), &count), E_ABORT);
        QCOMPARE(count, UInt32(0));
        input->failure = S_OK;
        check(alpha, "alpha.bin", 0, 4096);
    }
    void corruptCompressedExtraction_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"zisofs-crc", "zisofs-short-zlib", "zisofs-short-output", "zisofs-long-output",
              "zisofs-pointer-order", "zisofs-pointer-outside", "zisofs-pointer-header",
              "zisofs-short-table"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void corruptCompressedExtraction() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const UInt32 item = index("alpha.bin");
        QVERIFY(item != UInt32(-1));
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.SetFromCls(new Extract);
        QCOMPARE(archive_->Extract(&item, 1, 1, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kDataError));
    }
    void rockMetadataAndStreams_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"rock", "rock-joliet", "rock-no-er", "rock-metadata",
                                 "rock-pn-legacy", "rock-pn-wide"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void rockMetadataAndStreams() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const UInt32 item = index("alpha.bin");
        QVERIFY(item != UInt32(-1));
        const bool device = kind.startsWith("rock-pn");
        for (const auto &entry :
             {qMakePair(kpidPosixAttrib, UInt32(device ? 0020640 : 0100640)),
              qMakePair(kpidUserId, UInt32(1000)), qMakePair(kpidGroupId, UInt32(1000)),
              qMakePair(kpidLinks, UInt32(1))}) {
            NWindows::NCOM::CPropVariant value;
            QCOMPARE(archive_->GetProperty(item, entry.first, &value), S_OK);
            QCOMPARE(value.vt, VARTYPE(VT_UI4));
            QCOMPARE(value.ulVal, entry.second);
        }
        if (device) {
            for (const auto &entry :
                 {qMakePair(kpidDeviceMajor, UInt32(kind == "rock-pn-legacy" ? 0x12 : 0xabc)),
                  qMakePair(kpidDeviceMinor, UInt32(kind == "rock-pn-legacy" ? 0x34 : 0x54321))}) {
                NWindows::NCOM::CPropVariant value;
                QCOMPARE(archive_->GetProperty(item, entry.first, &value), S_OK);
                QCOMPARE(value.vt, VARTYPE(VT_UI4));
                QCOMPARE(value.ulVal, entry.second);
            }
            QVERIFY(!stream("alpha.bin"));
            CMyComPtr2<IArchiveExtractCallback, Extract> callback;
            callback.SetFromCls(new Extract);
            QCOMPARE(archive_->Extract(&item, 1, 1, callback), S_OK);
            QCOMPARE(callback->result,
                     Int32(NArchive::NExtract::NOperationResult::kUnsupportedMethod));
            return;
        }
        if (kind == "rock-metadata") {
            for (const auto &entry : {qMakePair(kpidMTime, 2041), qMakePair(kpidATime, 2042),
                                      qMakePair(kpidChangeTime, 2043)}) {
                NWindows::NCOM::CPropVariant value;
                QCOMPARE(archive_->GetProperty(item, entry.first, &value), S_OK);
                QCOMPARE(value.vt, VARTYPE(VT_FILETIME));
                const UInt64 actual =
                    (UInt64(value.filetime.dwHighDateTime) << 32) | value.filetime.dwLowDateTime;
                const qint64 seconds =
                    QDateTime(QDate(entry.second, 1, 2), QTime(3, 4, 5), QTimeZone::UTC)
                        .toSecsSinceEpoch();
                QCOMPARE(actual, UInt64(seconds + 11644473600LL) * 10000000 + 3700000);
            }
        }
        QString deep;
        for (int i = 0; i < 11; ++i)
            deep += "level" + QString::number(i) + '/';
        deep += "end.bin";
        auto alpha = stream("alpha.bin");
        auto relocated = stream(deep);
        auto unicode = stream(QString::fromUtf8("nested/данные 空.txt"));
        auto link = stream("long-link");
        QVERIFY(alpha && relocated && unicode && link);
        check(alpha, "alpha.bin", 0, 37);
        check(relocated, deep, 0, 37);
        check(alpha, "alpha.bin", 37, 37);
        NWindows::NCOM::CPropVariant target;
        const UInt32 linkIndex = index("long-link");
        QVERIFY(linkIndex != UInt32(-1));
        QCOMPARE(archive_->GetProperty(linkIndex, kpidSymLink, &target), S_OK);
        QCOMPARE(target.vt, VARTYPE(VT_BSTR));
        const QByteArray expected = "../" + QByteArray(220, 'x') + "/target";
        QCOMPARE(QString::fromWCharArray(target.bstrVal).toUtf8(), expected);
        QCOMPARE(archive_->Close(), S_OK);
        check(alpha, "alpha.bin", 32760, 32780);
        check(relocated, deep, 0, 4000);
        check(unicode, QString::fromUtf8("nested/данные 空.txt"), 0, 4000);
        QByteArray actual(expected.size(), '\0');
        UInt32 count = 0;
        QCOMPARE(link->Read(actual.data(), UInt32(actual.size()), &count), S_OK);
        QCOMPARE(count, UInt32(expected.size()));
        QCOMPARE(actual, expected);
    }
    void failedRockOpenClearsCatalog() {
        for (const char *kind : {"rock-ce-outside", "rock-ce-crossblock", "rock-ce-cycle",
                                 "rock-sl-overrun", "rock-cl-cycle", "rock-cl-outside"}) {
            loadFixture("rock");
            CMyComPtr<IInStream> input = new Input(bytes_);
            QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
            loadFixture(kind);
            input = new Input(bytes_);
            QVERIFY(archive_->Open(input, nullptr, nullptr) != S_OK);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        loadFixture("rock");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        CMyComPtr<IArchiveOpenCallback> callback = new CancelOpen;
        QCOMPARE(archive_->Open(input, nullptr, callback), E_ABORT);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream("alpha.bin"), "alpha.bin", 0, 4096);
    }
    void cleanupTestCase() { archive_.Release(); }
};

QTEST_GUILESS_MAIN(IsoStreamTests)
#include "iso_stream_tests.moc"
