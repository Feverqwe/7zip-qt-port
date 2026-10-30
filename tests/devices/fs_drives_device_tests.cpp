#include "7zip/PropID.h"
#include "Common/StringConvert.h"
#include "Windows/PropVariant.h"
#include "filemanager/providers/FSDrives.h"
#include "support/read_only_virtual_disk.h"
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class DeviceCopyCallback final : public IFolderOperationsExtractCallback, public CMyUnknownImp {
    Z7_COM_UNKNOWN_IMP_1(IFolderOperationsExtractCallback)

  public:
    UInt64 total = 0;
    UInt64 files = 0;
    UInt64 cancelAt = 0;
    int asks = 0;
    bool skip = false;
    QString source;
    Z7_COM7F_IMF(SetTotal(UInt64 value)) override {
        total = value;
        return S_OK;
    }
    Z7_COM7F_IMF(SetCompleted(const UInt64 *value)) override {
        return value && cancelAt && *value >= cancelAt ? E_ABORT : S_OK;
    }
    Z7_COM7F_IMF(SetNumFiles(UInt64 value)) override {
        files = value;
        return S_OK;
    }
    Z7_COM7F_IMF(SetCurrentFilePath(const wchar_t *path)) override {
        source = QString::fromWCharArray(path);
        return S_OK;
    }
    Z7_COM7F_IMF(ShowMessage(const wchar_t *)) override { return S_OK; }
    Z7_COM7F_IMF(AskWrite(const wchar_t *, Int32, const FILETIME *, const UInt64 *,
                          const wchar_t *destination, BSTR *result, Int32 *answer))
    override {
        ++asks;
        *answer = skip ? 0 : 1;
        return skip ? S_OK : StringToBstr(destination, result);
    }
};

class FsDrivesDeviceTests final : public QObject {
    Q_OBJECT
    static QByteArray read(const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    }

  private slots:
    void sourceCopyUsesReadOnlyPrivateDevice_data() {
        QTest::addColumn<QString>("mode");
        for (const char *mode : {"file", "directory", "cancel", "failure", "skip", "move"})
            QTest::newRow(mode) << QString::fromLatin1(mode);
    }
    void sourceCopyUsesReadOnlyPrivateDevice() {
        QFETCH(QString, mode);
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        ReadOnlyVirtualDisk disk(temporary.filePath(QStringLiteral("диск 空 с пробелом.vhd")));
        QVERIFY2(disk.valid(), qPrintable(QString::number(disk.error())));
        const QByteArray source = disk.readDevice();
        QCOMPARE(quint64(source.size()), ReadOnlyVirtualDisk::Size);
        auto *provider = new CFSDrives;
        provider->Init(true);
        CMyComPtr<IFolderFolder> folder = provider;
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        const QString deviceName = disk.device().mid(4);
        UInt32 index = count;
        NWindows::NCOM::CPropVariant property;
        for (UInt32 i = 0; i < count; ++i) {
            QCOMPARE(folder->GetProperty(i, kpidName, &property), S_OK);
            if (property.vt == VT_BSTR && QString::fromWCharArray(property.bstrVal) == deviceName)
                index = i;
        }
        QVERIFY(index < count);
        QCOMPARE(folder->GetProperty(index, kpidIsDir, &property), S_OK);
        QCOMPARE(property.vt, VARTYPE(VT_BOOL));
        QCOMPARE(property.boolVal, VARIANT_FALSE);
        QCOMPARE(folder->GetProperty(index, kpidTotalSize, &property), S_OK);
        QCOMPARE(property.vt, VARTYPE(VT_UI8));
        QCOMPARE(property.uhVal.QuadPart, UInt64(ReadOnlyVirtualDisk::Size));
        CMyComPtr<IFolderOperations> operations;
        QCOMPARE(folder.QueryInterface(IID_IFolderOperations, &operations), S_OK);
        DeviceCopyCallback callback;
        const QString target = temporary.filePath(QStringLiteral("образ й 空 с пробелом.img"));
        QString destination = mode == "directory" ? temporary.path() + '/' : target;
        if (mode == "cancel")
            callback.cancelAt = 4 * 1024 * 1024;
        else if (mode == "failure")
            destination = temporary.filePath(QStringLiteral("missing parent/образ.img"));
        else if (mode == "skip") {
            QFile existing(target);
            QVERIFY(existing.open(QIODevice::WriteOnly));
            QCOMPARE(existing.write("preserved"), qint64(9));
            existing.close();
            callback.skip = true;
        }
        const std::wstring native = QDir::toNativeSeparators(destination).toStdWString();
        const HRESULT result =
            operations->CopyTo(mode == "move", &index, 1, 0, 0, native.c_str(), &callback);
        if (mode == "move") {
            QCOMPARE(result, E_NOTIMPL);
            QCOMPARE(callback.asks, 0);
            QVERIFY(!QFileInfo::exists(target));
        } else if (mode == "failure") {
            QVERIFY(FAILED(result));
            QCOMPARE(callback.asks, 1);
            QVERIFY(!QFileInfo::exists(destination));
        } else if (mode == "cancel") {
            QCOMPARE(result, E_ABORT);
            const QByteArray partial = read(target);
            QVERIFY(!partial.isEmpty());
            QVERIFY(source.startsWith(partial));
        } else if (mode == "skip") {
            QCOMPARE(result, S_OK);
            QCOMPARE(read(target), QByteArray("preserved"));
            QCOMPARE(callback.total, UInt64(0));
        } else {
            QCOMPARE(result, S_OK);
            QCOMPARE(read(mode == "directory" ? temporary.filePath(deviceName) : target), source);
            QCOMPARE(callback.total, UInt64(ReadOnlyVirtualDisk::Size));
            QCOMPARE(callback.files, UInt64(1));
            QCOMPARE(callback.asks, 1);
            QCOMPARE(callback.source, disk.device());
        }
        QCOMPARE(disk.readDevice(), source);
        // Source provider never offers write, deletion, rename or Move to a device.
        QCOMPARE(operations->CreateFile(L"forbidden", &callback), E_NOTIMPL);
        QCOMPARE(operations->Delete(&index, 1, &callback), E_NOTIMPL);
        QCOMPARE(disk.detach(), DWORD(ERROR_SUCCESS));
    }
};

QTEST_GUILESS_MAIN(FsDrivesDeviceTests)
#include "fs_drives_device_tests.moc"
