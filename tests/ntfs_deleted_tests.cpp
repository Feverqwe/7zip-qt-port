#include "ntfs_test_support.h"

using namespace NtfsTestSupport;

class NtfsDeletedTests final : public QObject, protected Fixture {
    Q_OBJECT
    void showDeleted(bool show) {
        CMyComPtr<ISetProperties> properties;
        QCOMPARE(
            archive_->QueryInterface(IID_ISetProperties, reinterpret_cast<void **>(&properties)),
            S_OK);
        const wchar_t *name = L"ld";
        const NWindows::NCOM::CPropVariant value(show);
        QCOMPARE(properties->SetProperties(&name, &value, 1), S_OK);
    }
    void unavailableGamma() {
        const UInt32 item = index("gamma");
        QVERIFY(item != UINT32_MAX);
        NWindows::NCOM::CPropVariant size, error;
        QCOMPARE(archive_->GetProperty(item, kpidSize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(VT_EMPTY));
        QCOMPARE(archive_->GetProperty(item, kpidError, &error), S_OK);
        QCOMPARE(error.vt, VARTYPE(VT_BSTR));
        QVERIFY(!stream("gamma"));
        CMyComPtr2<IArchiveExtractCallback, Extract> callback;
        callback.SetFromCls(new Extract);
        QCOMPARE(archive_->Extract(&item, 1, 1, callback), S_OK);
        QVERIFY(callback->result != NArchive::NExtract::NOperationResult::kOK);
    }
  private slots:
    void initTestCase() { createArchive(); }
    void recoveryIsOptional_data() {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<bool>("show");
        for (const char *kind :
             {"deleted-data", "deleted-empty", "deleted-no-data", "deleted-link",
              "deleted-extension", "deleted-wof", "deleted-reparse-nonresident",
              "deleted-reparse-bad-run", "deleted-reparse-too-large", "deleted-reparse-empty",
              "deleted-reparse-missing", "deleted-link-invalid", "deleted-orphan-extension"})
            for (bool show : {false, true})
                QTest::newRow(qPrintable(QString::fromLatin1(kind) + (show ? "-shown" : "-hidden")))
                    << QString::fromLatin1(kind) << show;
    }
    void recoveryIsOptional() {
        QFETCH(QString, kind);
        QFETCH(bool, show);
        loadFixture(kind);
        showDeleted(show);
        CMyComPtr<IInStream> source = new Input(bytes_);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        QString limitations;
        QVERIFY(readLimitations(limitations));
        if (!show) {
            QVERIFY(limitations.isEmpty());
            if (kind != "deleted-orphan-extension")
                QCOMPARE(index("gamma"), UINT32_MAX);
        } else if (kind != "deleted-orphan-extension") {
            QVERIFY(limitations.contains("Deleted NTFS records"));
            NWindows::NCOM::CPropVariant deleted;
            QCOMPARE(archive_->GetProperty(index("gamma"), kpidIsDeleted, &deleted), S_OK);
            QCOMPARE(deleted.vt, VARTYPE(VT_BOOL));
            QCOMPARE(deleted.boolVal, VARIANT_TRUE);
            const auto recovered = record_["deleted_files"].toObject();
            const auto active = record_["files"].toObject();
            record_["files"] = recovered;
            if (recovered.contains("gamma")) {
                NWindows::NCOM::CPropVariant size;
                QCOMPARE(archive_->GetProperty(index("gamma"), kpidSize, &size), S_OK);
                QCOMPARE(size.vt, VARTYPE(VT_UI8));
                QCOMPARE(
                    size.uhVal.QuadPart,
                    UInt64(
                        QByteArray::fromBase64(recovered["gamma"].toString().toLatin1()).size()));
            }
            for (auto it = recovered.begin(); it != recovered.end(); ++it)
                check(stream(it.key()), it.key(), 0, 200000);
            if (!recovered.contains("gamma"))
                unavailableGamma();
            record_["files"] = active;
        }
        const auto files = record_["files"].toObject();
        for (auto it = files.begin(); it != files.end(); ++it)
            check(stream(it.key()), it.key(), 0, 200000);
        auto retained = stream("alpha");
        QVERIFY(retained);
        QCOMPARE(archive_->Close(), S_OK);
        source.Release();
        check(retained, "alpha", 0, 200000);
    }
    void failedDeletedMetadataDoesNotCloseLiveFiles() {
        loadFixture("deleted-reparse-nonresident");
        for (bool show : {false, true}) {
            for (HRESULT failure : {E_FAIL, S_OK, E_ABORT, E_OUTOFMEMORY}) {
                showDeleted(show);
                auto *input = new Input(bytes_);
                CMyComPtr<IInStream> source = input;
                input->failReadAt = 1400 * 4096;
                input->failReadEnd = 1401 * 4096;
                input->failedReadResult = failure;
                const bool fatal = show && (failure == E_ABORT || failure == E_OUTOFMEMORY);
                QCOMPARE(archive_->Open(source, nullptr, nullptr), fatal ? failure : S_OK);
                if (fatal)
                    continue;
                check(stream("alpha"), "alpha", 0, 200000);
                if (show)
                    unavailableGamma();
                input->failReadAt = input->failReadEnd = UINT64_MAX;
                QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
                if (show) {
                    const QJsonValue active = record_["files"];
                    record_["files"] = record_["deleted_files"];
                    check(stream("gamma"), "gamma", 0, 200000);
                    record_["files"] = active;
                }
            }
        }
    }
    void callbackFailuresRemainFatal() {
        loadFixture("deleted-reparse-nonresident");
        showDeleted(true);
        for (HRESULT failure : {E_FAIL, E_ABORT}) {
            auto *input = new Input(bytes_);
            CMyComPtr<IInStream> source = input;
            input->cancelAt = 1400 * 4096;
            CMyComPtr2<IArchiveOpenCallback, CancelEaOpen> callback;
            callback.SetFromCls(new CancelEaOpen);
            callback->source = input;
            callback->failure = failure;
            QCOMPARE(archive_->Open(source, nullptr, callback), failure);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(NtfsDeletedTests)
#include "ntfs_deleted_tests.moc"
