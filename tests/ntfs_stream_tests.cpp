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
#include <algorithm>

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
    HRESULT failedSeekResult = E_FAIL;
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
            return failedSeekResult;
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
class CancelEaOpen final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    unsigned checkpoints = 0, abortAt = UINT_MAX;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *files, const UInt64 *bytes)) override {
        if (!files && !bytes && ++checkpoints == abortAt)
            return E_ABORT;
        return S_OK;
    }
};
} // namespace

class NtfsStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_NTFS_CORE)};
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
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "NTFS")
                continue;
            NWindows::NCOM::CPropVariant clsid;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
            GUID guid;
            memcpy(&guid, clsid.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no NTFS handler");
    }
    UInt32 index(const QString &name) {
        UInt32 count = 0;
        if (archive_->GetNumberOfItems(&count) != S_OK)
            return UInt32(-1);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant path;
            if (archive_->GetProperty(i, kpidPath, &path) == S_OK && path.vt == VT_BSTR &&
                QDir::fromNativeSeparators(QString::fromWCharArray(path.bstrVal)) == name)
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
        QStringList arguments{QString::fromUtf8(SEVENZIP_NTFS_FIXTURE), "--kind=" + codec};
        generator.start(QString::fromUtf8(SEVENZIP_NTFS_PYTHON), arguments);
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
    void wslMetadata_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"ea-resident", "ea-nonresident", "ea-fragmented", "ea-extension", "ea-readonly",
              "ea-incomplete", "ea-long-uid", "ea-short-info", "ea-no-data", "ea-next-size",
              "ea-zero-size", "ea-device-wide", "ea-mode-high", "ea-chr", "ea-blk", "ea-fifo",
              "ea-sock", "ea-directory", "ea-stale-directory-bit"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void wslMetadata() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const auto expected = record_["wsl"].toObject();
        const UInt32 id = index(kind == "ea-directory" ? "[SYSTEM]/$Extend" : "gamma");
        QVERIFY(id != UINT32_MAX);
        NWindows::NCOM::CPropVariant directory, attributes;
        QCOMPARE(archive_->GetProperty(id, kpidIsDir, &directory), S_OK);
        QCOMPARE(archive_->GetProperty(id, kpidAttrib, &attributes), S_OK);
        QCOMPARE(directory.vt, VARTYPE(VT_BOOL));
        QCOMPARE(attributes.vt, VARTYPE(VT_UI4));
        QCOMPARE(bool(attributes.ulVal & 0x10), directory.boolVal != VARIANT_FALSE);
        for (const auto &field :
             {qMakePair(kpidUserId, "uid"), qMakePair(kpidGroupId, "gid"),
              qMakePair(kpidPosixAttrib, "mode"), qMakePair(kpidDeviceMajor, "major"),
              qMakePair(kpidDeviceMinor, "minor")}) {
            NWindows::NCOM::CPropVariant value;
            QCOMPARE(archive_->GetProperty(id, field.first, &value), S_OK);
            const QJsonValue wanted = expected[field.second];
            if (wanted.isUndefined() || wanted.isNull())
                QCOMPARE(value.vt, VARTYPE(VT_EMPTY));
            else {
                QCOMPARE(value.vt, VARTYPE(VT_UI4));
                QCOMPARE(value.ulVal, UInt32(wanted.toDouble()));
            }
        }
        if (record_["files"].toObject().contains("gamma")) {
            auto retained = stream("gamma");
            QVERIFY(retained);
            check(retained, "gamma", 4090, 8200);
            QCOMPARE(archive_->Close(), S_OK);
            check(retained, "gamma", 0, 200000);
        } else {
            CMyComPtr<IInArchiveGetStream> provider;
            QCOMPARE(archive_->QueryInterface(IID_IInArchiveGetStream,
                                              reinterpret_cast<void **>(&provider)),
                     S_OK);
            CMyComPtr<ISequentialInStream> data;
            QCOMPARE(provider->GetStream(id, &data), E_NOTIMPL);
            QVERIFY(!data);
            CMyComPtr2<IArchiveExtractCallback, Extract> callback;
            callback.Create_if_Empty();
            QCOMPARE(archive_->Extract(&id, 1, true, callback), S_OK);
            QCOMPARE(callback->result,
                     Int32(NArchive::NExtract::NOperationResult::kUnsupportedMethod));
        }
    }
    void eaReadFailuresAndRecovery() {
        loadFixture("ea-fragmented");
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(bytes_));
        for (HRESULT result : {E_FAIL, E_ABORT}) {
            source->failSeekAt = 1102 * 4096;
            source->failedSeekResult = result;
            QCOMPARE(archive_->Open(source, nullptr, nullptr), result);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        source->failSeekAt = UINT64_MAX;
        CMyComPtr2<IArchiveOpenCallback, CancelEaOpen> callback;
        callback.SetFromCls(new CancelEaOpen);
        callback->abortAt = 2; // AttrDef read succeeds; cancel in the nonresident EA read.
        QCOMPARE(archive_->Open(source, nullptr, callback), E_ABORT);
        callback->checkpoints = 0;
        callback->abortAt = UINT_MAX;
        QCOMPARE(archive_->Open(source, nullptr, callback), S_OK);
        QVERIFY(callback->checkpoints >= 2);
        check(stream("gamma"), "gamma", 0, 200000);
    }
    void reads_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"base",
                                 "sparse",
                                 "hole-first",
                                 "initialized",
                                 "compressed",
                                 "short-compressed",
                                 "sector4k",
                                 "boot-reserved",
                                 "backup-boot",
                                 "reparse",
                                 "mft-list",
                                 "mft-list-nonresident",
                                 "wof-xpress4k",
                                 "wof-xpress8k",
                                 "wof-xpress16k",
                                 "wof-lzx32k",
                                 "wof-xpress4k-resident",
                                 "wof-xpress8k-resident",
                                 "wof-xpress16k-resident",
                                 "wof-lzx32k-resident",
                                 "wof-lzx32k-initialized",
                                 "wof-xpress4k-uninitialized"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void reads() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const auto files = record_["files"].toObject();
        for (auto it = files.begin(); it != files.end(); ++it) {
            auto file = stream(it.key());
            QVERIFY2(file, qPrintable(it.key()));
            check(file, it.key(), 0, 200000);
            check(file, it.key(), 4090, 8200);
        }
        if (kind == "reparse") {
            CMyComPtr<IArchiveGetRawProps> raw;
            QCOMPARE(
                archive_->QueryInterface(IID_IArchiveGetRawProps, reinterpret_cast<void **>(&raw)),
                S_OK);
            const void *data = nullptr;
            UInt32 size = 0, type = 0;
            QCOMPARE(raw->GetRawProp(index("gamma"), kpidNtReparse, &data, &size, &type), S_OK);
            QCOMPARE(size, 30u);
            QCOMPARE(qFromLittleEndian<quint32>(data), 0xa000000cu);
            QCOMPARE(QByteArray(static_cast<const char *>(data) + 20, 10),
                     QByteArray("a\0l\0p\0h\0a\0", 10));
        }
    }
    void interleavedStreamsSurviveArchive_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"base", "wof-xpress4k", "wof-lzx32k"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void interleavedStreamsSurviveArchive() {
        QFETCH(QString, kind);
        loadFixture(kind);
        auto *source = new Input(bytes_);
        CMyComPtr<IInStream> input = source;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto first = stream("alpha"), second = stream("gamma"),
             resident = stream(QStringLiteral("данные 空"));
        QVERIFY(first && second && resident);
        char data[19];
        UInt32 count = 0;
        QCOMPARE(first->Read(data, sizeof(data), &count), S_OK);
        QCOMPARE(second->Read(data, sizeof(data), &count), S_OK);
        QCOMPARE(first->Read(data, sizeof(data), &count), S_OK);
        QCOMPARE(QByteArray(data, count),
                 QByteArray::fromBase64(record_["files"].toObject()["alpha"].toString().toLatin1())
                     .mid(19, 19));
        QCOMPARE(input->Seek(17, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(second->Read(data, sizeof(data), &count), S_OK);
        QCOMPARE(QByteArray(data, count),
                 QByteArray::fromBase64(record_["files"].toObject()["gamma"].toString().toLatin1())
                     .mid(19, 19));
        QCOMPARE(archive_->Close(), S_OK);
        CMyComPtr<IInStream> next = new Input(bytes_);
        QCOMPARE(archive_->Open(next, nullptr, nullptr), S_OK);
        archive_.Release();
        input.Release();
        check(first, "alpha", 8191, 8193);
        check(second, "gamma", 32500, 9000);
        check(second, "gamma", 0, 8200);
        check(resident, QStringLiteral("данные 空"), 0, 100);
        createArchive();
    }
    void errorsAndRetry_data() {
        QTest::addColumn<QString>("kind");
        QTest::newRow("raw") << QString("base");
        QTest::newRow("compressed") << QString("compressed");
        QTest::newRow("WOF-XPRESS") << QString("wof-xpress4k");
        QTest::newRow("WOF-LZX") << QString("wof-lzx32k");
    }
    void errorsAndRetry() {
        QFETCH(QString, kind);
        loadFixture(kind);
        auto *source = new Input(bytes_);
        CMyComPtr<IInStream> input = source;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const QString name = kind.startsWith("wof") ? "gamma" : "alpha";
        auto file = stream(name);
        QVERIFY(file);
        char data[19];
        UInt32 count = 99;
        source->seekFailure = E_FAIL;
        QCOMPARE(file->Read(data, sizeof(data), &count), E_FAIL);
        QCOMPARE(count, 0u);
        source->failure = E_ABORT;
        QCOMPARE(file->Read(data, sizeof(data), &count), E_ABORT);
        source->failure = S_OK;
        check(file, name, 0, 8200);
        QCOMPARE(file->Seek(INT64_MIN, STREAM_SEEK_CUR, nullptr),
                 HRESULT_WIN32_ERROR_NEGATIVE_SEEK);
        QCOMPARE(file->Seek(INT64_MAX, STREAM_SEEK_CUR, nullptr), E_INVALIDARG);
        if (kind == "base") {
            source->eof = true;
            QCOMPARE(file->Read(data, sizeof(data), &count), S_FALSE);
            source->eof = false;
        }
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        callback->cancel = true;
        const UInt32 id = index(name);
        QCOMPARE(archive_->Extract(&id, 1, true, callback), E_ABORT);
        callback->cancel = false;
        QCOMPARE(archive_->Extract(&id, 1, true, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
    }
    void malformed_data() {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<HRESULT>("result");
        QTest::newRow("out-of-volume") << QString("run-over-volume") << HRESULT(S_FALSE);
        QTest::newRow("underflow") << QString("run-underflow") << HRESULT(S_FALSE);
        QTest::newRow("normal-virtual-over-volume")
            << QString("normal-virtual-over-volume") << HRESULT(S_FALSE);
        QTest::newRow("encrypted") << QString("encrypted") << HRESULT(E_NOTIMPL);
        QTest::newRow("WOF-missing-ADS") << QString("wof") << HRESULT(S_FALSE);
        QTest::newRow("WOF-without-unnamed-data") << QString("wof-empty") << HRESULT(E_NOTIMPL);
        QTest::newRow("bad-compressed") << QString("bad-compressed") << HRESULT(S_OK);
    }
    void malformed() {
        QFETCH(QString, kind);
        QFETCH(HRESULT, result);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> file;
        const UInt32 id = index(kind.startsWith("wof") ? "gamma" : "alpha");
        QCOMPARE(provider->GetStream(id, &file), result);
        if (kind == "bad-compressed") {
            char data[100];
            UInt32 count = 99;
            QVERIFY(file);
            for (int i = 0; i < 2; ++i) {
                QCOMPARE(file->Read(data, sizeof(data), &count), S_FALSE);
                QCOMPARE(count, 0u);
            }
        }
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        QCOMPARE(archive_->Extract(&id, 1, true, callback), S_OK);
        QCOMPARE(callback->result,
                 Int32(result == E_NOTIMPL
                           ? NArchive::NExtract::NOperationResult::kUnsupportedMethod
                           : NArchive::NExtract::NOperationResult::kDataError));
    }
    void corruptMetadataRejectsOpenAndReuses_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"invalid-initialized", "mft-list-bad-sequence", "mft-list-bad-id",
                                 "mft-list-unmapped", "mft-list-bad-list"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void corruptMetadataRejectsOpenAndReuses() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_FALSE);
        loadFixture("base");
        input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
    }
    void wofCorruptionDoesNotPublishPartialCachedOutput_data() {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<int>("badOffset");
        for (const char *codec : {"xpress4k", "lzx32k"}) {
            const int size = QString(codec).startsWith("lzx") ? 32768 : 4096;
            QTest::newRow(qPrintable(QString(codec) + "-frame"))
                << (QString("wof-") + codec + "-bad-frame") << (2 * size);
            QTest::newRow(qPrintable(QString(codec) + "-table"))
                << (QString("wof-") + codec + "-bad-table") << size;
        }
        QTest::newRow("short-ADS") << QString("wof-xpress4k-short-ads") << 0;
    }
    void lzntFailureDoesNotRetainPreviousCacheTag() {
        loadFixture("cache-collision");
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto file = stream("alpha");
        QVERIFY(file);
        check(file, "alpha", 0, 100);
        QCOMPARE(file->Seek(2 * 65536, STREAM_SEEK_SET, nullptr), S_OK);
        char data[100];
        UInt32 count = 99;
        QCOMPARE(file->Read(data, sizeof(data), &count), S_FALSE);
        QCOMPARE(count, 0u);
        check(file, "alpha", 0, 100);
    }
    void largeWofUses64bitOffsetsAcrossTablePages() {
        loadFixture("wof-lzx32k-large");
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto file = stream("gamma");
        QVERIFY(file);
        const auto files = record_["files"].toObject();
        const QByteArray tail = QByteArray::fromBase64(files["large-last"].toString().toLatin1());
        NWindows::NCOM::CPropVariant size;
        QCOMPARE(archive_->GetProperty(index("gamma"), kpidSize, &size), S_OK);
        QCOMPARE(size.uhVal.QuadPart, (UInt64(1) << 32) + UInt64(tail.size()));
        QCOMPARE(file->Seek(Int64(1) << 32, STREAM_SEEK_SET, nullptr), S_OK);
        QByteArray actual(tail.size(), '\0');
        UInt32 count = 0;
        QCOMPARE(file->Read(actual.data(), UInt32(actual.size()), &count), S_OK);
        QCOMPARE(count, UInt32(actual.size()));
        QCOMPARE(actual, tail);
        for (int frame : {0, 511, 512, 513, 0}) {
            QCOMPARE(file->Seek(Int64(frame) << 15, STREAM_SEEK_SET, nullptr), S_OK);
            actual.resize(100);
            QCOMPARE(file->Read(actual.data(), 100, &count), S_OK);
            QCOMPARE(count, 100u);
            QCOMPARE(actual, QByteArray(100, 'A'));
        }
    }
    void wofCorruptionDoesNotPublishPartialCachedOutput() {
        QFETCH(QString, kind);
        QFETCH(int, badOffset);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto file = stream("gamma");
        QVERIFY(file);
        if (badOffset)
            check(file, "gamma", 0, 100);
        char bytes[100];
        UInt32 count = 99;
        for (int i = 0; i < 2; ++i) {
            QCOMPARE(file->Seek(badOffset, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(file->Read(bytes, sizeof(bytes), &count), S_FALSE);
            QCOMPARE(count, 0u);
        }
        if (badOffset)
            check(file, "gamma", 0, 100);
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        const UInt32 id = index("gamma");
        QCOMPARE(archive_->Extract(&id, 1, true, callback), S_OK);
        QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kDataError));
    }
    void openPropagatesMftSeekFailureAndCancellation_data() {
        QTest::addColumn<QString>("kind");
        QTest::newRow("classic") << QString("base");
        QTest::newRow("MFT-list") << QString("mft-list");
    }
    void openPropagatesMftSeekFailureAndCancellation() {
        QFETCH(QString, kind);
        loadFixture(kind);
        auto *source = new Input(bytes_);
        CMyComPtr<IInStream> input = source;
        source->failSeekAt = qFromLittleEndian<quint64>(bytes_.constData() + 0x30) * 4096;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), E_FAIL);
        source->failSeekAt = UINT64_MAX;
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        CMyComPtr<IArchiveOpenCallback> cancelled = new CancelOpen;
        QCOMPARE(archive_->Open(input, nullptr, cancelled), E_ABORT);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 0u);
        QCOMPARE(input->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
    }
    void backendSelectedExtraction_data() {
        QTest::addColumn<QString>("kind");
        QTest::newRow("sparse") << QString("sparse");
        QTest::newRow("compressed") << QString("short-compressed");
        QTest::newRow("corrupt") << QString("run-over-volume");
        QTest::newRow("WOF-XPRESS") << QString("wof-xpress4k");
        QTest::newRow("WSL-EA") << QString("ea-fragmented");
        QTest::newRow("WOF-LZX") << QString("wof-lzx32k");
        QTest::newRow("fragmented-MFT") << QString("mft-list-nonresident");
    }
    void backendSelectedExtraction() {
        QFETCH(QString, kind);
        loadFixture(kind);
        QTemporaryDir runtime;
        QVERIFY(runtime.isValid());
        const QDir prepared(QString::fromUtf8(SEVENZIP_NTFS_RUNTIME));
        const QString executable = QLatin1String("7z") +
#ifdef Q_OS_WIN
                                   ".exe";
        const QString core = "7z.dll";
#else
                                   "";
        const QString core = "7z.so";
#endif
        QVERIFY(QFile::copy(prepared.filePath(executable), runtime.filePath(executable)));
        QVERIFY(QFile::copy(prepared.filePath(core), runtime.filePath(core)));
        QVERIFY(QDir(runtime.path()).mkdir("Formats"));
        const QString module = QString::fromUtf8(SEVENZIP_NTFS_CORE);
        QVERIFY(QFile::copy(module, runtime.filePath("Formats/" + QFileInfo(module).fileName())));
        const QString path = runtime.filePath(QStringLiteral("образ 空.ntfs"));
        QFile source(path);
        QVERIFY(source.open(QIODevice::WriteOnly));
        QCOMPARE(source.write(bytes_), bytes_.size());
        source.close();
        ArchiveBackend backend(nullptr, runtime.filePath(executable));
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(runtime.filePath(core)));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 15000);
        QVERIFY2(ready[0][0].toBool(), qPrintable(ready[0][1].toString()));
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        QVector<ArchiveEntry> entries;
        connect(&backend, &ArchiveBackend::entriesReady, &backend,
                [&](const QVector<ArchiveEntry> &result) { entries = result; });
        backend.list(path);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QVERIFY(!backend.codecCatalog()->canUpdate("NTFS"));
        QVERIFY(std::any_of(entries.cbegin(), entries.cend(), [](const ArchiveEntry &entry) {
            return entry.path == QStringLiteral("данные 空");
        }));
        finished.clear();
        const QString output = runtime.filePath("output");
        const QString name = kind.startsWith("wof") ? "gamma" : "alpha";
        backend.extract(path, output, {name});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool() == (kind != "run-over-volume"),
                 qPrintable(finished[0][1].toString()));
        if (kind != "run-over-volume") {
            QFile file(QDir(output).filePath(name));
            QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(file.readAll(), QByteArray::fromBase64(
                                         record_["files"].toObject()[name].toString().toLatin1()));
        }
        if (kind.startsWith("wof")) {
            for (const QString &entry : {QString("gamma"), QString("gamma:WofCompressedData")}) {
                finished.clear();
                const QString saved =
                    runtime.filePath(entry == "gamma" ? "decoded.bin" : "packed.bin");
                backend.extractSourceEntry(path, entry, saved, "NTFS", {}, entry.contains(':'));
                QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
                QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
                QFile file(saved);
                QVERIFY(file.open(QIODevice::ReadOnly));
                QCOMPARE(file.readAll(),
                         QByteArray::fromBase64(
                             record_["files"].toObject()[entry].toString().toLatin1()));
            }
        }
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), bytes_);
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(NtfsStreamTests)
#include "ntfs_stream_tests.moc"
