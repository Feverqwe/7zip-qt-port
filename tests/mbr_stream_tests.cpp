#include "7zip/Archive/IArchive.h"
#include "7zip/Common/StreamUtils.h"
#include "Common/MyCom.h"
#include "Windows/PropVariant.h"
#include "archivecodeccatalog.h"
#include "archivecodecprobe.h"
#include "archivedevice.h"
#include "deviceaccess.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

namespace {
class Handle final : public DeviceReadHandle {
    QByteArray bytes_;
    quint32 sector_;

  public:
    HRESULT failure = E_FAIL;
    quint64 failAt = UINT64_MAX;
    Handle(QByteArray bytes, quint32 sector) : bytes_(std::move(bytes)), sector_(sector) {}
    qint32 readAt(quint64 offset, void *data, quint32 size, quint32 &read) override {
        read = 0;
        if (offset == failAt)
            return failure;
        if (offset % sector_ || size % sector_ || quintptr(data) % sector_ ||
            offset > quint64(bytes_.size()) || size > quint64(bytes_.size()) - offset)
            return E_INVALIDARG;
        memcpy(data, bytes_.constData() + offset, size);
        read = size;
        return S_OK;
    }
};
QString core() { return QString::fromUtf8(SEVENZIP_MBR_MODULE); }
} // namespace

class MbrStreamTests final : public QObject {
    Q_OBJECT
    QLibrary module_{core()};
    CMyComPtr<IInArchive> archive_;
    QByteArray image_, payload_;
    quint64 firstOffset_ = 0;

    void fixture(const QString &kind) {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString path = temporary.filePath("disk.mbr");
        QProcess process;
        const bool nested = kind.startsWith("nested-");
        const bool classification = kind.startsWith("classify-");
        process.start(QString::fromUtf8(SEVENZIP_MBR_PYTHON),
                      {QString::fromUtf8(nested           ? SEVENZIP_MBR_SUBTYPE_FIXTURE
                                         : classification ? SEVENZIP_MBR_CLASSIFICATION_FIXTURE
                                                          : SEVENZIP_MBR_FIXTURE),
                       nested           ? kind.mid(7)
                       : classification ? kind.mid(9)
                                        : kind,
                       path});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitCode(), 0);
        const auto part =
            QJsonDocument::fromJson(process.readAllStandardOutput()).array()[0].toObject();
        payload_ = QByteArray::fromHex(part["payload"].toString().toLatin1());
        firstOffset_ = quint64(part["offset"].toDouble());
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        image_ = file.readAll();
    }
    CMyComPtr<IInStream> input(const std::shared_ptr<Handle> &handle, quint32 sector) {
        ExternalDevice description;
        description.size = quint64(image_.size());
        description.sectorSize = sector;
        CMyComPtr<IInStream> result;
        if (CreateDeviceInput(description, handle, &result) != S_OK)
            return {};
        return result;
    }
    void checkPartition(UInt32 index = 0) {
        NWindows::NCOM::CPropVariant offset;
        QCOMPARE(archive_->GetProperty(index, kpidOffset, &offset), S_OK);
        QCOMPARE(offset.vt, VARTYPE(VT_UI8));
        QCOMPARE(offset.uhVal.QuadPart, UInt64(firstOffset_));
        CMyComPtr<IInArchiveGetStream> provider;
        QCOMPARE(
            archive_->QueryInterface(IID_IInArchiveGetStream, reinterpret_cast<void **>(&provider)),
            S_OK);
        CMyComPtr<ISequentialInStream> data;
        QCOMPARE(provider->GetStream(index, &data), S_OK);
        QVERIFY(data);
        QByteArray actual(payload_.size(), Qt::Uninitialized);
        UInt32 count = 0;
        QCOMPARE(data->Read(actual.data(), UInt32(actual.size()), &count), S_OK);
        QCOMPARE(count, UInt32(actual.size()));
        QCOMPARE(actual, payload_);
    }
  private slots:
    void init() {
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
            if (name.vt != VT_BSTR || QString::fromWCharArray(name.bstrVal) != "MBR")
                continue;
            QCOMPARE(property(i, NArchive::NHandlerPropID::kClassID, &id), S_OK);
            GUID guid;
            memcpy(&guid, id.bstrVal, sizeof(guid));
            QCOMPARE(create(&guid, &IID_IInArchive, reinterpret_cast<void **>(&archive_)), S_OK);
            return;
        }
        QFAIL("Prepared engine has no MBR handler");
    }
    void cleanup() { archive_.Release(); }
    void geometry_data() {
        QTest::addColumn<quint32>("sector");
        for (unsigned log = 9; log <= 16; ++log)
            QTest::newRow(qPrintable(QString::number(1u << log))) << quint32(1u << log);
    }
    void geometry() {
        QFETCH(quint32, sector);
        fixture("device-" + QString::number(sector));
        const auto handle = std::make_shared<Handle>(image_, sector);
        auto source = input(handle, sector);
        QVERIFY(source);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        checkPartition();
    }
    void failedOpenAndReuse() {
        fixture("extended-5");
        const auto handle = std::make_shared<Handle>(image_, 512);
        auto source = input(handle, 512);
        QVERIFY(source);
        handle->failAt = 40 * 512;
        for (HRESULT error : {E_FAIL, E_ABORT}) {
            handle->failure = error;
            QCOMPARE(archive_->Open(source, nullptr, nullptr), error);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        handle->failAt = UINT64_MAX;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        checkPartition();
    }
    void nestedGeometry_data() {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<quint32>("sector");
        for (const char *kind :
             {"freebsd", "netbsd", "openbsd", "solaris-new", "unixware", "minix"}) {
            QTest::newRow(kind) << QString::fromLatin1(kind) << quint32(512);
            const QString large = QString::fromLatin1(kind) + "-4kn";
            QTest::newRow(qPrintable(large)) << large << quint32(4096);
        }
    }
    void nestedGeometry() {
        QFETCH(QString, kind);
        QFETCH(quint32, sector);
        fixture("nested-" + kind);
        const auto handle = std::make_shared<Handle>(image_, sector);
        auto source = input(handle, sector);
        QVERIFY(source);
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        UInt32 count = 0;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant offset;
            QCOMPARE(archive_->GetProperty(i, kpidOffset, &offset), S_OK);
            if (offset.vt == VT_UI8 && offset.uhVal.QuadPart == firstOffset_) {
                checkPartition(i);
                return;
            }
        }
        QFAIL("Nested partition was not published");
    }
    void nestedReadFailureAndReuse() {
        fixture("nested-freebsd");
        const auto handle = std::make_shared<Handle>(image_, 512);
        auto source = input(handle, 512);
        QVERIFY(source);
        handle->failAt = 17 * 512;
        for (HRESULT error : {E_FAIL, E_ABORT}) {
            handle->failure = error;
            QCOMPARE(archive_->Open(source, nullptr, nullptr), error);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        handle->failAt = UINT64_MAX;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
    }
    void selectedNestedDeviceView() {
        fixture("nested-freebsd-4kn");
        const QDir runtime(QString::fromUtf8(SEVENZIP_MBR_RUNTIME));
        ArchiveCodecProbe probe;
        std::shared_ptr<const ArchiveCodecCatalog> catalog;
        bool ready = false;
#ifdef Q_OS_WIN
        const QString engine = runtime.filePath("7z.exe");
        const QString mainModule = runtime.filePath("7z.dll");
#else
        const QString engine = runtime.filePath("7z");
        const QString mainModule = runtime.filePath("7z.so");
#endif
        QVERIFY(probe.Start(mainModule, [&](auto result) {
            catalog = result.catalog;
            ready = true;
        }));
        QTRY_VERIFY(ready);
        QVERIFY(catalog);
        QCOMPARE(catalog->result(), S_OK);
        CMyComPtr<IInStream> selected;
        {
            const auto handle = std::make_shared<Handle>(image_, 4096);
            auto source = input(handle, 4096);
            QVERIFY(source);
            ArchiveDevice device(catalog);
            QCOMPARE(device.inspect(source, UInt64(image_.size()), QString::fromUtf8("диск 空"),
                                    nullptr),
                     S_OK);
            int match = -1;
            for (int i = 0; i < device.partitions().size(); ++i)
                if (device.partitions()[i].offset == firstOffset_)
                    match = i;
            QVERIFY(match >= 0);
            QCOMPARE(device.partitionInput(match, &selected), S_OK);
        }
        QByteArray bytes(payload_.size() + 16, Qt::Uninitialized);
        size_t count = size_t(bytes.size());
        QCOMPARE(ReadStream(selected, bytes.data(), &count), S_OK);
        QCOMPARE(count, size_t(payload_.size()));
        QCOMPARE(bytes.left(qsizetype(count)), payload_);
    }
    void classificationErrorsAndReuse() {
        fixture("classify-aix-no-lvm");
        image_.replace(7 * 512, 4, "_LVM");
        auto handle = std::make_shared<Handle>(image_, 512);
        auto source = input(handle, 512);
        QVERIFY(source);
        handle->failAt = 7 * 512;
        for (HRESULT error : {E_FAIL, E_ABORT}) {
            handle->failure = error;
            QCOMPARE(archive_->Open(source, nullptr, nullptr), error);
            UInt32 count = 99;
            QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(0));
        }
        handle->failAt = UINT64_MAX;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_FALSE);
        fixture("classify-aix-linux-131");
        handle = std::make_shared<Handle>(image_, 512);
        source = input(handle, 512);
        handle->failAt = 7 * 512;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        checkPartition();
    }
    void optionalProbeCancellation() {
        fixture("primary");
        image_[450] = 7;
        const auto handle = std::make_shared<Handle>(image_, 512);
        auto source = input(handle, 512);
        handle->failAt = 16 * 512;
        handle->failure = E_FAIL;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        handle->failure = E_ABORT;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), E_ABORT);
        UInt32 count = 99;
        QCOMPARE(archive_->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        handle->failAt = UINT64_MAX;
        QCOMPARE(archive_->Open(source, nullptr, nullptr), S_OK);
        checkPartition();
    }
};
QTEST_GUILESS_MAIN(MbrStreamTests)
#include "mbr_stream_tests.moc"
