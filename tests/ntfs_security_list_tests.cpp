#include "ntfs_test_support.h"

using namespace NtfsTestSupport;

class NtfsSecurityListTests final : public QObject, protected Fixture {
    Q_OBJECT
    void descriptor() {
        CMyComPtr<IArchiveGetRawProps> properties;
        QCOMPARE(archive_->QueryInterface(IID_IArchiveGetRawProps,
                                          reinterpret_cast<void **>(&properties)),
                 S_OK);
        const void *data = nullptr;
        UInt32 size = 0, type = 0;
        QCOMPARE(properties->GetRawProp(index("gamma"), kpidNtSecure, &data, &size, &type), S_OK);
        const QByteArray expected = QByteArray::fromHex(record_["security"].toString().toLatin1());
        QCOMPARE(size, UInt32(expected.size()));
        if (expected.isEmpty())
            QVERIFY(!data);
        else {
            QCOMPARE(type, UInt32(NPropDataType::kRaw));
            QVERIFY(data);
            QCOMPARE(QByteArray(static_cast<const char *>(data), size), expected);
        }
    }
  private slots:
    void initTestCase() { createArchive(); }
    void authoritativeIndexes_data() {
        QTest::addColumn<QString>("kind");
        for (const char *kind :
             {"security-list-selected", "security-list-nonresident", "security-list-split",
              "security-list-metadata-flags", "security-list-bad-id", "security-list-bad-sequence",
              "security-list-wrong-owner", "security-list-owner-sequence",
              "security-list-missing-root", "security-list-missing-tail", "security-list-no-list",
              "security-list-malformed", "security-list-unreadable",
              "security-list-unverified-record", "security-list-root-vcn",
              "security-list-duplicate"})
            QTest::newRow(kind) << QString::fromLatin1(kind);
    }
    void authoritativeIndexes() {
        QFETCH(QString, kind);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        descriptor();
        if (kind == "security-list-missing-tail") {
            CMyComPtr<IArchiveGetRawProps> properties;
            QCOMPARE(archive_->QueryInterface(IID_IArchiveGetRawProps,
                                              reinterpret_cast<void **>(&properties)),
                     S_OK);
            const void *data = nullptr;
            UInt32 size = 0, type = 0;
            QCOMPARE(properties->GetRawProp(index("alpha"), kpidNtSecure, &data, &size, &type),
                     S_OK);
            QVERIFY(data && size);
            const QByteArray alpha(static_cast<const char *>(data), size);
            loadFixture("security-root-key");
            const QByteArray expected =
                QByteArray::fromHex(record_["security"].toString().toLatin1());
            QCOMPARE(alpha, expected);
            loadFixture(kind);
        }
        QString limitations;
        QVERIFY(readLimitations(limitations));
        const bool unavailable = record_["security"].toString().isEmpty();
        QCOMPARE(limitations.contains("NTFS security descriptors"), unavailable);
        const auto files = record_["files"].toObject();
        for (auto it = files.begin(); it != files.end(); ++it)
            check(stream(it.key()), it.key(), 0, 200000);
        auto retained = stream("gamma");
        QVERIFY(retained);
        QCOMPARE(archive_->Close(), S_OK);
        input.Release();
        check(retained, "gamma", 0, 200000);
    }
    void listIoAndReopening() {
        loadFixture("security-list-nonresident");
        const QJsonValue expected = record_["security"];
        for (HRESULT failure : {E_FAIL, S_OK, E_ABORT, E_OUTOFMEMORY}) {
            auto *source = new Input(bytes_);
            CMyComPtr<IInStream> input = source;
            source->failReadAt = 1100 * 4096;
            source->failReadEnd = 1101 * 4096;
            source->failedReadResult = failure;
            const bool fatal = failure == E_ABORT || failure == E_OUTOFMEMORY;
            QCOMPARE(archive_->Open(input, nullptr, nullptr), fatal ? failure : S_OK);
            if (!fatal) {
                record_["security"] = "";
                descriptor();
                check(stream("alpha"), "alpha", 0, 200000);
                check(stream("gamma"), "gamma", 0, 200000);
            }
            source->failReadAt = source->failReadEnd = UINT64_MAX;
            QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
            record_["security"] = expected;
            descriptor();
        }
    }
    void callbackFailureIsNotOptionalMetadata() {
        loadFixture("security-list-nonresident");
        for (HRESULT failure : {E_FAIL, E_ABORT}) {
            auto *source = new Input(bytes_);
            CMyComPtr<IInStream> input = source;
            source->cancelAt = 1100 * 4096;
            CMyComPtr2<IArchiveOpenCallback, CancelEaOpen> callback;
            callback.SetFromCls(new CancelEaOpen);
            callback->source = source;
            callback->failure = failure;
            QCOMPARE(archive_->Open(input, nullptr, callback), failure);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(NtfsSecurityListTests)
#include "ntfs_security_list_tests.moc"
