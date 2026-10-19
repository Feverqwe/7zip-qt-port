#include "7zip/Common/StreamObjects.h"
#include "7zip/PropID.h"
#include "Windows/PropVariant.h"
#include "archivecodeccatalog.h"
#include "archivecodecprobe.h"
#include "archivedevice.h"
#include "archivenativeopen.h"
#include "deviceaccess.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

namespace {
class CDeviceInput final : public IInStream, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(ISequentialInStream, IInStream)
    CMyComPtr<IInStream> stream_;
    UInt64 position_ = 0;

  public:
    bool Disconnected = false;
    UInt64 FarthestRead = 0;
    explicit CDeviceInput(const QByteArray &data) {
        auto *input = new CBufInStream;
        stream_ = input;
        input->Init(reinterpret_cast<const Byte *>(data.constData()), size_t(data.size()));
    }
    Z7_COM7F_IMF(Read(void *data, UInt32 size, UInt32 *processed)) override {
        if (processed)
            *processed = 0;
        if (Disconnected)
            return E_FAIL;
        UInt32 count = 0;
        const HRESULT result = stream_->Read(data, size, &count);
        position_ += count;
        FarthestRead = qMax(FarthestRead, position_);
        if (processed)
            *processed = count;
        return result;
    }
    Z7_COM7F_IMF(Seek(Int64 offset, UInt32 origin, UInt64 *position)) override {
        const HRESULT result = stream_->Seek(offset, origin, &position_);
        if (position)
            *position = position_;
        return result;
    }
};

class CAlignedHandle final : public DeviceReadHandle {
    CMyComPtr<IInStream> input_;

  public:
    explicit CAlignedHandle(IInStream *input) : input_(input) {}
    qint32 readAt(quint64 offset, void *data, quint32 size, quint32 &read) override {
        if (offset % 4096 || size % 4096 || quintptr(data) % 4096)
            return E_INVALIDARG;
        RINOK(input_->Seek(Int64(offset), STREAM_SEEK_SET, nullptr))
        return input_->Read(data, size, &read);
    }
};

HRESULT DeviceInput(IInStream *input, quint64 size, IInStream **stream) {
    ExternalDevice description;
    description.size = size;
    description.sectorSize = 4096;
    return CreateDeviceInput(description, std::make_shared<CAlignedHandle>(input), stream);
}

class COpenUi final : public IOpenCallbackUI {
  public:
    bool Cancel = false;
    HRESULT Open_CheckBreak() override { return Cancel ? E_ABORT : S_OK; }
    HRESULT Open_SetTotal(const UInt64 *, const UInt64 *) override { return Open_CheckBreak(); }
    HRESULT Open_SetCompleted(const UInt64 *, const UInt64 *) override { return Open_CheckBreak(); }
    HRESULT Open_Finished() override { return S_OK; }
    HRESULT Open_CryptoGetTextPassword(BSTR *) override { return E_ABORT; }
};

class CExtractUi final : public IArchiveExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IArchiveExtractCallback)

  public:
    bool Cancel = false;
    Int32 ItemResult = -1;
    CDynBufSeqOutStream *Output = new CDynBufSeqOutStream;
    CMyComPtr<ISequentialOutStream> Lease = Output;
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return Cancel ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(GetStream(UInt32, ISequentialOutStream **stream, Int32)) override {
        *stream = Lease;
        (*stream)->AddRef();
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(Int32)) override { return S_OK; }
    Z7_COM7F_IMF(SetOperationResult(Int32 result)) override {
        ItemResult = result;
        return S_OK;
    }
    QByteArray bytes() const {
        return QByteArray(reinterpret_cast<const char *>(Output->GetBuffer()),
                          qsizetype(Output->GetSize()));
    }
};
} // namespace

class ArchiveDeviceTests final : public QObject {
    Q_OBJECT
    std::shared_ptr<const ArchiveCodecCatalog> catalog_;
    QString engine_;

    QByteArray fixture(const QString &name) const {
        QProcess process;
        process.start(
            engine_, {"x", "-so", "--",
                      QStringLiteral(DEVICE_FIXTURE_DIR) + '/' + name + QStringLiteral(".img.gz")});
        if (!process.waitForFinished(10000) || process.exitCode() != 0)
            return {};
        return process.readAllStandardOutput();
    }

  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; direct-device engine integration unverified");
#else
        const QString module = QStringLiteral(SEVENZIP_CODEC_MODULE);
        engine_ = QFileInfo(module).dir().filePath(
#ifdef Q_OS_WIN
            "7z.exe"
#else
            "7z"
#endif
        );
        ArchiveCodecProbe probe;
        bool done = false;
        QVERIFY(probe.Start(engine_, module, [&](auto result) {
            catalog_ = result.catalog;
            done = true;
        }));
        QTRY_VERIFY(done);
        QVERIFY(catalog_);
        QVERIFY2(catalog_->result() == S_OK, qPrintable(catalog_->error()));
#endif
    }
    void readsSelectedExtPartitionWithoutWholeDiskCopy_data() {
        QTest::addColumn<QString>("layout");
        QTest::addColumn<QString>("mode");
        for (const QString &layout : {"ext2", "ext3", "ext4", "mbr", "gpt"})
            for (const QString &mode : {"success", "disconnect", "cancel"})
                QTest::newRow(qPrintable(layout + '-' + mode)) << layout << mode;
    }
    void readsSelectedExtPartitionWithoutWholeDiskCopy() {
        QFETCH(QString, layout);
        QFETCH(QString, mode);
        const QByteArray disk = fixture(layout);
        QVERIFY(!disk.isEmpty());
        auto *input = new CDeviceInput(disk);
        CMyComPtr<IInStream> lease = input;
        CMyComPtr<IInStream> direct;
        QCOMPARE(DeviceInput(lease, quint64(disk.size()), &direct), S_OK);
        ArchiveDevice device(catalog_);
        COpenUi ui;
        QCOMPARE(device.inspect(direct, UInt64(disk.size()), QStringLiteral("USB диск 空"), &ui),
                 S_OK);
        QCOMPARE(device.partitions().size(), layout == "gpt" ? 2 : 1);
        QCOMPARE(device.partitions()[0].offset, UInt64(layout.startsWith("ext") ? 0 : 1024 * 1024));
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(device.openPartition(0, &ui, reader), S_OK);
        QVERIFY(reader);
        // The reader owns its input/module after the discovery owner is reset.
        device = ArchiveDevice(catalog_);
        UInt32 count = 0;
        QCOMPARE(reader->arc().Archive->GetNumberOfItems(&count), S_OK);
        UInt32 fileIndex = count;
        for (UInt32 index = 0; index < count; ++index) {
            NWindows::NCOM::CPropVariant path;
            QCOMPARE(reader->arc().Archive->GetProperty(index, kpidPath, &path), S_OK);
            if (path.vt == VT_BSTR &&
                QDir::fromNativeSeparators(QString::fromWCharArray(path.bstrVal)) ==
                    QStringLiteral("папка 空/данные с пробелом.txt"))
                fileIndex = index;
        }
        QVERIFY(fileIndex < count);
        auto *extracted = new CExtractUi;
        CMyComPtr<IArchiveExtractCallback> callback = extracted;
        input->Disconnected = mode == "disconnect";
        extracted->Cancel = mode == "cancel";
        const HRESULT result = reader->arc().Archive->Extract(&fileIndex, 1, 0, callback);
        if (mode == "cancel")
            QCOMPARE(result, E_ABORT);
        else if (mode == "disconnect")
            QVERIFY(result != S_OK ||
                    extracted->ItemResult != NArchive::NExtract::NOperationResult::kOK);
        else {
            QCOMPARE(result, S_OK);
            QCOMPARE(extracted->ItemResult, Int32(NArchive::NExtract::NOperationResult::kOK));
            QCOMPARE(extracted->bytes(), QByteArray("device ext4 payload\n"));
        }
        QVERIFY(input->FarthestRead <= UInt64(disk.size()));
    }
    void cancellationAndUnsupportedPartitionDoNotPublishReader() {
        const QByteArray disk = fixture("gpt");
        QVERIFY(!disk.isEmpty());
        CMyComPtr<IInStream> input = new CDeviceInput(disk);
        CMyComPtr<IInStream> direct;
        QCOMPARE(DeviceInput(input, quint64(disk.size()), &direct), S_OK);
        ArchiveDevice device(catalog_);
        COpenUi ui;
        ui.Cancel = true;
        QCOMPARE(device.inspect(direct, UInt64(disk.size()), "USB", &ui), E_ABORT);
        QVERIFY(device.partitions().isEmpty());
        ui.Cancel = false;
        QCOMPARE(device.inspect(direct, UInt64(disk.size()), "USB", &ui), S_OK);
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(device.openPartition(1, &ui, reader), S_FALSE);
        QVERIFY(!reader);
        ui.Cancel = true;
        QCOMPARE(device.openPartition(0, &ui, reader), E_ABORT);
        QVERIFY(!reader);
        QCOMPARE(device.openPartition(2, nullptr, reader), E_INVALIDARG);
    }
    void readsModernLinuxLayoutsFromDirectDevices_data() {
        QTest::addColumn<QString>("layout");
        for (const auto &name : {"inline", "bigalloc", "meta-bg", "verity"})
            QTest::newRow(name) << QString::fromLatin1(name);
    }
    void readsModernLinuxLayoutsFromDirectDevices() {
        QFETCH(QString, layout);
        const QDir directory(QStringLiteral(DEVICE_FIXTURE_DIR "/../ext4"));
        QFile manifest(directory.filePath("manifest.json"));
        QVERIFY(manifest.open(QIODevice::ReadOnly));
        const auto info = QJsonDocument::fromJson(manifest.readAll())
                              .object()["images"]
                              .toObject()[layout]
                              .toObject();
        QFile compressed(directory.filePath(layout + ".ext4.zlib"));
        QVERIFY(compressed.open(QIODevice::ReadOnly));
        const UInt32 size = UInt32(info["size"].toDouble());
        QByteArray encoded;
        for (int shift : {24, 16, 8, 0})
            encoded.append(char(size >> shift));
        encoded.append(compressed.readAll());
        const auto bytes = qUncompress(encoded);
        QCOMPARE(bytes.size(), qsizetype(size));
        CMyComPtr<IInStream> input = new CDeviceInput(bytes);
        ArchiveDevice device(catalog_);
        QCOMPARE(device.inspect(input, size, "USB", nullptr), S_OK);
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(device.openPartition(0, nullptr, reader), S_OK);
        QVERIFY(reader);
        UInt32 count = 0, index = UInt32(-1);
        QCOMPARE(reader->arc().Archive->GetNumberOfItems(&count), S_OK);
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant path;
            QCOMPARE(reader->arc().Archive->GetProperty(i, kpidPath, &path), S_OK);
            if (path.vt == VT_BSTR &&
                QString::fromWCharArray(path.bstrVal) == QStringLiteral("данные 空.bin"))
                index = i;
        }
        QVERIFY(index != UInt32(-1));
        auto *output = new CExtractUi;
        CMyComPtr<IArchiveExtractCallback> callback = output;
        QCOMPARE(reader->arc().Archive->Extract(&index, 1, 0, callback), S_OK);
        QCOMPARE(output->ItemResult, Int32(NArchive::NExtract::NOperationResult::kOK));
        QByteArray expected;
        if (layout == "verity") {
            expected = QByteArray(35 * 1024 + 17, '\0');
            expected.replace(2 * 1024, 1024, QByteArray(1024, 'A'));
            expected.replace(35 * 1024, 4, "tail");
        } else {
            for (int i = 0; i < 100; ++i)
                expected.append(char(i));
        }
        QCOMPARE(output->bytes(), expected);
    }
    void rejectsPartitionsBeyondAuthorizedDeviceBoundary() {
        const QByteArray disk = fixture("mbr");
        QVERIFY(!disk.isEmpty());
        auto *source = new CDeviceInput(disk);
        CMyComPtr<IInStream> input = source;
        CMyComPtr<IInStream> direct;
        QCOMPARE(DeviceInput(input, quint64(disk.size()), &direct), S_OK);
        ArchiveDevice device(catalog_);
        const UInt64 authorizedSize = 2 * 1024 * 1024;
        QVERIFY(device.inspect(direct, authorizedSize, "USB", nullptr) != S_OK);
        QVERIFY(device.partitions().isEmpty());
        QVERIFY(source->FarthestRead <= authorizedSize);
    }
    void refusesUnsupportedFeaturesOnlyForDirectDevices_data() {
        QTest::addColumn<int>("field");
        QTest::addColumn<int>("bit");
        QTest::addColumn<QString>("feature");
        QTest::newRow("journal-recovery") << 0x60 << 2 << QString("RECOVER");
        QTest::newRow("encryption") << 0x60 << 16 << QString("ENCRYPT");
    }
    void refusesUnsupportedFeaturesOnlyForDirectDevices() {
        QFETCH(int, field);
        QFETCH(int, bit);
        QFETCH(QString, feature);
        QByteArray disk = fixture("ext4");
        QVERIFY(disk.size() > 2048);
        // Controlled declarations of recover/encrypt with no changed file data.
        // Disable optional checksums so declaration changes reach device policy.
        disk[1024 + 0x65] = char(quint8(disk[1024 + 0x65]) & ~4u);
        const int byte = 1024 + field + bit / 8;
        disk[byte] = char(quint8(disk[byte]) | (1u << (bit % 8)));
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath("declared-feature.img");
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(disk), qint64(disk.size()));
        file.close();
        ArchiveNativeOpen ordinary(catalog_);
        QCOMPARE(ordinary.Open(path, "Ext", nullptr), S_OK);
        CMyComPtr<IInStream> input = new CDeviceInput(disk);
        ArchiveDevice device(catalog_);
        QCOMPARE(device.inspect(input, UInt64(disk.size()), "USB", nullptr), S_OK);
        std::shared_ptr<ArchiveNativeOpen> reader;
        QString error;
        QCOMPARE(device.openPartition(0, nullptr, reader, &error), E_NOTIMPL);
        QVERIFY2(error.contains(feature), qPrintable(error));
        QVERIFY(!reader);
    }
};

QTEST_GUILESS_MAIN(ArchiveDeviceTests)
#include "archive_device_tests.moc"
