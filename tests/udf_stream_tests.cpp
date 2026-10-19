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
#include <QtTest>

namespace {
class Input final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    QByteArray bytes_;
    UInt64 position_ = 0;

  public:
    HRESULT failure = S_OK;
    HRESULT readFailure = S_OK;
    HRESULT seekFailure = E_FAIL;
    UInt64 failSeekAt = UINT64_MAX;
    UInt64 lastSeek = 0;
    explicit Input(QByteArray bytes) : bytes_(std::move(bytes)) {}
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (failure != S_OK)
            return failure;
        if (readFailure != S_OK)
            return readFailure;
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
        lastSeek = position_;
        if (position)
            *position = position_;
        return S_OK;
    }
};

class Extract final : public IArchiveExtractCallback, public CMyUnknownImp {
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
class CancelOpen final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override { return E_ABORT; }
};
class CancelAt final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
    Input *input_;
    UInt64 checkpoint_;
    HRESULT result_;
    bool fired_ = false;

  public:
    CancelAt(Input *input, UInt64 checkpoint, HRESULT result = E_ABORT)
        : input_(input), checkpoint_(checkpoint), result_(result) {}
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        if (!fired_ && input_->lastSeek >= checkpoint_) {
            fired_ = true;
            return result_;
        }
        return S_OK;
    }
};
class CancelOnce final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
    bool consumed_ = false;

  public:
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        if (consumed_)
            return S_OK;
        consumed_ = true;
        return E_ABORT;
    }
};
class OpenProgress final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    bool closed = false;
    unsigned calls = 0;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        ++calls;
        return closed ? E_FAIL : S_OK;
    }
};
} // namespace

class UdfStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_UDF_CORE)};
    CMyComPtr<IInArchive> archive_;
    QJsonObject files_;
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
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "Udf")
                continue;
            NWindows::NCOM::CPropVariant clsid;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
            GUID guid;
            memcpy(&guid, clsid.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no UDF handler");
    }
    void fixture(const QString &kind) {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath("volume.udf");
        QProcess process;
        process.start(QString::fromUtf8(SEVENZIP_UDF_PYTHON),
                      {QString::fromUtf8(SEVENZIP_UDF_FIXTURE), kind, path});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        files_ = QJsonDocument::fromJson(process.readAllStandardOutput()).object();
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        bytes_ = file.readAll();
        QVERIFY(!bytes_.isEmpty());
    }
    CMyComPtr<IInStream> stream(const QString &name) {
        CMyComPtr<IInArchiveGetStream> provider;
        if (archive_->QueryInterface(IID_IInArchiveGetStream,
                                     reinterpret_cast<void **>(&provider)) != S_OK)
            return {};
        UInt32 count = 0;
        if (archive_->GetNumberOfItems(&count) != S_OK)
            return {};
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant path;
            if (archive_->GetProperty(i, kpidPath, &path) != S_OK || path.vt != VT_BSTR ||
                QString::fromWCharArray(path.bstrVal).replace('\\', '/') != name)
                continue;
            CMyComPtr<ISequentialInStream> sequential;
            if (provider->GetStream(i, &sequential) != S_OK || !sequential)
                return {};
            CMyComPtr<IInStream> result;
            sequential->QueryInterface(IID_IInStream, reinterpret_cast<void **>(&result));
            return result;
        }
        return {};
    }
    void check(IInStream *stream, const QByteArray &expected) {
        QVERIFY(stream);
        QCOMPARE(stream->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QByteArray actual;
        for (;;) {
            char data[37];
            UInt32 count = 0;
            QCOMPARE(stream->Read(data, sizeof(data), &count), S_OK);
            if (!count)
                break;
            actual.append(data, count);
            QVERIFY(actual.size() <= expected.size());
        }
        QCOMPARE(actual, expected);
        UInt64 end = 0;
        QCOMPARE(stream->Seek(0, STREAM_SEEK_END, &end), S_OK);
        QCOMPARE(end, UInt64(expected.size()));
    }

  private slots:
    void init() { createArchive(); }
    void allocationStreams_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"native", "sparse-short", "sparse-long", "hole-only", "allocated-hole", "eof-short",
              "eof-long", "unused-tail", "inline", "directory-padded", "directory-inline",
              "metadata-bootstrap", "implicit-tail", "implicit-hole", "eof-partition-tail",
              "terminator-short", "terminator-long"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
        for (const char *kind :
             {"ad-short", "ad-long", "ad-chain-short", "ad-chain-long", "ad-limit-16",
              "ad-directory", "ad-cross-partition", "ad-zero", "ad-implicit-tail"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
        for (const char *kind : {"icb-file",
                                 "icb-efe",
                                 "icb-inline",
                                 "icb-root",
                                 "icb-directory",
                                 "icb-chain",
                                 "icb-limit-1024",
                                 "icb-cross-partition",
                                 "icb-ad-continuation",
                                 "icb-metadata",
                                 "icb-metadata-cross",
                                 "icb-no-ie",
                                 "icb-terminal",
                                 "icb-empty-ie",
                                 "icb-bad-ie-crc",
                                 "icb-bad-ie-location",
                                 "icb-reserved",
                                 "icb-extent-short",
                                 "icb-extent-long",
                                 "icb-root-extent-zero"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
        for (const char *kind :
             {"sp-packet", "sp-cross", "sp-reverse", "sp-backup", "sp-first-wins", "sp-no-table",
              "sp-invalid-table", "sp-table-overrun", "sp-max-table", "sp-declared-zero",
              "sp-reserved", "sp-sparse", "sp-icb", "sp-directory-cross", "sp-metadata"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
        for (const char *kind :
             {"vat15-inline", "vat20-inline", "vat15-external", "vat20-external",
              "vat20-extended-header", "vat20-previous", "vat15-offset", "vat20-sparse",
              "vat20-unknown-newest", "vat20-cross-partition", "vat20-sparable"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
        for (const char *kind :
             {"walk-zero-short", "walk-zero-long", "walk-zero-many", "walk-zero-chain-short",
              "walk-zero-chain-long", "walk-active-eof", "walk-aed-tags", "walk-tail-short",
              "walk-tail-long", "walk-cross-partition", "walk-metadata-zero",
              "walk-metadata-zero-chain", "walk-metadata-cycle-mirror"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void metadataStreams_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"md-fragmented", "md-mirror-bootstrap", "md-mirror-lazy", "md-primary-only",
              "md-bitmap-missing", "md-bitmap-present", "md-short-size", "md-ad-chain", "md-offset",
              "md-sparable", "md-long-primary", "md-efe", "md-attributes", "metadata-hole",
              "metadata-outside"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void metadataStreams() { allocationStreams(); }
    void discoveryStreams_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"ds-block-512",
                                 "ds-block-1024",
                                 "ds-block-2048",
                                 "ds-block-4096",
                                 "ds-block-4096-spaced",
                                 "ds-lvd-size",
                                 "ds-backup-last",
                                 "ds-backup-middle",
                                 "ds-bad-primary",
                                 "ds-open-512",
                                 "ds-last-minus-one",
                                 "ds-last-minus-two",
                                 "ds-last-minus-150",
                                 "ds-last-minus-152",
                                 "ds-main-reserve",
                                 "ds-no-terminator",
                                 "ds-stop-invalid",
                                 "ds-vdp",
                                 "ds-vdp-64",
                                 "ds-prevailing-new",
                                 "ds-prevailing-tie",
                                 "ds-prevailing-older",
                                 "ds-prevailing-lvd",
                                 "ds-partitions-growth",
                                 "ds-fsd-zero",
                                 "ds-fsd-large",
                                 "ds-nsr03",
                                 "ds-direct-nsr",
                                 "ds-vrs-type-version",
                                 "ds-lvid-max",
                                 "ds-lvid-chain",
                                 "ds-lvid-missing",
                                 "ds-lvid-cycle",
                                 "ds-lvid-invalid-last",
                                 "ds-lvid-short-use",
                                 "ds-lvid-partial"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void discoveryStreams() { allocationStreams(); }
    void allocationStreams() {
        QFETCH(QString, kind);
        fixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto alpha = stream("alpha.bin");
        auto beta = stream("nested/tail.bin");
        auto empty = stream("empty");
        QVERIFY(alpha && beta && empty);
        const QByteArray expected = QByteArray::fromHex(files_["alpha.bin"].toString().toLatin1());
        check(alpha, expected);
        check(beta, QByteArray("directory payload"));
        char first[17];
        UInt32 count = 0;
        QCOMPARE(alpha->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(alpha->Read(first, sizeof(first), &count), S_OK);
        QCOMPARE(QByteArray(first, count), expected.left(17));
        check(beta, QByteArray("directory payload"));
        char next[17];
        QCOMPARE(alpha->Read(next, sizeof(next), &count), S_OK);
        QCOMPARE(QByteArray(next, count), expected.mid(17, 17));
        check(empty, {});
        QCOMPARE(archive_->Close(), S_OK);
        fixture("native");
        CMyComPtr<IInStream> replacement = new Input(bytes_);
        QCOMPARE(archive_->Open(replacement, nullptr, nullptr), S_OK);
        archive_.Release();
        check(alpha, expected);
        check(beta, QByteArray("directory payload"));
    }
    void failuresAndCancellation() {
        fixture("sparse-long");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto alpha = stream("alpha.bin");
        QVERIFY(alpha);
        input->failure = E_FAIL;
        char data[23];
        UInt32 count = 99;
        QCOMPARE(alpha->Read(data, sizeof(data), &count), E_FAIL);
        QCOMPARE(count, UInt32(0));
        input->failure = S_OK;
        CMyComPtr<IArchiveExtractCallback> callback = new Extract;
        QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 1, callback), E_ABORT);
    }
    void continuationOpenFailureAndCancellation() {
        fixture("ad-chain-long");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        input->failSeekAt = UInt64(257 + 21) * 2048;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), E_FAIL);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        input->failSeekAt = UINT64_MAX;
        CMyComPtr<IArchiveOpenCallback> cancel = new CancelOpen;
        QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream("alpha.bin"), QByteArray::fromHex(files_["alpha.bin"].toString().toLatin1()));
    }
    void indirectIcbFailuresAndFallback() {
        fixture("icb-file");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        input->failSeekAt = UInt64(257 + 23) * 2048;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), E_FAIL);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        input->failSeekAt = UINT64_MAX;
        CMyComPtr<IArchiveOpenCallback> cancel =
            new CancelAt(input.operator->(), UInt64(257 + 22) * 2048);
        QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        input->failSeekAt = UInt64(257 + 22) * 2048;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream("alpha.bin"), {});
        NWindows::NCOM::CPropVariant flags;
        QCOMPARE(archive_->GetArchiveProperty(kpidErrorFlags, &flags), S_OK);
        QCOMPARE(flags.vt, VARTYPE(VT_UI4));
        QCOMPARE(flags.ulVal, UInt32(0));
        input->failSeekAt = UINT64_MAX;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream("alpha.bin"), QByteArray::fromHex(files_["alpha.bin"].toString().toLatin1()));
    }
    void sparingLoaderErrorsAndCancellation() {
        fixture("sp-first-wins");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        input->failSeekAt = UInt64(64) * 2048;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream("alpha.bin"), QByteArray(8192, 'X'));
        input->failSeekAt = UINT64_MAX;
        CMyComPtr<IArchiveOpenCallback> cancel = new CancelOnce;
        QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto alpha = stream("alpha.bin");
        QVERIFY(alpha);
        input->failSeekAt = UInt64(335) * 2048;
        char data[23];
        count = 99;
        QCOMPARE(alpha->Read(data, sizeof(data), &count), E_FAIL);
        QCOMPARE(count, UInt32(0));
        input->failSeekAt = UINT64_MAX;
        check(alpha, QByteArray::fromHex(files_["alpha.bin"].toString().toLatin1()));
    }
    void discoveryFailuresAndCancellation() {
        fixture("ds-block-2048");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        const QByteArray expected = QByteArray::fromHex(files_["alpha.bin"].toString().toLatin1());
        for (UInt64 offset : {UInt64(32768), UInt64(256 * 2048), UInt64(80 * 2048)}) {
            input->failSeekAt = offset;
            QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
            check(stream("alpha.bin"), expected);
            NWindows::NCOM::CPropVariant flags;
            QCOMPARE(archive_->GetArchiveProperty(kpidErrorFlags, &flags), S_OK);
            QCOMPARE(flags.vt, VARTYPE(VT_UI4));
            QCOMPARE(flags.ulVal, UInt32(0));
        }
        input->seekFailure = E_ABORT;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), E_ABORT);
        input->seekFailure = E_FAIL;
        input->failSeekAt = UINT64_MAX;
        for (HRESULT result : {E_ABORT, E_FAIL}) {
            CMyComPtr<IArchiveOpenCallback> cancel =
                new CancelAt(input.operator->(), UInt64(80 * 2048), result);
            QCOMPARE(archive_->Open(input, nullptr, cancel), result);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
            input->failSeekAt = UInt64(257) * 2048;
            input->seekFailure = result;
            QCOMPARE(archive_->Open(input, nullptr, nullptr), result);
            input->failSeekAt = UINT64_MAX;
        }
        input->seekFailure = E_FAIL;
        input->readFailure = E_FAIL;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), E_FAIL);
        input->readFailure = S_OK;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream("alpha.bin"), expected);
    }
    void guardedAllocationIoAndCancellation() {
        fixture("walk-zero-chain-long");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        input->failSeekAt = UInt64(279) * 2048;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), E_FAIL);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        input->failSeekAt = UINT64_MAX;
        for (HRESULT result : {E_ABORT, E_FAIL}) {
            CMyComPtr<IArchiveOpenCallback> cancel =
                new CancelAt(input.operator->(), UInt64(279) * 2048, result);
            QCOMPARE(archive_->Open(input, nullptr, cancel), result);
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream("alpha.bin"), QByteArray::fromHex(files_["alpha.bin"].toString().toLatin1()));
    }
    void vatLoaderFailuresAndCancellation() {
        fixture("vat20-previous");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        // Optional invalid/missing last entries do not prevent finding the previous VAT.
        input->failSeekAt = UInt64(1399) * 2048;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream("alpha.bin"), QByteArray::fromHex(files_["alpha.bin"].toString().toLatin1()));
        input->failSeekAt = UINT64_MAX;
        CMyComPtr<IArchiveOpenCallback> cancel =
            new CancelAt(input.operator->(), UInt64(1396) * 2048);
        QCOMPARE(archive_->Open(input, nullptr, cancel), E_ABORT);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        CMyComPtr<IArchiveOpenCallback> failedProgress =
            new CancelAt(input.operator->(), UInt64(1396) * 2048, E_FAIL);
        QCOMPARE(archive_->Open(input, nullptr, failedProgress), E_FAIL);
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        fixture("vat20-external");
        input.SetFromCls(new Input(bytes_));
        input->failSeekAt = UInt64(390) * 2048;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), E_FAIL);
        input->failSeekAt = UINT64_MAX;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        auto alpha = stream("alpha.bin");
        QVERIFY(alpha);
        input->failSeekAt = UInt64(315) * 2048;
        char data[23];
        count = 99;
        QCOMPARE(alpha->Read(data, sizeof(data), &count), E_FAIL);
        QCOMPARE(count, UInt32(0));
        input->failSeekAt = UINT64_MAX;
        check(alpha, QByteArray::fromHex(files_["alpha.bin"].toString().toLatin1()));
    }
    void metadataMirrorErrorsAndLifetime() {
        fixture("md-ad-chain");
        CMyComPtr2<IInStream, Input> input;
        input.SetFromCls(new Input(bytes_));
        const QByteArray expected = QByteArray::fromHex(files_["alpha.bin"].toString().toLatin1());
        input->failSeekAt = UInt64(387) * 2048;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream("alpha.bin"), expected);
        input->failSeekAt = UINT64_MAX;
        for (HRESULT result : {E_ABORT, E_FAIL}) {
            for (UInt64 sector : {UInt64(387), UInt64(390)}) {
                CMyComPtr<IArchiveOpenCallback> cancel =
                    new CancelAt(input.operator->(), sector * 2048, result);
                QCOMPARE(archive_->Open(input, nullptr, cancel), result);
                UInt32 count = 99;
                QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
                QCOMPARE(count, UInt32(0));
            }
        }
        CMyComPtr2<IArchiveOpenCallback, OpenProgress> progress;
        progress.SetFromCls(new OpenProgress);
        QCOMPARE(archive_->Open(input, nullptr, progress), S_OK);
        const unsigned calls = progress->calls;
        progress->closed = true;
        input->failSeekAt = UInt64(390) * 2048;
        auto alpha = stream("alpha.bin");
        QVERIFY(alpha);
        check(alpha, expected);
        QCOMPARE(progress->calls, calls);
        input->failure = E_FAIL;
        char data[17];
        UInt32 count = 99;
        QCOMPARE(alpha->Seek(0, STREAM_SEEK_SET, nullptr), S_OK);
        QCOMPARE(alpha->Read(data, sizeof(data), &count), E_FAIL);
        QCOMPARE(count, UInt32(0));
        input->failure = S_OK;
        check(alpha, expected);

        input->failSeekAt = UINT64_MAX;
        input->failure = E_FAIL;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), E_FAIL);
        input->failure = S_OK;
        input->failSeekAt = UInt64(390) * 2048;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        check(stream("alpha.bin"), expected);
        QCOMPARE(archive_->Close(), S_OK);
        archive_.Release();
        check(alpha, expected);
    }
    void backendExtraction_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"sparse-long",
                                 "ad-directory",
                                 "ad-chain-long",
                                 "ad-cross-partition",
                                 "icb-efe",
                                 "icb-root",
                                 "icb-cross-partition",
                                 "icb-metadata-cross",
                                 "sp-reverse",
                                 "sp-directory-cross",
                                 "sp-metadata",
                                 "vat15-inline",
                                 "vat20-external",
                                 "md-fragmented",
                                 "md-mirror-lazy",
                                 "md-ad-chain",
                                 "md-sparable",
                                 "walk-zero-chain-long",
                                 "walk-metadata-zero-chain",
                                 "walk-metadata-cycle-mirror",
                                 "ds-block-1024",
                                 "ds-block-4096-spaced",
                                 "ds-backup-last",
                                 "ds-vdp",
                                 "ds-prevailing-new",
                                 "ds-main-reserve"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void backendExtraction() {
        QFETCH(QString, kind);
        fixture(kind);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QFile image(temporary.filePath(QString::fromUtf8("образ 空.udf")));
        QVERIFY(image.open(QIODevice::WriteOnly));
        QCOMPARE(image.write(bytes_), qint64(bytes_.size()));
        image.close();
        const QDir runtime(QString::fromUtf8(SEVENZIP_UDF_RUNTIME));
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
        QVERIFY(backend.codecCatalog()->formatIndex("Udf") >= 0);
        QVERIFY(!backend.codecCatalog()->canUpdate("Udf"));
        QSignalSpy finished(&backend, &ArchiveBackend::finished);
        backend.list(image.fileName());
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        finished.clear();
        const QString output = temporary.filePath("output");
        backend.extract(image.fileName(), output, {"alpha.bin"});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile file(QDir(output).filePath("alpha.bin"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray expected = QByteArray::fromHex(files_["alpha.bin"].toString().toLatin1());
        QCOMPARE(file.readAll(), expected);
        finished.clear();
        const QString saved = temporary.filePath("source.bin");
        backend.extractSourceEntry(image.fileName(), "alpha.bin", saved, "Udf", {});
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 10000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QFile source(saved);
        QVERIFY(source.open(QIODevice::ReadOnly));
        QCOMPARE(source.readAll(), expected);
    }
};

QTEST_GUILESS_MAIN(UdfStreamTests)
#include "udf_stream_tests.moc"
