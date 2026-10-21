#include "ntfs_test_support.h"

using namespace NtfsTestSupport;

class NtfsDirectoryTests final : public QObject, protected Fixture {
    Q_OBJECT
  private slots:
    void initTestCase() { createArchive(); }
    void directoryIndexes_data() {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<bool>("limited");
        for (const char *kind :
             {"dir-resident", "dir-allocation", "dir-bitmap-nonresident", "dir-fragmented",
              "dir-extension", "dir-small-block", "dir-case-links"})
            QTest::newRow(kind) << QString::fromLatin1(kind) << false;
        for (const char *kind :
             {"dir-index-only", "dir-unindexed-name", "dir-root-missing", "dir-root-invalid",
              "dir-node-vbn", "dir-node-fixup", "dir-node-name-key", "dir-bitmap-outside",
              "dir-bad-list-ref", "dir-bitmap-initialized", "dir-child-outside",
              "dir-bitmap-late-outside", "dir-extension-sequence", "dir-extension-owner",
              "dir-list-malformed", "dir-path-escape", "dir-dangling-record", "dir-no-usable-name",
              "dir-duplicate-fn"})
            QTest::newRow(kind) << QString::fromLatin1(kind) << true;
    }
    void directoryIndexes() {
        QFETCH(QString, kind);
        QFETCH(bool, limited);
        loadFixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QString limitations;
        QVERIFY(readLimitations(limitations));
        QCOMPARE(!limitations.isEmpty(), limited);
        const auto files = record_["files"].toObject();
        for (auto it = files.begin(); it != files.end(); ++it) {
            auto file = stream(it.key());
            QVERIFY2(file, qPrintable(it.key()));
            check(file, it.key(), 0, 200000);
        }
        if (kind == "dir-path-escape") {
            QCOMPARE(index("../escape"), UInt32(UINT32_MAX));
            QCOMPARE(index("escape"), UInt32(UINT32_MAX));
        }
        if (kind == "dir-dangling-record")
            QCOMPARE(index("unreadable"), UInt32(UINT32_MAX));
        const QString chosen = files.contains("index 空") ? QStringLiteral("index 空")
                               : files.contains("gamma")  ? QStringLiteral("gamma")
                                                          : QStringLiteral("[LOST]/[NONAME]-66");
        auto retained = stream(chosen);
        QVERIFY(retained);
        QCOMPARE(archive_->Close(), S_OK);
        input.Release();
        check(retained, chosen, 0, 200000);
    }
    void directoryReadFailureAndCancellationPreserveUsableData() {
        loadFixture("dir-allocation");
        auto *source = new Input(bytes_);
        CMyComPtr<IInStream> input = source;
        source->failSeekAt = 1100 * 4096;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QString limitations;
        QVERIFY(readLimitations(limitations));
        QVERIFY(!limitations.isEmpty());
        check(stream("gamma"), "gamma", 0, 200000);
        source->failedSeekResult = E_ABORT;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), E_ABORT);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        source->failSeekAt = UINT64_MAX;
        CMyComPtr2<IArchiveOpenCallback, CancelEaOpen> callback;
        callback.SetFromCls(new CancelEaOpen);
        callback->source = source;
        for (HRESULT result : {E_FAIL, E_ABORT}) {
            source->cancelRequested = false;
            source->cancelAt = 1100 * 4096;
            callback->failure = result;
            QCOMPARE(archive_->Open(input, nullptr, callback), result);
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        source->cancelRequested = false;
        source->cancelAt = UINT64_MAX;
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        QVERIFY(readLimitations(limitations));
        QVERIFY(limitations.isEmpty());
        check(stream("gamma"), "gamma", 0, 200000);
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(NtfsDirectoryTests)
#include "ntfs_directory_tests.moc"
