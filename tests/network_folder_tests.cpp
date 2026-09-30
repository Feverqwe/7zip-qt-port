#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/NetFolder.h"
#include "Windows/PropVariant.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class NetworkFolderTests final : public QObject {
    Q_OBJECT
  private slots:
    void mountedShareCanOpenThenDisappear() {
#ifdef _WIN32
        QSKIP("Windows network service requires platform runtime verification");
#else
        QTemporaryDir fixture;
        QVERIFY(fixture.isValid());
        const QString share = fixture.filePath(QStringLiteral("общий ресурс с пробелом"));
        QVERIFY(QDir().mkpath(share));
        QList<QtVolumeMount> mounts{{share, QStringLiteral("общий ресурс с пробелом"),
                                     QStringLiteral("smbfs"), 4096, 2048, true},
                                    {QStringLiteral("/"), QStringLiteral("Локальный"),
                                     QStringLiteral("apfs"), 4096, 2048, false}};
        CMyComPtr<IFolderFolder> network = new CNetFolder([&mounts] { return mounts; });
        auto *owner = dynamic_cast<CNetFolder *>((IFolderFolder *)network);
        QVERIFY(owner);
        owner->Init(UString(L"Сеть/"));
        QCOMPARE(network->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(network->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
        NWindows::NCOM::CPropVariant prop;
        QCOMPARE(network->GetProperty(0, kpidName, &prop), S_OK);
        QCOMPARE(QString::fromWCharArray(prop.bstrVal), QStringLiteral("общий ресурс с пробелом"));
        prop.Clear();
        QCOMPARE(network->GetProperty(0, kpidProvider, &prop), S_OK);
        QCOMPARE(QString::fromWCharArray(prop.bstrVal), QStringLiteral("smbfs"));
        CMyComPtr<IFolderFolder> folder;
        QCOMPARE(network->BindToFolder(UInt32(0), &folder), S_OK);
        QVERIFY(folder);
        folder.Release();
        QVERIFY(QDir().rmdir(share));
        QFile unavailable(share);
        QVERIFY(unavailable.open(QIODevice::WriteOnly));
        unavailable.close();
        QCOMPARE(network->BindToFolder(UInt32(0), &folder), E_FAIL);
        QVERIFY(!folder);
        mounts.removeFirst();
        QCOMPARE(network->BindToFolder(UInt32(0), &folder), E_FAIL);
        QVERIFY(!folder);
        QCOMPARE(network->LoadItems(), S_OK);
        QCOMPARE(network->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(0));
#endif
    }
};

QTEST_MAIN(NetworkFolderTests)
#include "network_folder_tests.moc"
