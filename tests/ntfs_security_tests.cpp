#include "7zip/Archive/IArchive.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "archivebackend.h"
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
    UInt64 failAt = UINT64_MAX, cancelAt = UINT64_MAX;
    HRESULT failure = E_FAIL;
    bool cancelRequested = false;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (position_ == failAt)
            return failure;
        const UInt64 available =
            position_ < UInt64(bytes_.size()) ? UInt64(bytes_.size()) - position_ : 0;
        size = UInt32(qMin<UInt64>(size, available));
        if (position_ < failAt)
            size = UInt32(qMin<UInt64>(size, failAt - position_));
        if (position_ <= cancelAt && size > cancelAt - position_)
            cancelRequested = true;
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
    Input *source;
    explicit Cancel(Input *value) : source(value) {}
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        return source->cancelRequested ? E_ABORT : S_OK;
    }
};
QString runtimeFile(const char *name) {
    return QDir(QString::fromUtf8(SEVENZIP_NTFS_RUNTIME)).filePath(QString::fromLatin1(name));
}
} // namespace

class NtfsSecurityTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_NTFS_MODULE)};
    CMyComPtr<IInArchive> archive_;
    QByteArray image_, expected_;

    void fixture(const QString &kind) {
        QProcess process;
        process.start(QString::fromUtf8(SEVENZIP_NTFS_PYTHON),
                      {QString::fromUtf8(SEVENZIP_NTFS_FIXTURE), "--kind=" + kind});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        const auto object = QJsonDocument::fromJson(process.readAllStandardOutput()).object();
        QVERIFY(!object.isEmpty());
        QByteArray packed(4, '\0');
        qToBigEndian(quint32(object["size"].toInt()), packed.data());
        packed += QByteArray::fromBase64(object["image"].toString().toLatin1());
        image_ = qUncompress(packed);
        QVERIFY(!image_.isEmpty());
        expected_ = QByteArray::fromHex(object["security"].toString().toLatin1());
    }
    UInt32 index() {
        UInt32 count = 0;
        if (archive_->GetNumberOfItems(&count) != S_OK)
            return UINT32_MAX;
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant value;
            if (archive_->GetProperty(i, kpidPath, &value) == S_OK && value.vt == VT_BSTR &&
                QDir::fromNativeSeparators(QString::fromWCharArray(value.bstrVal))
                        .section('/', -1) == "gamma")
                return i;
        }
        return UINT32_MAX;
    }
    void descriptor() {
        CMyComPtr<IArchiveGetRawProps> raw;
        QCOMPARE(archive_->QueryInterface(IID_IArchiveGetRawProps, reinterpret_cast<void **>(&raw)),
                 S_OK);
        const void *bytes = nullptr;
        UInt32 size = 99, type = 99;
        QCOMPARE(raw->GetRawProp(index(), kpidNtSecure, &bytes, &size, &type), S_OK);
        QCOMPARE(size, UInt32(expected_.size()));
        if (expected_.isEmpty()) {
            QVERIFY(!bytes);
            QCOMPARE(type, UInt32(0));
        } else {
            QCOMPARE(type, UInt32(NPropDataType::kRaw));
            QVERIFY(bytes);
            QCOMPARE(QByteArray(static_cast<const char *>(bytes), size), expected_);
        }
    }
    CMyComPtr<ISequentialInStream> dataStream() {
        CMyComPtr<IInArchiveGetStream> provider;
        if (archive_->QueryInterface(IID_IInArchiveGetStream,
                                     reinterpret_cast<void **>(&provider)) != S_OK)
            return {};
        CMyComPtr<ISequentialInStream> result;
        if (provider->GetStream(index(), &result) != S_OK)
            return {};
        return result;
    }
    void payload(ISequentialInStream *stream) {
        QVERIFY(stream);
        QByteArray actual;
        char bytes[1024];
        for (;;) {
            UInt32 size = 0;
            QCOMPARE(stream->Read(bytes, sizeof(bytes), &size), S_OK);
            if (!size)
                break;
            actual.append(bytes, size);
            QVERIFY(actual.size() <= 65536);
        }
        QCOMPARE(actual, QByteArray(65536, 'B'));
    }
  private slots:
    void initTestCase() {
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
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "NTFS")
                continue;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &id), S_OK);
            GUID guid;
            memcpy(&guid, id.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared module has no NTFS reader");
    }
    void indexedDescriptors_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"security-index",
                                 "security-physical-order",
                                 "security-mirror",
                                 "security-partial-sds",
                                 "security-tree",
                                 "security-root-key",
                                 "security-many",
                                 "security-small-block",
                                 "security-fragmented-index",
                                 "security-extension",
                                 "security-sparse-sds",
                                 "security-empty",
                                 "security-reserved",
                                 "security-no-metadata",
                                 "security-fixup-mismatch",
                                 "security-deleted-corrupt",
                                 "standard-full",
                                 "standard-legacy",
                                 "standard-partial56",
                                 "standard-partial64",
                                 "standard-duplicate",
                                 "standard-extension",
                                 "standard-deleted-short"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void indexedDescriptors() {
        QFETCH(QString, kind);
        fixture(kind);
        CMyComPtr<ISetProperties> properties;
        QCOMPARE(
            archive_->QueryInterface(IID_ISetProperties, reinterpret_cast<void **>(&properties)),
            S_OK);
        const wchar_t *name = L"ld";
        const NWindows::NCOM::CPropVariant showDeleted(kind.contains("deleted"));
        QCOMPARE(properties->SetProperties(&name, &showDeleted, 1), S_OK);
        CMyComPtr<IInStream> source = new Input(image_);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        QVERIFY(index() != UINT32_MAX);
        if (kind.startsWith("standard-") && !kind.contains("deleted")) {
            NWindows::NCOM::CPropVariant time, attributes;
            QCOMPARE(archive_->GetProperty(index(), kpidMTime, &time), S_OK);
            QCOMPARE(time.vt, VARTYPE(VT_FILETIME));
            QCOMPARE((UInt64(time.filetime.dwHighDateTime) << 32) | time.filetime.dwLowDateTime,
                     UInt64(132537600000000000));
            QCOMPARE(archive_->GetProperty(index(), kpidAttrib, &attributes), S_OK);
            QCOMPARE(attributes.vt, VARTYPE(VT_UI4));
            QVERIFY(attributes.ulVal & 1);
        }
        descriptor();
        payload(dataStream());
        descriptor();
        auto retained = dataStream();
        QCOMPARE(archive_->Close(), S_OK);
        source.Release();
        payload(retained);
    }
    void malformedDescriptors_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"security-root-offset",
                                 "security-root-used",
                                 "security-root-terminal",
                                 "security-key-overrun",
                                 "security-subnode-flag",
                                 "security-view-short",
                                 "security-header-mismatch",
                                 "security-sds-range",
                                 "security-sd-short",
                                 "security-sid-overflow",
                                 "security-sid-length",
                                 "security-acl-revision",
                                 "security-ace-overrun",
                                 "security-node-vbn",
                                 "security-node-fixup-range",
                                 "security-node-key-overrun",
                                 "security-index-range",
                                 "security-cycle",
                                 "standard-short",
                                 "standard-nonresident",
                                 "standard-missing",
                                 "standard-bad-second",
                                 "standard-record-total"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void malformedDescriptors() {
        QFETCH(QString, kind);
        fixture(kind);
        CMyComPtr<IInStream> source = new Input(image_);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_FALSE);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        fixture("security-tree");
        source = new Input(image_);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        descriptor();
    }
    void ioCancellationAndRecovery() {
        fixture("security-fragmented-index");
        CMyComPtr2<IInStream, Input> source;
        source.SetFromCls(new Input(image_));
        source->failAt = 1202 * 4096;
        for (HRESULT result : {E_FAIL, E_ABORT}) {
            source->failure = result;
            QCOMPARE(archive_->Open(source, nullptr, nullptr), result);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        source->failAt = UINT64_MAX;
        source->cancelAt = 1202 * 4096;
        CMyComPtr<IArchiveOpenCallback> callback = new Cancel(source.ClsPtr());
        QCOMPARE(archive_->Open(source, nullptr, callback), E_ABORT);
        QVERIFY(source->cancelRequested);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        source->cancelAt = UINT64_MAX;
        source->cancelRequested = false;
        QCOMPARE(archive_->Open(source, nullptr, callback), S_OK);
        descriptor();
    }
    void backendExtraction() {
        fixture("security-tree");
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath(QString::fromUtf8("образ 空.ntfs"));
        QFile image(path);
        QVERIFY(image.open(QIODevice::WriteOnly));
        QCOMPARE(image.write(image_), image_.size());
        image.close();
#ifdef Q_OS_WIN
        const QString engine = runtimeFile("7z.exe"), core = runtimeFile("7z.dll");
#else
        const QString engine = runtimeFile("7z"), core = runtimeFile("7z.so");
#endif
        ArchiveBackend backend(nullptr, engine);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(core));
        QTRY_COMPARE_WITH_TIMEOUT(ready.size(), 1, 10000);
        QVERIFY(ready[0][0].toBool());
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list(path);
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        finished.clear();
        const QString saved = temporary.filePath("payload.bin");
        backend.extractSourceEntry(path, "gamma", saved, "NTFS", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile result(saved);
        QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(), QByteArray(65536, 'B'));
        QVERIFY(image.open(QIODevice::ReadOnly));
        QCOMPARE(image.readAll(), image_);
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(NtfsSecurityTests)
#include "ntfs_security_tests.moc"
