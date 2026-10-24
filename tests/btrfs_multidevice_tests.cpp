#include "btrfs_native_fixture.h"

namespace {
class Volumes final : public IArchiveOpenCallback,
                      public IArchiveOpenVolumeCallback,
                      public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IArchiveOpenCallback, IArchiveOpenVolumeCallback)
  public:
    QMap<quint64, QByteArray> devices;
    QStringList requested;
    bool cancel = false;
    Z7_COM7F_IMF(SetTotal(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *, const UInt64 *)) override { return S_OK; }
    Z7_COM7F_IMF(GetProperty(PROPID, PROPVARIANT *value)) override {
        NWindows::NCOM::CPropVariant prop;
        prop.Detach(value);
        return S_OK;
    }
    Z7_COM7F_IMF(GetStream(const wchar_t *filename, IInStream **stream)) override {
        *stream = nullptr;
        const QString path = QString::fromWCharArray(filename);
        requested.append(path);
        if (cancel)
            return E_ABORT;
        if (!path.startsWith("devid-") || !path.endsWith(".img"))
            return E_INVALIDARG;
        const auto id = path.mid(6, path.size() - 10).toULongLong();
        if (!devices.contains(id))
            return S_FALSE;
        CMyComPtr<IInStream> input = new Input(devices[id]);
        *stream = input.Detach();
        return S_OK;
    }
};
} // namespace

class BtrfsMultideviceTests final : public QObject {
    Q_OBJECT
    QLibrary module_{deployedModulePath(QString::fromUtf8(SEVENZIP_BTRFS_MODULE))};
    CMyComPtr<IInArchive> archive_;
    UInt32 index(const QString &path) {
        UInt32 count = 0;
        archive_->GetNumberOfItems(&count);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant value;
            if (archive_->GetProperty(i, kpidPath, &value) == S_OK && value.vt == VT_BSTR &&
                QDir::fromNativeSeparators(QString::fromWCharArray(value.bstrVal)) == path)
                return i;
        }
        return UInt32(-1);
    }
    HRESULT open(const QMap<quint64, QByteArray> &devices, Volumes *callback) {
        CMyComPtr<IInStream> source = new Input(devices.value(1));
        return archive_->Open(source, nullptr, callback);
    }
    void cannotReadStripedPayload(const QMap<quint64, QByteArray> &devices, Volumes *callback) {
        const HRESULT result = open(devices, callback);
        QVERIFY(result == S_OK || result == S_FALSE);
        if (result == S_FALSE) {
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
            return;
        }
        const UInt32 item = index(name);
        QVERIFY(item != UInt32(-1));
        CMyComPtr2<IArchiveExtractCallback, Extract> output;
        output.Create_if_Empty();
        QCOMPARE(archive_->Extract(&item, 1, 0, output), S_OK);
        QVERIFY(output->result != Int32(NArchive::NExtract::NOperationResult::kOK));
    }
  private slots:
    void initTestCase() {
        QVERIFY2(module_.load(), qPrintable(module_.errorString()));
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
    void linuxProfiles_data() {
        QTest::addColumn<QString>("profile");
        const auto profiles = BtrfsFixture::multiManifest()["profiles"].toObject();
        QVERIFY(!profiles.isEmpty());
        for (auto it = profiles.begin(); it != profiles.end(); ++it)
            QTest::newRow(qPrintable(it.key())) << it.key();
    }
    void linuxProfiles() {
        QFETCH(QString, profile);
        const auto devices = BtrfsFixture::devices(profile);
        QVERIFY(devices.size() >= 2);
        {
            CMyComPtr2<IArchiveOpenCallback, Volumes> callback;
            callback.Create_if_Empty();
            callback->devices = devices;
            QCOMPARE(open(devices, callback.ClsPtr()), S_OK);
            QVERIFY(!callback->requested.isEmpty());
        }
        const auto files = BtrfsFixture::multiManifest()["profiles"]
                               .toObject()[profile]
                               .toObject()["expected"]
                               .toObject()["files"]
                               .toObject();
        // Release the opening callback before extraction: member streams belong
        // to the retained archive, not to the callback's lifetime.
        for (auto it = files.begin(); it != files.end(); ++it) {
            const UInt32 item = index(it.key());
            QVERIFY2(item != UInt32(-1), qPrintable(it.key()));
            CMyComPtr2<IArchiveExtractCallback, Extract> output;
            output.Create_if_Empty();
            QCOMPARE(archive_->Extract(&item, 1, 0, output), S_OK);
            QCOMPARE(output->result, Int32(NArchive::NExtract::NOperationResult::kOK));
            QCOMPARE(QString::fromLatin1(
                         QCryptographicHash::hash(output->output->bytes, QCryptographicHash::Sha256)
                             .toHex()),
                     it->toObject()["sha256"].toString());
        }
    }
    void missingMembersAndIdentity() {
        const auto striped = BtrfsFixture::devices("raid0");
        QVERIFY(striped.size() >= 2);
        cannotReadStripedPayload(striped, nullptr);
        CMyComPtr2<IArchiveOpenCallback, Volumes> callback;
        callback.Create_if_Empty();
        callback->devices = striped;
        callback->cancel = true;
        QCOMPARE(open(striped, callback.ClsPtr()), E_ABORT);
        callback->cancel = false;
        callback->devices[2] = striped[1]; // Wrong device ID/UUID from the same filesystem.
        cannotReadStripedPayload(striped, callback.ClsPtr());
        callback->devices[2] = image(); // Valid Btrfs super, unrelated filesystem.
        cannotReadStripedPayload(striped, callback.ClsPtr());
        callback->devices = striped;
        QCOMPARE(open(striped, callback.ClsPtr()), S_OK);
        const auto mirrored = BtrfsFixture::devices("raid1");
        QVERIFY(mirrored.size() >= 2);
        QCOMPARE(open(mirrored, nullptr), S_OK);
        const UInt32 item = index(name);
        QVERIFY(item != UInt32(-1));
        CMyComPtr2<IArchiveExtractCallback, Extract> output;
        output.Create_if_Empty();
        QCOMPARE(archive_->Extract(&item, 1, 0, output), S_OK);
        QCOMPARE(output->result, Int32(NArchive::NExtract::NOperationResult::kOK));
        QCOMPARE(output->output->bytes, payload());
    }
    void cleanupTestCase() { archive_.Release(); }
};
QTEST_GUILESS_MAIN(BtrfsMultideviceTests)
#include "btrfs_multidevice_tests.moc"
