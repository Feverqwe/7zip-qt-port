#include "CPP/7zip/UI/Agent/Agent.h"
#include "Common/StringConvert.h"
#include "archivebackend.h"
#include "archivedevice.h"
#include "deviceaccess.h"
#include "qtarchivefoldermanager.h"
#include "qtarchiveproperties.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>
#include <cstring>
#include <future>

namespace {
class CMemoryDevice final : public DeviceReadHandle {
  public:
    QByteArray Bytes;
    bool Disconnected = false;
    quint64 TotalRead = 0;
    explicit CMemoryDevice(QByteArray bytes) : Bytes(std::move(bytes)) {}
    qint32 readAt(quint64 offset, void *data, quint32 size, quint32 &read) override {
        read = 0;
        if (Disconnected)
            return E_FAIL;
        if (offset > quint64(Bytes.size()) || size > quint64(Bytes.size()) - offset ||
            offset % 4096 || size % 4096 || quintptr(data) % 4096)
            return E_INVALIDARG;
        memcpy(data, Bytes.constData() + offset, size);
        TotalRead += size;
        read = size;
        return S_OK;
    }
};
class CObservedBackend final : public ArchiveBackend {
  public:
    unsigned Listings = 0;
    using ArchiveBackend::ArchiveBackend;
    void openFolder(const QString &path, const QString &type, const QString &password) override {
        ++Listings;
        ArchiveBackend::openFolder(path, type, password);
    }
};
class CCopyCallback final : public IFolderOperationsExtractCallback,
                            public IFolderArchiveExtractCallback,
                            public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_2(IFolderOperationsExtractCallback, IFolderArchiveExtractCallback)
  public:
    bool Cancel = false;
    unsigned Errors = 0;
    Z7_COM7F_IMF(SetTotal(UInt64)) override { return Cancel ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *)) override { return Cancel ? E_ABORT : S_OK; }
    Z7_COM7F_IMF(SetCurrentFilePath(const wchar_t *)) override { return S_OK; }
    Z7_COM7F_IMF(SetNumFiles(UInt64)) override { return S_OK; }
    Z7_COM7F_IMF(ShowMessage(const wchar_t *)) override {
        ++Errors;
        return S_OK;
    }
    Z7_COM7F_IMF(AskWrite(const wchar_t *, Int32, const FILETIME *, const UInt64 *,
                          const wchar_t *path, BSTR *result, Int32 *answer))
    override {
        *answer = 1;
        return StringToBstr(path, result);
    }
    Z7_COM7F_IMF(AskOverwrite(const wchar_t *, const FILETIME *, const UInt64 *, const wchar_t *,
                              const FILETIME *, const UInt64 *, Int32 *answer))
    override {
        *answer = NOverwriteAnswer::kYes;
        return S_OK;
    }
    Z7_COM7F_IMF(PrepareOperation(const wchar_t *, Int32, Int32, const UInt64 *)) override {
        return Cancel ? E_ABORT : S_OK;
    }
    Z7_COM7F_IMF(MessageError(const wchar_t *)) override {
        ++Errors;
        return S_OK;
    }
    Z7_COM7F_IMF(SetOperationResult(Int32 result, Int32)) override {
        if (result)
            ++Errors;
        return S_OK;
    }
};
} // namespace

class DeviceFolderTests final : public QObject {
    Q_OBJECT
    QString engine_;
    QString module_;

    QByteArray fixture() const {
        QProcess process;
        process.start(engine_,
                      {"x", "-so", "--", QStringLiteral(DEVICE_FIXTURE_DIR "/gpt.img.gz")});
        if (!process.waitForFinished(10000) || process.exitCode())
            return {};
        return process.readAllStandardOutput();
    }
    static HRESULT worker(const std::function<HRESULT()> &action) {
        return std::async(std::launch::async, action).get();
    }
  private slots:
    void initTestCase() {
#ifndef SEVENZIP_CODEC_MODULE
        QSKIP("Official module unavailable; direct-device folder integration unverified");
#else
        module_ = QStringLiteral(SEVENZIP_CODEC_MODULE);
        engine_ = QFileInfo(module_).dir().filePath(
#ifdef Q_OS_WIN
            "7z.exe"
#else
            "7z"
#endif
        );
#endif
    }
    void sourceFolderCopiesSelectedUnicodeFile_data() {
        QTest::addColumn<QString>("mode");
        QTest::addColumn<bool>("allowIncompleteRead");
        for (bool allowIncompleteRead : {false, true})
            for (const QString &mode :
                 {"success", "refresh", "cancel", "disconnect", "cancel-open", "disconnect-open"})
                QTest::newRow(qPrintable(mode + (allowIncompleteRead ? "-journal" : "")))
                    << mode << allowIncompleteRead;
    }
    void sourceFolderCopiesSelectedUnicodeFile() {
        QFETCH(QString, mode);
        QFETCH(bool, allowIncompleteRead);
        CObservedBackend backend(nullptr, engine_);
        QSignalSpy ready(&backend, &ArchiveBackend::codecCatalogReady);
        QVERIFY(backend.discoverCodecCatalog(module_));
        QTRY_COMPARE(ready.size(), 1);
        QVERIFY(ready[0][0].toBool());
        QByteArray bytes = fixture();
        QVERIFY(!bytes.isEmpty());
        if (allowIncompleteRead) {
            const int superblock = 1024 * 1024 + 1024;
            bytes[superblock + 0x65] = char(quint8(bytes[superblock + 0x65]) & ~4u);
            bytes[superblock + 0x60] = char(quint8(bytes[superblock + 0x60]) | 4u);
        }
        auto handle = std::make_shared<CMemoryDevice>(bytes);
        ExternalDevice description;
        description.size = quint64(bytes.size());
        description.sectorSize = 4096;
        CMyComPtr<IInStream> input, selected;
        QCOMPARE(CreateDeviceInput(description, handle, &input), S_OK);
        ArchiveDevice device(backend.codecCatalog());
        QCOMPARE(device.inspect(input, description.size, QStringLiteral("USB 空"), nullptr), S_OK);
        QCOMPARE(device.partitionInput(0, &selected), S_OK);
        if (allowIncompleteRead) {
            CMyComPtr<IInStream> withoutReplay;
            QCOMPARE(CreateDeviceInputAllowingIncompleteRead(selected, &withoutReplay), S_OK);
            selected = withoutReplay;
        }
        CMyComPtr<IFolderManager> manager = new QtArchiveFolderManager(backend);
        CMyComPtr<IFolderFolder> root;
        auto *opening = new CCopyCallback;
        CMyComPtr<IFolderOperationsExtractCallback> progress = opening;
        const auto open = [&] {
            return manager->OpenFolderFile(selected, L"USB 空/Linux data", L"Ext", &root, progress);
        };
        if (mode == "cancel-open" || mode == "disconnect-open") {
            opening->Cancel = mode == "cancel-open";
            handle->Disconnected = mode == "disconnect-open";
            const HRESULT rejected = worker(open);
            QVERIFY(rejected != S_OK);
            if (opening->Cancel)
                QCOMPARE(rejected, E_ABORT);
            QVERIFY(!root);
            opening->Cancel = false;
            handle->Disconnected = false;
        }
        QCOMPARE(worker(open), S_OK);
        QVERIFY(root);
        auto *agent = dynamic_cast<CAgentFolder *>(root.Interface());
        QVERIFY(agent);
        QVERIFY(!ArchiveUpdateEnabled(agent->GetArchiveProperties()));
        CMyComPtr<IFolderFolder> folder;
        QCOMPARE(root->BindToFolder(L"папка 空", &folder), S_OK);
        QVERIFY(folder);
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
        if (mode == "refresh") {
            auto *child = dynamic_cast<CAgentFolder *>(folder.Interface());
            QVERIFY(child);
            CMyComPtr<IFolderFolder> refreshed;
            QCOMPARE(worker([&] { return child->ReOpen(nullptr, &refreshed); }), S_OK);
            QVERIFY(folder->GetNumberOfItems(&count) != S_OK);
            folder = refreshed;
            QCOMPARE(folder->LoadItems(), S_OK);
            QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
            QCOMPARE(count, UInt32(1));
        }
        CMyComPtr<IFolderOperations> operations;
        QCOMPARE(folder.QueryInterface(IID_IFolderOperations, &operations), S_OK);
        QTemporaryDir destination;
        QVERIFY(destination.isValid());
        const auto prefix = (destination.path() + '/').toStdWString();
        auto *observer = new CCopyCallback;
        CMyComPtr<IFolderOperationsExtractCallback> callback = observer;
        observer->Cancel = mode == "cancel";
        handle->Disconnected = mode == "disconnect";
        const UInt32 index = 0;
        const auto result = worker(
            [&] { return operations->CopyTo(0, &index, 1, 0, 0, prefix.c_str(), callback); });
        if (mode == "cancel")
            QCOMPARE(result, E_ABORT);
        else if (mode == "disconnect")
            QVERIFY(result != S_OK || observer->Errors);
        else {
            QCOMPARE(result, S_OK);
            QCOMPARE(observer->Errors, 0u);
            QFile copied(destination.filePath(QStringLiteral("данные с пробелом.txt")));
            QVERIFY(copied.open(QIODevice::ReadOnly));
            QCOMPARE(copied.readAll(), QByteArray("device ext4 payload\n"));
        }
        QCOMPARE(operations->Delete(&index, 1, nullptr), E_NOTIMPL);
        QCOMPARE(backend.Listings, 0u);
        QCOMPARE(handle->Bytes, bytes);
        QVERIFY(handle->TotalRead < device.partitions()[0].size);
        std::weak_ptr<DeviceReadHandle> lease = handle;
        handle.reset();
        operations.Release();
        folder.Release();
        root.Release();
        selected.Release();
        input.Release();
        device = ArchiveDevice(backend.codecCatalog());
        QVERIFY(lease.expired());
    }
};
QTEST_GUILESS_MAIN(DeviceFolderTests)
#include "device_folder_tests.moc"
