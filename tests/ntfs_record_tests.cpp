#include "ntfs_test_support.h"
#include <QJsonArray>

using namespace NtfsTestSupport;

class NtfsRecordTests final : public QObject, protected Fixture {
    Q_OBJECT
    void checkUnavailable(const QString &name) {
        const UInt32 item = index(name);
        QVERIFY2(item != UINT32_MAX, qPrintable(name));
        NWindows::NCOM::CPropVariant size, error;
        QCOMPARE(archive_->GetProperty(item, kpidSize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(VT_EMPTY));
        QCOMPARE(archive_->GetProperty(item, kpidError, &error), S_OK);
        QCOMPARE(error.vt, VARTYPE(VT_BSTR));
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> failed;
        QCOMPARE(provider->GetStream(item, &failed), S_FALSE);
        QVERIFY(!failed);
    }
  private slots:
    void initTestCase() { createArchive(); }
    void damagedRecords_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind : {"record-base",     "record-total",          "record-usa-count",
                                 "record-usa-tail", "record-signature",      "record-baad",
                                 "record-offset",   "record-before-data",    "record-after-data",
                                 "record-unicode",  "record-root",           "record-bitmap",
                                 "record-security", "record-extension",      "record-boot-fixup",
                                 "record-number",   "standard-record-total", "invalid-initialized",
                                 "record-usa-zero", "record-usa-overrun"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void damagedRecords() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QString limitations;
        QVERIFY(readLimitations(limitations));
        QCOMPARE(limitations.contains("NTFS MFT records"), kind != "record-base");
        const auto files = record_["files"].toObject();
        for (auto it = files.begin(); it != files.end(); ++it) {
            if (kind == "standard-record-total" && it.key() == "gamma")
                continue;
            if (kind == "invalid-initialized" && it.key() == "alpha")
                continue;
            auto file = stream(it.key());
            QVERIFY2(file, qPrintable(it.key()));
            check(file, it.key(), 0, 200000);
        }
        for (const auto &name : record_["unavailable"].toArray())
            checkUnavailable(name.toString());
        if (kind == "standard-record-total")
            checkUnavailable("gamma");
        if (kind == "invalid-initialized")
            checkUnavailable("alpha");
        if (kind == "record-bitmap") {
            NWindows::NCOM::CPropVariant free;
            QCOMPARE(archive_->GetArchiveProperty(kpidFreeSpace, &free), S_OK);
            QCOMPARE(free.vt, VARTYPE(VT_EMPTY));
        }
        const QString retainedName = kind == "invalid-initialized" ? "gamma" : "alpha";
        auto retained = stream(retainedName);
        QVERIFY(retained);
        QCOMPARE(archive_->Close(), S_OK);
        input.Release();
        check(retained, retainedName, 0, 200000);
    }
    void failedChunkIsRetriedByRecordAndReopenRecovers() {
        loadFixture("record-base");
        const UInt64 mft = qFromLittleEndian<quint64>(bytes_.constData() + 48) * 4096;
        for (HRESULT failure : {E_FAIL, S_OK}) {
            auto *source = new Input(bytes_);
            CMyComPtr<IInStream> input = source;
            source->failReadAt = mft + 65 * 1024;
            source->failReadEnd = source->failReadAt + 1024;
            source->failedReadResult = failure; // S_OK + zero bytes is unexpected EOF.
            QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
            check(stream("alpha"), "alpha", 0, 200000);
            check(stream("gamma"), "gamma", 0, 200000);
            checkUnavailable(QStringLiteral("данные 空"));
            QVERIFY(source->failedReads > 0);
            QString limitations;
            QVERIFY(readLimitations(limitations));
            QVERIFY(limitations.contains("NTFS MFT records"));
            source->failReadAt = source->failReadEnd = UINT64_MAX;
            QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
            QVERIFY(readLimitations(limitations));
            QVERIFY(limitations.isEmpty());
            check(stream(QStringLiteral("данные 空")), QStringLiteral("данные 空"), 0, 200000);
        }
    }
    void recoveredBulkReadDoesNotInventLimitations() {
        loadFixture("record-base");
        auto *source = new Input(bytes_);
        CMyComPtr<IInStream> input = source;
        source->failReadAt = qFromLittleEndian<quint64>(bytes_.constData() + 48) * 4096;
        source->failReadEnd = source->failReadAt + 67 * 1024;
        source->failReadMinSize = 1025;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QString limitations;
        QVERIFY(readLimitations(limitations));
        QVERIFY(limitations.isEmpty());
        check(stream("gamma"), "gamma", 0, 200000);
    }
    void cancellationAndCompleteReadFailureRemainFatal() {
        loadFixture("record-base");
        const UInt64 fault = qFromLittleEndian<quint64>(bytes_.constData() + 48) * 4096 + 65 * 1024;
        for (HRESULT failure : {E_FAIL, E_ABORT}) {
            auto *source = new Input(bytes_);
            CMyComPtr<IInStream> input = source;
            CMyComPtr2<IArchiveOpenCallback, CancelOpen> callback;
            callback.SetFromCls(new CancelOpen);
            callback->failure = failure;
            QCOMPARE(archive_->Open(input, nullptr, callback), failure);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        {
            auto *source = new Input(bytes_);
            CMyComPtr<IInStream> input = source;
            source->failReadAt = fault;
            source->failReadEnd = fault + 1024;
            source->failedReadResult = E_ABORT;
            QCOMPARE(archive_->Open(input, nullptr, nullptr), E_ABORT);
        }
        auto *source = new Input(bytes_);
        CMyComPtr<IInStream> input = source;
        source->failure = E_FAIL;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), E_FAIL);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        loadFixture("record-boot-total");
        input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_FALSE);
    }
    void boundedSources_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"source-prefix", "source-partial-data", "source-partial-cluster", "source-after-mft",
              "source-no-backup", "source-backup-used", "source-backup-mismatch",
              "source-backup-invalid", "source-bootstrap-short", "source-header-short",
              "source-no-valid-boot"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void boundedSources() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr<IInStream> source = new Input(bytes_);
        const bool fatal = kind == "source-bootstrap-short" || kind == "source-header-short" ||
                           kind == "source-no-valid-boot";
        QCOMPARE(archive_->Open(source, nullptr, nullptr), fatal ? S_FALSE : S_OK);
        if (fatal) {
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
            return;
        }
        QString limitations;
        QVERIFY(readLimitations(limitations));
        QCOMPARE(limitations.isEmpty(), kind == "source-no-backup");
        NWindows::NCOM::CPropVariant physical;
        QCOMPARE(archive_->GetArchiveProperty(kpidPhySize, &physical), S_OK);
        QCOMPARE(physical.vt, VARTYPE(VT_UI8));
        QVERIFY(physical.uhVal.QuadPart <= UInt64(bytes_.size()));
        if (kind == "source-after-mft") {
            NWindows::NCOM::CPropVariant free;
            QCOMPARE(archive_->GetArchiveProperty(kpidFreeSpace, &free), S_OK);
            QCOMPARE(free.vt, VARTYPE(VT_EMPTY));
        }
        const auto files = record_["files"].toObject();
        for (auto it = files.begin(); it != files.end(); ++it)
            check(stream(it.key()), it.key(), 0, 200000);
        for (const QString &name : {QStringLiteral("alpha"), QStringLiteral("gamma")}) {
            if (files.contains(name))
                continue;
            const UInt32 item = index(name);
            QVERIFY(item != UINT32_MAX);
            CMyComPtr2<IArchiveExtractCallback, Extract> callback;
            callback.SetFromCls(new Extract);
            const HRESULT result = archive_->Extract(&item, 1, 1, callback);
            QVERIFY(result != S_OK ||
                    callback->result == NArchive::NExtract::NOperationResult::kDataError);
        }
        const QString name = files.contains("alpha") ? "alpha" : QStringLiteral("данные 空");
        auto retained = stream(name);
        QVERIFY(retained);
        QCOMPARE(archive_->Close(), S_OK);
        source.Release();
        check(retained, name, 0, 200000);
    }
    void optionalBackupFailuresAndReopen() {
        loadFixture("base");
        const UInt64 backup = qFromLittleEndian<quint64>(bytes_.constData() + 40) * 512;
        for (bool seek : {false, true}) {
            for (HRESULT failure : {E_FAIL, S_OK, E_ABORT, E_OUTOFMEMORY}) {
                if (seek && failure == S_OK)
                    continue; // Zero-byte success models EOF only for a read.
                auto *input = new Input(bytes_);
                CMyComPtr<IInStream> source = input;
                if (seek) {
                    input->failSeekAt = backup;
                    input->failedSeekResult = failure;
                } else {
                    input->failReadAt = backup;
                    input->failReadEnd = backup + 512;
                    input->failedReadResult = failure;
                }
                const bool fatal = failure == E_ABORT || failure == E_OUTOFMEMORY;
                QCOMPARE(archive_->Open(source, nullptr, nullptr), fatal ? failure : S_OK);
                if (fatal)
                    continue;
                QString limitations;
                QVERIFY(readLimitations(limitations));
                QVERIFY(limitations.contains("backup NTFS boot sector"));
                check(stream("alpha"), "alpha", 0, 200000);
                input->failSeekAt = input->failReadAt = input->failReadEnd = UINT64_MAX;
                QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
                QVERIFY(readLimitations(limitations));
                QVERIFY(limitations.isEmpty());
                check(stream("gamma"), "gamma", 0, 200000);
            }
        }
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(NtfsRecordTests)
#include "ntfs_record_tests.moc"
