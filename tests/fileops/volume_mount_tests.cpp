#include "7zip/PropID.h"
#include "Windows/PropVariant.h"
#include "filemanager/providers/FSDrives.h"
#include <QStorageInfo>
#include <QTemporaryDir>
#include <QtTest>

class VolumeMountTests final : public QObject {
    Q_OBJECT
  private slots:
#ifdef Q_OS_WIN
    void nativeDriveContainsDisposableFixture() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString drive = QFileInfo(fixture.path()).absoluteFilePath().left(2);
        CMyComPtr<IFolderFolder> drives = new CFSDrives;
        QCOMPARE(drives->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(drives->GetNumberOfItems(&count), S_OK);
        UInt32 index = count;
        for (UInt32 i = 0; i < count; ++i) {
            NWindows::NCOM::CPropVariant name;
            QCOMPARE(drives->GetProperty(i, kpidName, &name), S_OK);
            if (name.vt == VT_BSTR && QString::fromWCharArray(name.bstrVal) == drive)
                index = i;
        }
        QVERIFY(index < count);
        NWindows::NCOM::CPropVariant size;
        QCOMPARE(drives->GetProperty(index, kpidTotalSize, &size), S_OK);
        QCOMPARE(size.vt, VARTYPE(VT_UI8));
        QCOMPARE(size.uhVal.QuadPart, UInt64(QStorageInfo(fixture.path()).bytesTotal()));
        CMyComPtr<IFolderFolder> folder;
        QCOMPARE(drives->BindToFolder(index, &folder), S_OK);
        QVERIFY(folder);
        NWindows::NCOM::CPropVariant path;
        QCOMPARE(folder->GetFolderProperty(kpidPath, &path), S_OK);
        QCOMPARE(path.vt, VARTYPE(VT_BSTR));
        QCOMPARE(QDir::fromNativeSeparators(QString::fromWCharArray(path.bstrVal)), drive + '/');
        QVERIFY(QFileInfo(fixture.path()).isDir());
    }
#else
    void mountedVolumeCanOpenThenDisappear() {
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        QList<QtVolumeMount> mounts{
            {fixture.path(), QStringLiteral("Том с пробелом"), QStringLiteral("apfs"), 4096, 2048}};
        CMyComPtr<IFolderFolder> drives = new CFSDrives([&mounts] { return mounts; });
        QCOMPARE(drives->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(drives->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
        NWindows::NCOM::CPropVariant prop;
        QCOMPARE(drives->GetProperty(0, kpidName, &prop), S_OK);
        QCOMPARE(QString::fromWCharArray(prop.bstrVal), QStringLiteral("Том с пробелом"));
        prop.Clear();
        QCOMPARE(drives->GetProperty(0, kpidTotalSize, &prop), S_OK);
        QCOMPARE(prop.uhVal.QuadPart, UInt64(4096));
        CMyComPtr<IFolderFolder> folder;
        QCOMPARE(drives->BindToFolder(UInt32(0), &folder), S_OK);
        QVERIFY(folder);
        folder.Release();
        mounts.clear();
        QCOMPARE(drives->BindToFolder(UInt32(0), &folder), E_FAIL);
        QVERIFY(!folder);
        QCOMPARE(drives->LoadItems(), S_OK);
        QCOMPARE(drives->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
        mounts.append({fixture.filePath("absent"), QStringLiteral("Недоступный том"), {}});
        QCOMPARE(drives->LoadItems(), S_OK);
        QCOMPARE(drives->BindToFolder(UInt32(0), &folder), E_FAIL);
        QVERIFY(!folder);
    }
#endif
};

QTEST_MAIN(VolumeMountTests)
#include "volume_mount_tests.moc"
