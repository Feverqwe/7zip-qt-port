#include "Windows/PropVariant.h"
#include "fat_test_support.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QProcess>
#include <QSet>
#include <QtEndian>
#include <QtTest>

using namespace FatTestSupport;

class DirectoryCallback final : public IArchiveOpenCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveOpenCallback)
  public:
    Input *source = nullptr;
    HRESULT failure = E_ABORT;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override {
        return source->reachedFault ? failure : S_OK;
    }
};

class FatDirectoryTests final : public QObject {
    Q_OBJECT
    QLibrary module_{QString::fromUtf8(SEVENZIP_FAT_MODULE)};
    CMyComPtr<IInArchive> archive_;
    QByteArray bytes_;
    QJsonObject fixture_;
    void fixture(const QString &kind) {
        QProcess process;
        process.start(QString::fromUtf8(SEVENZIP_FAT_PYTHON),
                      {QString::fromUtf8(SEVENZIP_FAT_FIXTURE), "--directory=" + kind, "--bundle"});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        fixture_ = QJsonDocument::fromJson(process.readAllStandardOutput()).object();
        QByteArray packed(4, '\0');
        qToBigEndian(quint32(fixture_["size"].toInt()), packed.data());
        packed += QByteArray::fromBase64(fixture_["image"].toString().toLatin1());
        bytes_ = qUncompress(packed);
        QVERIFY(!bytes_.isEmpty());
    }
    UInt32 index(const QString &name) {
        UInt32 count = 0;
        archive_->GetNumberOfItems(&count);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant path;
            archive_->GetProperty(i, kpidPath, &path);
            if (path.vt == VT_BSTR &&
                QString::fromWCharArray(path.bstrVal).replace('\\', '/') == name)
                return i;
        }
        return UINT32_MAX;
    }
    void verifyFiles() {
        const auto files = fixture_["files"].toObject();
        QSet<QString> actualPaths;
        UInt32 count = 0;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant path, directory;
            QCOMPARE(archive_->GetProperty(i, kpidPath, &path), S_OK);
            QCOMPARE(archive_->GetProperty(i, kpidIsDir, &directory), S_OK);
            if (directory.vt == VT_BOOL && !directory.boolVal) {
                QCOMPARE(path.vt, VARTYPE(VT_BSTR));
                actualPaths.insert(QString::fromWCharArray(path.bstrVal).replace('\\', '/'));
            }
        }
        const auto expectedPaths = files.keys();
        QCOMPARE(actualPaths, QSet<QString>(expectedPaths.begin(), expectedPaths.end()));
        for (auto it = files.begin(); it != files.end(); ++it) {
            const UInt32 id = index(it.key());
            QVERIFY2(id != UINT32_MAX, qPrintable(it.key()));
            CMyComPtr2<IArchiveExtractCallback, Extract> callback;
            callback.Create_if_Empty();
            QCOMPARE(archive_->Extract(&id, 1, 0, callback), S_OK);
            QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kOK));
            QCOMPARE(callback->output->bytes,
                     QByteArray::fromBase64(it.value().toString().toLatin1()));
        }
    }
    void verifyLimitations(bool limited) {
        UInt32 count = 0;
        QCOMPARE(archive_->GetNumberOfArchiveProperties(&count), S_OK);
        bool found = false;
        for (UInt32 i = 0; i < count; ++i) {
            CMyComBSTR name;
            PROPID id;
            VARTYPE type;
            QCOMPARE(archive_->GetArchivePropertyInfo(i, &name, &id, &type), S_OK);
            if (!name || QString::fromWCharArray(name) != "Read-only limitations")
                continue;
            found = true;
            NWindows::NCOM::CPropVariant value;
            QCOMPARE(archive_->GetArchiveProperty(id, &value), S_OK);
            QCOMPARE(value.vt, VARTYPE(limited ? VT_BSTR : VT_EMPTY));
            if (limited)
                QVERIFY(QString::fromWCharArray(value.bstrVal).contains("FAT directories"));
        }
        QVERIFY(found);
    }
  private slots:
    void initTestCase() {
        QVERIFY(module_.load());
        const auto property =
            reinterpret_cast<Func_GetHandlerProperty2>(module_.resolve("GetHandlerProperty2"));
        const auto create = reinterpret_cast<Func_CreateObject>(module_.resolve("CreateObject"));
        QVERIFY(property && create);
        NWindows::NCOM::CPropVariant clsid;
        QCOMPARE(property(0, NArchive::NHandlerPropID::kClassID, &clsid), S_OK);
        GUID guid;
        memcpy(&guid, clsid.bstrVal, sizeof(guid));
        QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
    }
    void isolatedChains_data() {
        QTest::addColumn<QString>("kind");
        for (const QString &bits : {"12", "16", "32"})
            for (const QString &mode :
                 {"control", "free", "range", "bad", "cycle", "start", "eoc", "alias"})
                QTest::newRow(qPrintable(mode + bits)) << mode + bits;
    }
    void isolatedChains() {
        QFETCH(QString, kind);
        fixture(kind);
        CMyComPtr<IInStream> input = new Input(bytes_);
        QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
        verifyFiles();
        verifyLimitations(fixture_["directory_limited"].toBool());
        for (const auto &name : fixture_["incomplete_dirs"].toArray()) {
            const UInt32 id = index(name.toString());
            QVERIFY(id != UINT32_MAX);
            NWindows::NCOM::CPropVariant error;
            QCOMPARE(archive_->GetProperty(id, kpidError, &error), S_OK);
            QCOMPARE(error.vt, VARTYPE(VT_BSTR));
            CMyComPtr2<IArchiveExtractCallback, Extract> callback;
            callback.Create_if_Empty();
            QCOMPARE(archive_->Extract(&id, 1, 0, callback), S_OK);
            QCOMPARE(callback->result, Int32(NArchive::NExtract::NOperationResult::kDataError));
        }
        QCOMPARE(archive_->Close(), S_OK);
        verifyLimitations(false);
    }
    void ioGapsCancellationAndReuse() {
        for (const QString &kind : {"iochild32", "ioroot12", "prefix16", "eodprefix32"}) {
            fixture(kind);
            for (HRESULT failure : {E_FAIL, S_OK, E_ABORT, E_OUTOFMEMORY}) {
                auto *source = new Input(bytes_);
                CMyComPtr<IInStream> input = source;
                source->failAt = UInt64(fixture_["fault_offset"].toInteger());
                source->failEnd = source->failAt + 512;
                source->partialBytes = kind.startsWith("eodprefix") ? 1
                                       : kind.startsWith("prefix")  ? 32
                                                                    : 0;
                source->failure = failure;
                const bool fatal = failure == E_ABORT || failure == E_OUTOFMEMORY;
                QCOMPARE(archive_->Open(input, nullptr, nullptr), fatal ? failure : S_OK);
                if (!fatal) {
                    verifyFiles();
                    verifyLimitations(kind != "eodprefix32" || failure != S_OK);
                }
                source->failAt = UINT64_MAX;
                QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
                verifyFiles();
                verifyLimitations(false);
            }
        }
        fixture("iochild32");
        for (HRESULT failure : {E_FAIL, E_ABORT}) {
            auto *source = new Input(bytes_);
            CMyComPtr<IInStream> input = source;
            source->failAt = UInt64(fixture_["fault_offset"].toInteger());
            source->failure = E_FAIL;
            CMyComPtr2<IArchiveOpenCallback, DirectoryCallback> callback;
            callback.Create_if_Empty();
            callback->source = source;
            callback->failure = failure;
            QCOMPARE(archive_->Open(input, nullptr, callback), failure);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
    }
    void seekFailuresAndReopening() {
        fixture("iochild32");
        for (HRESULT failure : {E_FAIL, E_ABORT, E_OUTOFMEMORY}) {
            auto *source = new Input(bytes_);
            CMyComPtr<IInStream> input = source;
            source->failAt = UInt64(fixture_["fault_offset"].toInteger());
            source->failSeek = true;
            source->failure = failure;
            QCOMPARE(archive_->Open(input, nullptr, nullptr), failure == E_FAIL ? S_OK : failure);
            if (failure == E_FAIL) {
                verifyFiles();
                verifyLimitations(true);
            } else {
                UInt32 count = 99;
                QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
                QCOMPARE(count, UInt32(0));
            }
            source->failAt = UINT64_MAX;
            QCOMPARE(archive_->Open(input, nullptr, nullptr), S_OK);
            verifyFiles();
            verifyLimitations(false);
        }
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(FatDirectoryTests)
#include "fat_directory_tests.moc"
