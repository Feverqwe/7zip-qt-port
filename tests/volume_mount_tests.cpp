#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/FSDrives.h"
#include "Windows/PropVariant.h"
#include <QTemporaryDir>
#include <QtTest>

class VolumeMountTests final : public QObject {
    Q_OBJECT
  private slots:
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
};

QTEST_MAIN(VolumeMountTests)
#include "volume_mount_tests.moc"
