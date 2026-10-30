#include "formats/ntfs/support/ntfs_test_support.h"
#include <QMap>

using namespace NtfsTestSupport;

namespace {
class Sink final : public ISequentialOutStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(ISequentialOutStream)
    QByteArray &bytes_;
    HRESULT failure_;

  public:
    Sink(QByteArray &bytes, HRESULT failure) : bytes_(bytes), failure_(failure) {}
    Z7_COM7F_IMF(Write(const void *data, UInt32 size, UInt32 *done)) override {
        if (done)
            *done = 0;
        if (failure_ != S_OK)
            return failure_;
        bytes_.append(static_cast<const char *>(data), size);
        if (done)
            *done = size;
        return S_OK;
    }
};
class Copy final : public IArchiveExtractCallback,
                   public IArchiveExtractCallbackMessage2,
                   public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IArchiveExtractCallback, IArchiveExtractCallbackMessage2)
    UInt32 current_ = 0;

  public:
    QMap<UInt32, QByteArray> bytes;
    QMap<UInt32, Int32> results;
    HRESULT outputFailure = S_OK, reportFailure = S_OK;
    int reports = 0;
    UInt32 indexType = UINT32_MAX;
    Int32 reportedError = -1;
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(GetStream(UInt32 index, ISequentialOutStream **stream, Int32 mode)) override {
        current_ = index;
        *stream = nullptr;
        if (mode == NArchive::NExtract::NAskMode::kExtract) {
            CMyComPtr<ISequentialOutStream> output = new Sink(bytes[index], outputFailure);
            *stream = output.Detach();
        }
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 result)) override {
        results[current_] = result;
        return S_OK;
    }
    Z7_COM7F_IMF(ReportExtractResult(UInt32 type, UInt32, Int32 error)) override {
        ++reports;
        indexType = type;
        reportedError = error;
        return reportFailure;
    }
};
} // namespace

class NtfsRecoveryTests final : public QObject, protected Fixture {
    Q_OBJECT
    Input *source(const QString &kind) {
        auto *input = new Input(bytes_);
        if (kind == "recovery-mirror-io" || kind == "recovery-boot-io" ||
            kind == "recovery-list-io") {
            input->failReadAt = kind == "recovery-boot-io" ? 0
                                : kind == "recovery-list-io"
                                    ? 1100 * 4096
                                    : qFromLittleEndian<quint64>(bytes_.constData() + 48) * 4096;
            input->failReadEnd = input->failReadAt + (kind == "recovery-boot-io" ? 512 : 1024);
        }
        return input;
    }
  private slots:
    void initTestCase() { createArchive(); }
    void corruptBootstrapUsesMirrorAndReopens_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"mft-list-bad-sequence", "mft-list-bad-id", "mft-list-unmapped", "mft-list-bad-list",
              "mft-list-named-only", "mft-list-nul-name", "mft-list-nul-extension"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void corruptBootstrapUsesMirrorAndReopens() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QString limitations;
        QVERIFY(readLimitations(limitations));
        QVERIFY(limitations.contains("backup MFT record"));
        check(stream("alpha"), "alpha", 0, 200000);
        loadFixture("base");
        input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
    }
    void readableBootstrap_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"recovery-boot-io", "recovery-primary-address", "recovery-mirror-address",
              "recovery-mirror-header", "recovery-mirror-signature", "recovery-mirror-data",
              "recovery-mirror-io", "recovery-primary-control", "recovery-list-partial",
              "recovery-list-io", "recovery-list-segment", "recovery-attribute-tail",
              "recovery-record-tail", "recovery-inactive-bootstrap"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void readableBootstrap() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr<IInStream> input = source(kind);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QString limitations;
        QVERIFY(readLimitations(limitations));
        QCOMPARE(limitations.isEmpty(), kind == "recovery-primary-control");
        QCOMPARE(limitations.contains("backup MFT record"),
                 kind == "recovery-primary-address" ||
                     (kind.startsWith("recovery-mirror-") && kind != "recovery-mirror-address"));
        if (kind == "recovery-boot-io")
            QVERIFY(limitations.contains("backup boot sector"));
        const auto files = record_["files"].toObject();
        QVERIFY(!files.isEmpty());
        for (auto it = files.begin(); it != files.end(); ++it) {
            auto file = stream(it.key());
            check(file, it.key(), 0, 200000);
            const UInt32 selected = index(it.key());
            CMyComPtr2<IArchiveExtractCallback, Copy> copy;
            copy.Create_if_Empty();
            QCOMPARE(archive_->Extract(&selected, 1, 0, copy), S_OK);
            QCOMPARE(copy->results[selected], Int32(NArchive::NExtract::NOperationResult::kOK));
            QCOMPARE(copy->bytes[selected],
                     QByteArray::fromBase64(it.value().toString().toLatin1()));
            QCOMPARE(copy->reports, 0);
        }
        loadFixture("base");
        input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QVERIFY(readLimitations(limitations));
        QVERIFY(limitations.isEmpty());
    }
    void unusableBootstrap_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"recovery-both-bad", "recovery-both-addresses", "recovery-only-inactive",
              "recovery-mirror-bounds", "recovery-mirror-named"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void unusableBootstrap() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_FALSE);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        NWindows::NCOM::CPropVariant error;
        QCOMPARE(archive_->GetArchiveProperty(kpidError, &error), S_OK);
        QCOMPARE(error.vt, VARTYPE(VT_BSTR));
        QVERIFY(SysStringLen(error.bstrVal) > 0);
        loadFixture("base");
        input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QCOMPARE(archive_->GetArchiveProperty(kpidError, &error), S_OK);
        QCOMPARE(error.vt, VARTYPE(VT_EMPTY));
    }
    void fullCopySeparatesMissingRecordsFromUnverifiedReads_data() {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<bool>("incomplete");
        for (const char *kind :
             {"recovery-attribute-tail", "recovery-list-partial", "recovery-record-tail",
              "recovery-mirror-signature", "record-root", "record-total"})
            QTest::newRow(kind) << QString::fromLatin1(kind) << true;
        for (const char *kind : {"record-number", "record-usa-tail"})
            QTest::newRow(kind) << QString::fromLatin1(kind) << false;
    }
    void fullCopySeparatesMissingRecordsFromUnverifiedReads() {
        QFETCH(QString, kind);
        QFETCH(bool, incomplete);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        UInt32 count = 0;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QVector<UInt32> indices;
        for (UInt32 i = 0; i < count; ++i)
            indices.append(i);
        for (bool explicitIndices : {false, true}) {
            CMyComPtr2<IArchiveExtractCallback, Copy> copy;
            copy.Create_if_Empty();
            QCOMPARE(archive_->Extract(explicitIndices ? indices.constData() : nullptr,
                                       explicitIndices ? count : UInt32(-1), 1, copy),
                     S_OK);
            QCOMPARE(copy->reports, incomplete ? 1 : 0);
            if (incomplete) {
                QCOMPARE(copy->indexType, UInt32(NArchive::NEventIndexType::kNoIndex));
                QCOMPARE(copy->reportedError,
                         Int32(NArchive::NExtract::NOperationResult::kHeadersError));
            }
            CMyComPtr2<IArchiveExtractCallback, Extract> legacy;
            legacy.Create_if_Empty();
            QCOMPARE(archive_->Extract(explicitIndices ? indices.constData() : nullptr,
                                       explicitIndices ? count : UInt32(-1), 1, legacy),
                     incomplete ? S_FALSE : S_OK);
        }
        const QString healthy = "alpha";
        const UInt32 selected = index(healthy);
        QVERIFY(selected != UINT32_MAX);
        CMyComPtr2<IArchiveExtractCallback, Copy> copy;
        copy.Create_if_Empty();
        copy->outputFailure = E_FAIL;
        QCOMPARE(archive_->Extract(&selected, 1, 0, copy), E_FAIL);
        copy->outputFailure = S_OK;
        if (incomplete) {
            copy->reportFailure = E_ABORT;
            QCOMPARE(archive_->Extract(nullptr, UInt32(-1), 1, copy), E_ABORT);
        }
    }
    void bootstrapFailuresDoNotAbsorbCancellation() {
        for (const QString &kind : {"recovery-mirror-io", "recovery-list-io", "recovery-boot-io"}) {
            loadFixture(kind);
            for (HRESULT failure : {E_ABORT, E_OUTOFMEMORY}) {
                auto *raw = source(kind);
                raw->failedReadResult = failure;
                CMyComPtr<IInStream> input = raw;
                QCOMPARE(archive_->Open(input, nullptr, nullptr), failure);
            }
        }
        loadFixture("mft-list-nonresident");
        for (HRESULT failure : {E_FAIL, S_FALSE, E_ABORT, E_OUTOFMEMORY}) {
            auto *raw = new Input(bytes_);
            raw->cancelAt = 1100 * 4096;
            CMyComPtr<IInStream> input = raw;
            CMyComPtr2<IArchiveOpenCallback, CancelEaOpen> callback;
            callback.Create_if_Empty();
            callback->source = raw;
            callback->failure = failure;
            QCOMPARE(archive_->Open(input, nullptr, callback), failure);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
            NWindows::NCOM::CPropVariant error;
            QCOMPARE(archive_->GetArchiveProperty(kpidError, &error), S_OK);
            QCOMPARE(error.vt, VARTYPE(VT_EMPTY));
        }
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(NtfsRecoveryTests)
#include "ntfs_recovery_tests.moc"
