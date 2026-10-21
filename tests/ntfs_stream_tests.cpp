#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "archivebackend.h"
#include "archivecodeccatalog.h"
#include <QDir>
#include <QFile>
#include <QHash>
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
    UInt64 cancelAt = UINT64_MAX;
    bool cancelRequested = false;
    HRESULT failedSeekResult = E_FAIL;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (position_ <= cancelAt && cancelAt - position_ < size)
            cancelRequested = true;
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
    const Input *source = nullptr;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        return source && source->cancelRequested ? E_ABORT : S_OK;
    }
};
} // namespace

class NtfsStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_NTFS_CORE)};
    CMyComPtr<IInArchive> archive_;
    QJsonObject record_;
    QByteArray bytes_;
    QHash<QString, QJsonObject> fixtures_;

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
        if (!fixtures_.contains(codec)) {
            QProcess generator;
            QStringList arguments{QString::fromUtf8(SEVENZIP_NTFS_FIXTURE), "--kind=" + codec};
            generator.start(QString::fromUtf8(SEVENZIP_NTFS_PYTHON), arguments);
            QVERIFY(generator.waitForFinished(10000));
            QCOMPARE(generator.exitCode(), 0);
            const auto record = QJsonDocument::fromJson(generator.readAllStandardOutput()).object();
            QVERIFY(!record.isEmpty());
            fixtures_.insert(codec, record);
        }
        record_ = fixtures_.value(codec);
        QVERIFY(!record_.isEmpty());
        QByteArray packed(4, '\0');
        qToBigEndian(quint32(record_["size"].toInt()), packed.data());
        packed += QByteArray::fromBase64(record_["image"].toString().toLatin1());
        bytes_ = qUncompress(packed);
        QVERIFY(!bytes_.isEmpty());
    }
  private slots:
    void initTestCase() { createArchive(); }
    void volumeStateAndOpeningLifetime() {
        for (const char *kind : {"volume-dirty",
                                 "volume-clean",
                                 "volume-short",
                                 "volume-missing",
                                 "volume-named",
                                 "volume-compressed",
                                 "volume-resize",
                                 "volume-duplicate",
                                 "volume-list-clean",
                                 "volume-list-dirty",
                                 "volume-list-nonresident",
                                 "volume-list-base",
                                 "volume-list-override",
                                 "volume-list-bad-id",
                                 "volume-list-bad-sequence",
                                 "volume-list-wrong-owner",
                                 "volume-list-missing",
                                 "volume-list-bad-list",
                                 "volume-list-bad-run",
                                 "volume-clean"}) {
            loadFixture(kind);
            CMyComPtr<IInStream> input = new Input(bytes_);
            QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
            NWindows::NCOM::CPropVariant readOnly;
            QCOMPARE(archive_->GetArchiveProperty(kpidReadOnly, &readOnly), S_OK);
            QCOMPARE(readOnly.vt, VARTYPE(VT_BOOL));
            QVERIFY(readOnly.boolVal != VARIANT_FALSE);
            UInt32 count = 0;
            QCOMPARE(archive_->GetNumberOfArchiveProperties(&count), S_OK);
            bool found = false;
            for (UInt32 i = 0; i < count; ++i) {
                BSTR name = nullptr;
                PROPID id;
                VARTYPE type;
                QCOMPARE(archive_->GetArchivePropertyInfo(i, &name, &id, &type), S_OK);
                const QString label = name ? QString::fromWCharArray(name) : QString();
                SysFreeString(name);
                if (label != "Read-only limitations")
                    continue;
                found = true;
                NWindows::NCOM::CPropVariant state;
                QCOMPARE(archive_->GetArchiveProperty(id, &state), S_OK);
                const QString expected = record_["volume_limitation"].toString();
                if (expected.isEmpty())
                    QCOMPARE(state.vt, VARTYPE(VT_EMPTY));
                else {
                    QCOMPARE(state.vt, VARTYPE(VT_BSTR));
                    QCOMPARE(QString::fromWCharArray(state.bstrVal), expected);
                }
            }
            QVERIFY(found);
            if (record_.contains("volume_label")) {
                NWindows::NCOM::CPropVariant label;
                QCOMPARE(archive_->GetArchiveProperty(kpidVolumeName, &label), S_OK);
                const QString expected = record_["volume_label"].toString();
                if (expected.isEmpty())
                    QCOMPARE(label.vt, VARTYPE(VT_EMPTY));
                else {
                    QCOMPARE(label.vt, VARTYPE(VT_BSTR));
                    QCOMPARE(QString::fromWCharArray(label.bstrVal), expected);
                }
            }
            auto retained = stream("alpha");
            QVERIFY(retained);
            check(retained, "alpha", 0, 4096);
            QCOMPARE(archive_->Close(), S_OK);
            check(retained, "alpha", 4096, 1024);
        }
    }
    void volumeListFailureAndCancellation() {
        loadFixture("volume-list-nonresident");
        for (HRESULT failure : {E_FAIL, E_ABORT}) {
            CMyComPtr2<IInStream, Input> source;
            source.SetFromCls(new Input(bytes_));
            source->failSeekAt = 1100 * 4096;
            source->failedSeekResult = failure;
            QCOMPARE(archive_->Open(source, nullptr, nullptr), failure == E_ABORT ? E_ABORT : S_OK);
            if (failure == E_FAIL) {
                NWindows::NCOM::CPropVariant label;
                QCOMPARE(archive_->GetArchiveProperty(kpidVolumeName, &label), S_OK);
                QCOMPARE(label.vt, VARTYPE(VT_EMPTY));
                auto readable = stream("alpha");
                QVERIFY(readable);
                check(readable, "alpha", 0, 4096);
            } else {
                UInt32 count = 99;
                QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
                QCOMPARE(count, 0u);
            }
            source->failSeekAt = UINT64_MAX;
            QCOMPARE(source->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
            NWindows::NCOM::CPropVariant label;
            QCOMPARE(archive_->GetArchiveProperty(kpidVolumeName, &label), S_OK);
            QCOMPARE(label.vt, VARTYPE(VT_BSTR));
            QCOMPARE(QString::fromWCharArray(label.bstrVal), record_["volume_label"].toString());
            QCOMPARE(archive_->Close(), S_OK);
        }
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(bytes_));
        source->cancelAt = 1100 * 4096;
        CMyComPtr2<IArchiveOpenCallback, CancelEaOpen> callback;
        callback.SetFromCls(new CancelEaOpen);
        callback->source = source.ClsPtr();
        QCOMPARE(archive_->Open(source, nullptr, callback), E_ABORT);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, 0u);
    }
    void volumeFreeSpace_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"bitmap-base", "bitmap-padding", "bitmap-last", "bitmap-full",
                                 "bitmap-fragmented", "bitmap-resident", "bitmap-short",
                                 "bitmap-uninitialized", "bitmap-bad-run", "bitmap-hole"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void volumeFreeSpace() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        NWindows::NCOM::CPropVariant value;
        const bool available = !record_["free_space"].isNull();
        QCOMPARE(archive_->GetArchiveProperty(kpidFreeSpace, &value), available ? S_OK : S_FALSE);
        if (available) {
            QCOMPARE(value.vt, VARTYPE(VT_UI8));
            QCOMPARE(value.uhVal.QuadPart, UInt64(record_["free_space"].toDouble()));
        }
        // Unavailable allocation metadata must not block ordinary file recovery.
        auto retained = stream("alpha");
        QVERIFY(retained);
        check(retained, "alpha", 0, 200000);
        QCOMPARE(archive_->Close(), S_OK);
        QCOMPARE(archive_->GetArchiveProperty(kpidFreeSpace, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_EMPTY));
        check(retained, "alpha", 0, 200000);
    }
    void volumeFreeSpaceReadFailures() {
        loadFixture("bitmap-base");
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(bytes_));
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        NWindows::NCOM::CPropVariant value;
        for (HRESULT failure : {E_FAIL, E_ABORT}) {
            source->failure = failure;
            QCOMPARE(archive_->GetArchiveProperty(kpidFreeSpace, &value), failure);
        }
        source->failure = S_OK;
        QCOMPARE(archive_->GetArchiveProperty(kpidFreeSpace, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_UI8));
        QCOMPARE(value.uhVal.QuadPart, UInt64(record_["free_space"].toDouble()));
        QCOMPARE(archive_->Close(), S_OK);
    }
    void dataAttributeFlags_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"flags-clear", "flags-sparse", "flags-compressed", "flags-encrypted",
              "flags-resident", "flags-ads", "flags-directory", "flags-badclus", "flags-secure",
              "flags-extension-sparse", "flags-extension-plain", "flags-ads-sparse",
              "flags-ads-resident", "flags-ads-encrypted"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void dataAttributeFlags() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const auto expected = record_["attributes"].toObject();
        const QString path = expected["path"].toString();
        const UInt32 id = index(path);
        QVERIFY(id != UINT32_MAX);
        NWindows::NCOM::CPropVariant value;
        QCOMPARE(archive_->GetProperty(id, kpidAttrib, &value), S_OK);
        QCOMPARE(value.vt, VARTYPE(VT_UI4));
        QCOMPARE(value.ulVal, UInt32(expected["value"].toInt()));
        if (expected.contains("base_value")) {
            QCOMPARE(archive_->GetProperty(index("alpha"), kpidAttrib, &value), S_OK);
            QCOMPARE(value.vt, VARTYPE(VT_UI4));
            QCOMPARE(value.ulVal, UInt32(expected["base_value"].toInt()));
        }
        CMyComPtr<IInStream> retained;
        if (record_["files"].toObject().contains(path)) {
            retained = stream(path);
            QVERIFY(retained);
            check(retained, path, 0, 200000);
        }
        if (kind == "flags-ads-encrypted") {
            CMyComPtr2<IArchiveExtractCallback, Extract> callback;
            callback.Create_if_Empty();
            QCOMPARE(archive_->Extract(&id, 1, true, callback), S_OK);
            QCOMPARE(callback->result,
                     Int32(NArchive::NExtract::NOperationResult::kUnsupportedMethod));
        }
        QCOMPARE(archive_->Close(), S_OK);
        if (retained)
            check(retained, path, 0, 200000);
    }
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
        callback->source = source.ClsPtr();
        source->cancelAt = 1102 * 4096;
        QCOMPARE(archive_->Open(source, nullptr, callback), E_ABORT);
        source->cancelAt = UINT64_MAX;
        source->cancelRequested = false;
        QCOMPARE(archive_->Open(source, nullptr, callback), S_OK);
        check(stream("gamma"), "gamma", 0, 200000);
    }
    void eaValuesRetainTheirOwner_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"ea-xattrs", "ea-xattrs-fragmented", "ea-xattrs-extension", "ea-xattrs-collision"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void eaValuesRetainTheirOwner() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const QString name =
            kind == "ea-xattrs-collision" ? "gamma:$EA.user.comment#1" : "gamma:$EA.user.comment";
        auto value = stream(name);
        auto empty = stream("gamma:$EA.user.empty");
        QVERIFY(value && empty);
        const QByteArray expected =
            QByteArray::fromBase64(record_["files"].toObject()[name].toString().toLatin1());
        QCOMPARE(archive_->Close(), S_OK);
        input.Release();
        archive_.Release();
        char buffer[128];
        UInt32 bytes = 99;
        QCOMPARE(empty->Read(buffer, sizeof(buffer), &bytes), S_OK);
        QCOMPARE(bytes, UInt32(0));
        QCOMPARE(value->Read(buffer, sizeof(buffer), &bytes), S_OK);
        QCOMPARE(QByteArray(buffer, int(bytes)), expected);
        createArchive();
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
                                 "wof-xpress4k-unknown-uninitialized",
                                 "wof-xpress4k-uninitialized"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void reparseLinks_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"link-symlink",       "link-print-name",  "link-unicode",    "link-surrogate",
              "link-nul",           "link-backslashes", "link-junction",   "link-junction-lost",
              "link-nonresident",   "link-fragmented",  "link-extension",  "link-directory",
              "link-readonly",      "link-ads",         "link-user",       "link-named-reparse",
              "link-odd-offset",    "link-cloud",       "link-bad-offset", "link-bad-length",
              "link-no-print-name", "link-unknown-ms",  "link-user-short", "link-junction-no-drive",
              "link-dedup"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void reparseLinks() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const QString name = kind == "link-junction-lost" ? "[LOST]/gamma" : "gamma";
        const UInt32 id = index(name);
        QVERIFY(id != UINT32_MAX);
        const auto files = record_["files"].toObject();
        const bool supported = files.contains(name), regular = kind == "link-named-reparse";
        NWindows::NCOM::CPropVariant link, mode, directory, size;
        QCOMPARE(archive_->GetProperty(id, kpidSymLink, &link), S_OK);
        QCOMPARE(archive_->GetProperty(id, kpidPosixAttrib, &mode), S_OK);
        QCOMPARE(archive_->GetProperty(id, kpidIsDir, &directory), S_OK);
        QCOMPARE(directory.vt, VARTYPE(VT_BOOL));
        QCOMPARE(directory.boolVal, VARIANT_FALSE);
        if (regular) {
            QCOMPARE(link.vt, VARTYPE(VT_EMPTY));
            QCOMPARE(mode.vt, VARTYPE(VT_EMPTY));
        } else if (kind == "link-dedup") {
            QCOMPARE(mode.vt, VARTYPE(VT_EMPTY));
        } else {
            QCOMPARE(mode.vt, VARTYPE(VT_UI4));
            QCOMPARE(mode.ulVal, ULONG(kind == "link-readonly" ? 0120555
                                       : kind == "link-cloud"  ? 0120751
                                                               : 0120777));
        }
        const QByteArray expected = QByteArray::fromBase64(files[name].toString().toLatin1());
        if (supported && !regular) {
            QCOMPARE(link.vt, VARTYPE(VT_BSTR));
            QCOMPARE(QString::fromWCharArray(link.bstrVal), QString::fromUtf8(expected));
            QCOMPARE(archive_->GetProperty(id, kpidSize, &size), S_OK);
            QCOMPARE(size.vt, VARTYPE(VT_UI8));
            QCOMPARE(size.uhVal.QuadPart, UInt64(expected.size()));
        } else if (!supported) {
            QCOMPARE(link.vt, VARTYPE(VT_EMPTY));
            CMyComPtr<IInArchiveGetStream> provider;
            QCOMPARE(archive_->QueryInterface(IID_IInArchiveGetStream,
                                              reinterpret_cast<void **>(&provider)),
                     S_OK);
            CMyComPtr<ISequentialInStream> data;
            QCOMPARE(provider->GetStream(id, &data), E_NOTIMPL);
            QVERIFY(!data);
        }
        auto retained = supported ? stream(name) : CMyComPtr<IInStream>();
        if (kind == "link-ads") {
            const UInt32 adsId = index("gamma:side");
            QVERIFY(adsId != UINT32_MAX);
            QCOMPARE(archive_->GetProperty(adsId, kpidSymLink, &link), S_OK);
            QCOMPARE(link.vt, VARTYPE(VT_EMPTY));
            CMyComPtr<IArchiveGetRawProps> raw;
            QCOMPARE(
                archive_->QueryInterface(IID_IArchiveGetRawProps, reinterpret_cast<void **>(&raw)),
                S_OK);
            const void *data = nullptr;
            UInt32 bytes = 0, type = 0;
            QCOMPARE(raw->GetRawProp(adsId, kpidNtReparse, &data, &bytes, &type), S_OK);
            QCOMPARE(bytes, UInt32(0));
            check(stream("gamma:side"), "gamma:side", 0, 200000);
        }
        QCOMPARE(archive_->Close(), S_OK);
        input.Release();
        if (supported) {
            QVERIFY(retained);
            check(retained, name, 0, 200000);
        }
    }
    void wofTailBeforePayloadErrors_data() {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<HRESULT>("failure");
        for (const char *kind :
             {"wof-xpress4k-unknown-initialized", "wof-xpress4k-unknown-high-initialized",
              "wof-xpress4k-unknown-max-initialized", "wof-lzx32k-initialized-encrypted-ads"})
            QTest::newRow(kind) << QString::fromLatin1(kind) << HRESULT(E_NOTIMPL);
        for (const char *kind :
             {"wof-xpress4k-initialized-missing-ads", "wof-lzx32k-initialized-missing-ads",
              "wof-xpress4k-initialized-bad-map"})
            QTest::newRow(kind) << QString::fromLatin1(kind) << HRESULT(S_FALSE);
        for (const char *kind : {"wof-xpress4k-initialized", "wof-lzx32k-initialized"})
            QTest::newRow(kind) << QString::fromLatin1(kind) << HRESULT(E_FAIL);
        QTest::newRow("source-abort") << QString("wof-xpress4k-initialized") << HRESULT(E_ABORT);
    }
    void wofTailBeforePayloadErrors() {
        QFETCH(QString, kind);
        QFETCH(HRESULT, failure);
        loadFixture(kind);
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        const bool sourceFailure = failure == E_FAIL || failure == E_ABORT;
        if (sourceFailure)
            input->failure = failure;
        auto file = stream("gamma");
        QVERIFY(file);
        const UInt64 valid = (UInt64(1) << (kind.contains("unknown")     ? 16
                                            : kind.startsWith("wof-lzx") ? 15
                                                                         : 12)) +
                             37;
        const UInt64 tail = (valid + 4095) & ~UInt64(4095);
        const auto zeros = [&] {
            QCOMPARE(file->Seek(tail, STREAM_SEEK_SET, nullptr), S_OK);
            char data[97];
            memset(data, 'X', sizeof(data));
            UInt32 processed = 0;
            QCOMPARE(file->Read(data, sizeof(data), &processed), S_OK);
            QCOMPARE(processed, UInt32(sizeof(data)));
            QCOMPARE(QByteArray(data, sizeof(data)), QByteArray(sizeof(data), '\0'));
        };
        zeros();
        QCOMPARE(file->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        char byte = 0;
        UInt32 processed = 99;
        QCOMPARE(file->Read(&byte, 1, &processed), failure);
        QCOMPARE(processed, UInt32(0));
        zeros();
        const UInt32 id = index("gamma");
        if (sourceFailure) {
            CMyComPtr2<IArchiveExtractCallback, Extract> callback;
            callback.Create_if_Empty();
            QCOMPARE(archive_->Extract(&id, 1, true, callback), failure);
        }
        input->failure = S_OK;
        if (sourceFailure) {
            QCOMPARE(file->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
            QCOMPARE(file->Read(&byte, 1, &processed), S_OK);
            QCOMPARE(processed, UInt32(1));
            QCOMPARE(byte, 'A');
        }
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.Create_if_Empty();
        QCOMPARE(archive_->Extract(&id, 1, true, callback), S_OK);
        QCOMPARE(callback->result,
                 Int32(sourceFailure ? NArchive::NExtract::NOperationResult::kOK
                       : failure == E_NOTIMPL
                           ? NArchive::NExtract::NOperationResult::kUnsupportedMethod
                           : NArchive::NExtract::NOperationResult::kDataError));
        QCOMPARE(archive_->Close(), S_OK);
        input.SetFromCls(nullptr);
        zeros();
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
        QTest::newRow("WOF-missing-ADS") << QString("wof") << HRESULT(S_OK);
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
        if (kind == "bad-compressed" || kind == "wof") {
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
        QTest::newRow("dirty-volume") << QString("volume-dirty");
        QTest::newRow("volume-extension-metadata") << QString("volume-list-nonresident");
        QTest::newRow("unknown-volume-state") << QString("volume-short");
        QTest::newRow("sparse") << QString("sparse");
        QTest::newRow("compressed") << QString("short-compressed");
        QTest::newRow("corrupt") << QString("run-over-volume");
        QTest::newRow("WOF-XPRESS") << QString("wof-xpress4k");
        QTest::newRow("WSL-EA") << QString("ea-fragmented");
        QTest::newRow("EA-values") << QString("ea-xattrs-collision");
        QTest::newRow("WOF-LZX") << QString("wof-lzx32k");
        QTest::newRow("fragmented-MFT") << QString("mft-list-nonresident");
        QTest::newRow("Unicode-link") << QString("link-unicode");
        QTest::newRow("junction") << QString("link-junction");
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
        if (kind == "ea-xattrs-collision") {
            finished.clear();
            const QString saved = runtime.filePath("ea-value.bin");
            const QString entry = "gamma:$EA.user.comment#1";
            backend.extractSourceEntry(path, entry, saved, "NTFS", {}, true);
            QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
            QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
            QFile file(saved);
            QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(file.readAll(), QByteArray::fromBase64(
                                         record_["files"].toObject()[entry].toString().toLatin1()));
        }
        if (kind.startsWith("link-")) {
            QVERIFY(std::any_of(entries.cbegin(), entries.cend(), [](const ArchiveEntry &entry) {
                return entry.path == "gamma" && entry.link && !entry.directory;
            }));
            finished.clear();
            const QString saved = runtime.filePath("link-target.txt");
            backend.extractSourceEntry(path, "gamma", saved, "NTFS", {});
            QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
            QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
            QFile file(saved);
            QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(
                file.readAll(),
                QByteArray::fromBase64(record_["files"].toObject()["gamma"].toString().toLatin1()));
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
