#include "../third_party/7zip/C/CpuArch.h"
#include "7zip/Common/StreamObjects.h"
#include "7zip/Common/StreamUtils.h"
#include "7zip/PropID.h"
#include "Windows/PropVariant.h"
#include "archivecodeccatalog.h"
#include "archivecodecprobe.h"
#include "archivedevice.h"
#include "archivenativeopen.h"
#include "deviceaccess.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
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
    QString Password;
    HRESULT PasswordResult = E_ABORT;
    HRESULT Open_CheckBreak() override { return Cancel ? E_ABORT : S_OK; }
    HRESULT Open_SetTotal(const UInt64 *, const UInt64 *) override { return Open_CheckBreak(); }
    HRESULT Open_SetCompleted(const UInt64 *, const UInt64 *) override { return Open_CheckBreak(); }
    HRESULT Open_Finished() override { return S_OK; }
    HRESULT Open_CryptoGetTextPassword(BSTR *value) override {
        *value = nullptr;
        if (PasswordResult != S_OK)
            return PasswordResult;
        const auto text = Password.toStdWString();
        *value = SysAllocStringLen(text.data(), (UINT)text.size());
        return *value ? S_OK : E_OUTOFMEMORY;
    }
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
        QByteArray image = process.readAllStandardOutput();
        if (name == "mbr") {
            // The shared disk fixture uses 512-byte LBAs; this device mock has
            // 4096-byte logical sectors. Preserve its actual partition byte spans.
            for (unsigned i = 0; i < 4; ++i) {
                auto *entry = reinterpret_cast<Byte *>(image.data()) + 446 + 16 * i;
                const UInt32 start = GetUi32(entry + 8), count = GetUi32(entry + 12);
                if ((start | count) & 7)
                    return {};
                SetUi32(entry + 8, start / 8);
                SetUi32(entry + 12, count / 8);
            }
        }
        return image;
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
        for (const QString &layout : {"ext2", "ext3", "ext4", "mbr", "gpt", "recover"})
            for (const QString &mode : {"success", "disconnect", "cancel"})
                QTest::newRow(qPrintable(layout + '-' + mode)) << layout << mode;
    }
    void readsSelectedExtPartitionWithoutWholeDiskCopy() {
        QFETCH(QString, layout);
        QFETCH(QString, mode);
        QByteArray disk = fixture(layout == "recover" ? "ext3" : layout);
        QVERIFY(!disk.isEmpty());
        if (layout == "recover")
            disk[1024 + 0x60] = char(quint8(disk[1024 + 0x60]) | 4u);
        auto *input = new CDeviceInput(disk);
        CMyComPtr<IInStream> lease = input;
        CMyComPtr<IInStream> direct;
        QCOMPARE(DeviceInput(lease, quint64(disk.size()), &direct), S_OK);
        ArchiveDevice device(catalog_);
        COpenUi ui;
        QCOMPARE(device.inspect(direct, UInt64(disk.size()), QStringLiteral("USB диск 空"), &ui),
                 S_OK);
        QCOMPARE(device.partitions().size(), layout == "gpt" ? 2 : 1);
        QCOMPARE(device.partitions()[0].offset,
                 UInt64(layout.startsWith("ext") || layout == "recover" ? 0 : 1024 * 1024));
        std::shared_ptr<ArchiveNativeOpen> reader;
        if (layout == "recover") {
            QCOMPARE(device.openPartition(0, &ui, reader), kDeviceJournalRecoveryRequired);
            QVERIFY(!reader);
            CMyComPtr<IInStream> selected, withoutReplay;
            QCOMPARE(device.partitionInput(0, &selected), S_OK);
            QCOMPARE(CreateDeviceInputIgnoringJournal(selected, &withoutReplay), S_OK);
            QCOMPARE(OpenDeviceFilesystem(catalog_, withoutReplay, "USB/Ext", &ui, reader), S_OK);
            std::shared_ptr<ArchiveNativeOpen> rejected;
            QCOMPARE(device.openPartition(0, &ui, rejected), kDeviceJournalRecoveryRequired);
            QVERIFY(!rejected); // Accepting one session never relaxes another.
        } else
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
    void limitsPartitionsToAuthorizedDeviceBoundary() {
        const QByteArray disk = fixture("mbr");
        QVERIFY(!disk.isEmpty());
        auto *source = new CDeviceInput(disk);
        CMyComPtr<IInStream> input = source;
        CMyComPtr<IInStream> direct;
        QCOMPARE(DeviceInput(input, quint64(disk.size()), &direct), S_OK);
        ArchiveDevice device(catalog_);
        const UInt64 authorizedSize = 2 * 1024 * 1024;
        QCOMPARE(device.inspect(direct, authorizedSize, "USB", nullptr), S_OK);
        QCOMPARE(device.partitions().size(), 1);
        QCOMPARE(device.partitions()[0].offset, UInt64(1024 * 1024));
        QCOMPARE(device.partitions()[0].size, authorizedSize - device.partitions()[0].offset);
        CMyComPtr<IInStream> partition;
        QCOMPARE(device.partitionInput(0, &partition), S_OK);
        QCOMPARE(partition->Seek(-1, STREAM_SEEK_END, nullptr), S_OK);
        char tail[2];
        UInt32 count = 99;
        QCOMPARE(partition->Read(tail, sizeof(tail), &count), S_OK);
        QCOMPARE(count, UInt32(1));
        QVERIFY(source->FarthestRead <= authorizedSize);
    }
    void refusesUnsupportedFeaturesOnlyForDirectDevices_data() {
        QTest::addColumn<int>("field");
        QTest::addColumn<int>("bit");
        QTest::addColumn<QString>("feature");
        QTest::addColumn<bool>("recoveryAlso");
        QTest::newRow("journal-recovery") << 0x60 << 2 << QString("RECOVER") << false;
        QTest::newRow("encrypted-recovery") << 0x60 << 16 << QString("RECOVER") << true;
    }
    void refusesUnsupportedFeaturesOnlyForDirectDevices() {
        QFETCH(int, field);
        QFETCH(int, bit);
        QFETCH(QString, feature);
        QFETCH(bool, recoveryAlso);
        QByteArray disk = fixture("ext4");
        QVERIFY(disk.size() > 2048);
        // Controlled declarations of recover/encrypt with no changed file data.
        // Disable optional checksums so declaration changes reach device policy.
        disk[1024 + 0x65] = char(quint8(disk[1024 + 0x65]) & ~4u);
        const int byte = 1024 + field + bit / 8;
        disk[byte] = char(quint8(disk[byte]) | (1u << (bit % 8)));
        if (recoveryAlso)
            disk[1024 + 0x60] = char(quint8(disk[1024 + 0x60]) | 4u);
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
        QCOMPARE(device.openPartition(0, nullptr, reader, &error), kDeviceJournalRecoveryRequired);
        QVERIFY2(error.contains(feature), qPrintable(error));
        QVERIFY(!reader);
        CMyComPtr<IInStream> direct, withoutReplay;
        QCOMPARE(DeviceInput(input, quint64(disk.size()), &direct), S_OK);
        QCOMPARE(CreateDeviceInputIgnoringJournal(direct, &withoutReplay), S_OK);
        QCOMPARE(OpenDeviceFilesystem(catalog_, withoutReplay, "USB", nullptr, reader, &error),
                 S_OK);
    }
    void signedVerityPropertiesReachAlignedDeviceStreams() {
        QFile manifest(QStringLiteral(DEVICE_FIXTURE_DIR "/../ext4/verity-signatures.json"));
        QVERIFY(manifest.open(QIODevice::ReadOnly));
        const auto fixtures = QJsonDocument::fromJson(manifest.readAll()).object();
        const auto record = fixtures["cases"].toArray().first().toObject();
        QByteArray compressed(4, '\0');
        SetBe32(compressed.data(), UInt32(record["size"].toInt()));
        compressed += QByteArray::fromBase64(record["image"].toString().toLatin1());
        const auto bytes = qUncompress(compressed);
        QVERIFY(!bytes.isEmpty());
        CMyComPtr<IInStream> raw = new CDeviceInput(bytes), direct;
        QCOMPARE(DeviceInput(raw, quint64(bytes.size()), &direct), S_OK);
        CObjectVector<CProperty> properties;
        properties.AddNew().Name = L"verity.require+";
        auto &certificate = properties.AddNew();
        certificate.Name = L"verity.trust";
        certificate.Value = fixtures["root"].toString().toStdWString().c_str();
        for (bool trusted : {false, true}) {
            COpenOptions options;
            options.props = trusted ? &properties : nullptr;
            std::shared_ptr<ArchiveNativeOpen> reader;
            QCOMPARE(OpenDeviceFilesystem(catalog_, direct, "signed USB", nullptr, reader, nullptr,
                                          &options),
                     S_OK);
            CMyComPtr<IInArchiveGetStream> streams;
            QCOMPARE(
                reader->arc().Archive->QueryInterface(IID_IInArchiveGetStream, (void **)&streams),
                S_OK);
            UInt32 count = 0;
            QCOMPARE(reader->arc().Archive->GetNumberOfItems(&count), S_OK);
            UInt32 index = 0;
            for (; index < count; ++index) {
                NWindows::NCOM::CPropVariant path;
                QCOMPARE(reader->arc().Archive->GetProperty(index, kpidPath, &path), S_OK);
                if (path.vt == VT_BSTR && QDir::fromNativeSeparators(QString::fromWCharArray(
                                              path.bstrVal)) == fixtures["path"].toString())
                    break;
            }
            QVERIFY(index < count);
            CMyComPtr<ISequentialInStream> stream;
            const HRESULT status = streams->GetStream(index, &stream);
            if (!trusted) {
                QVERIFY(status != S_OK);
                QVERIFY(!stream);
                continue;
            }
            QCOMPARE(status, S_OK);
            reader.reset();
            streams.Release();
            QByteArray actual(record["file_size"].toInt(), '\0');
            size_t amount = size_t(actual.size());
            QCOMPARE(ReadStream(stream, actual.data(), &amount), S_OK);
            QCOMPARE(amount, size_t(actual.size()));
            QCOMPARE(QCryptographicHash::hash(actual, QCryptographicHash::Sha256).toHex(),
                     record["sha256"].toString().toLatin1());
        }
    }
    void encryptedLinuxFilesThroughAlignedDeviceInput() {
        const QDir fixtures(QStringLiteral(DEVICE_FIXTURE_DIR "/../fscrypt"));
        QFile manifest(fixtures.filePath("native.json"));
        QVERIFY(manifest.open(QIODevice::ReadOnly));
        const auto record = QJsonDocument::fromJson(manifest.readAll()).object();
        QFile packed(fixtures.filePath("native.ext4.zlib"));
        QVERIFY(packed.open(QIODevice::ReadOnly));
        QByteArray compressed(4, '\0');
        const auto size = UInt32(record["image_size"].toDouble());
        SetBe32(compressed.data(), size);
        compressed += packed.readAll();
        const auto bytes = qUncompress(compressed);
        QVERIFY(!bytes.isEmpty());
        QStringList keys;
        for (const auto &entry : record["cases"].toArray()) {
            const auto value = entry.toObject();
            keys << (value["key_type"].toInt() == 1 ? "v1:" + value["descriptor"].toString() + ":"
                                                    : "v2:") +
                        value["raw_key"].toString();
        }
        CMyComPtr<IInStream> raw = new CDeviceInput(bytes), direct;
        QCOMPARE(DeviceInput(raw, quint64(bytes.size()), &direct), S_OK);
        COpenUi ui;
        std::shared_ptr<ArchiveNativeOpen> reader;
        QCOMPARE(OpenDeviceFilesystem(catalog_, direct, "encrypted USB", &ui, reader), E_ABORT);
        QVERIFY(!reader);
        ui.PasswordResult = S_OK;
        ui.Password = "fscrypt:" + keys.join(';');
        QCOMPARE(OpenDeviceFilesystem(catalog_, direct, "encrypted USB", &ui, reader), S_OK);
        IInArchive *archive = reader->arc().Archive;
        CMyComPtr<IInArchiveGetStream> streams;
        QCOMPARE(archive->QueryInterface(IID_IInArchiveGetStream, (void **)&streams), S_OK);
        UInt32 count = 0;
        QCOMPARE(archive->GetNumberOfItems(&count), S_OK);
        for (const auto &entry : record["cases"].toArray()) {
            const auto value = entry.toObject();
            const auto expected = value["name"].toString() + '/' + value["filename"].toString();
            UInt32 index = 0;
            for (; index < count; ++index) {
                NWindows::NCOM::CPropVariant path;
                QCOMPARE(archive->GetProperty(index, kpidPath, &path), S_OK);
                if (path.vt == VT_BSTR &&
                    QDir::fromNativeSeparators(QString::fromWCharArray(path.bstrVal)) == expected)
                    break;
            }
            QVERIFY2(index < count, qPrintable(expected));
            CMyComPtr<ISequentialInStream> sequential;
            QCOMPARE(streams->GetStream(index, &sequential), S_OK);
            CMyComPtr<IInStream> input;
            QCOMPARE(sequential->QueryInterface(IID_IInStream, (void **)&input), S_OK);
            QCOMPARE(input->Seek(8192, STREAM_SEEK_SET, nullptr), S_OK);
            char tail[3] = {};
            UInt32 done = 0;
            QCOMPARE(input->Read(tail, 3, &done), S_OK);
            QCOMPARE(done, UInt32(3));
            QCOMPARE(QByteArray(tail, 3), QByteArray("end"));
        }
    }
};

QTEST_GUILESS_MAIN(ArchiveDeviceTests)
#include "archive_device_tests.moc"
