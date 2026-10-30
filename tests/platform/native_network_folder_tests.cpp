#include "7zip/PropID.h"
#include "Windows/PropVariant.h"
#include "filemanager/providers/NetFolder.h"
#include "filemanager/providers/RootFolder.h"
#include "support/native_network_share.h"
#include <QDir>
#include <QFile>
#include <QtTest>
#include <lm.h>

namespace {
int findShare(IFolderFolder &folder, const QString &name) {
    UInt32 count = 0;
    if (folder.GetNumberOfItems(&count) != S_OK)
        return -1;
    for (UInt32 index = 0; index < count; ++index) {
        NWindows::NCOM::CPropVariant prop;
        if (folder.GetProperty(index, kpidName, &prop) == S_OK && prop.vt == VT_BSTR &&
            QString::fromWCharArray(prop.bstrVal) == name)
            return int(index);
    }
    return -1;
}
} // namespace

class NativeNetworkFolderTests final : public QObject {
    Q_OBJECT
  private slots:
    void nativeShareCanOpenThenDisappear_data() {
        QTest::addColumn<bool>("fromRoot");
        QTest::newRow("direct-network-provider") << false;
        QTest::newRow("root-server-fallback") << true;
    }
    void nativeShareCanOpenThenDisappear() {
        QFETCH(bool, fromRoot);
        NativeNetworkShare fixture;
        const QString error = fixture.create();
        QVERIFY2(error.isEmpty(), qPrintable(error));
        const QString name = fixture.name();
        const QString fileName = NativeNetworkShare::fileName();
        const QByteArray payload = NativeNetworkShare::payload();
        QFile file(fixture.filePath());
        const QString server = fixture.server();
        CMyComPtr<IFolderFolder> network;
        if (fromRoot) {
            CMyComPtr<IFolderFolder> root = new CRootFolder;
            static_cast<CRootFolder *>((IFolderFolder *)root)->Init();
            // CFSFolder cannot bind a server container: original RootFolder must
            // delegate this exact local UNC address to CNetFolder.
            QCOMPARE(root->BindToFolder(server.toStdWString().c_str(), &network), S_OK);
            QVERIFY(network);
        } else {
            network = new CNetFolder;
            static_cast<CNetFolder *>((IFolderFolder *)network)
                ->Init(UString(server.toStdWString().c_str()));
        }
        NWindows::NCOM::CPropVariant type;
        QCOMPARE(network->GetFolderProperty(kpidType, &type), S_OK);
        QCOMPARE(type.vt, VARTYPE(VT_BSTR));
        QCOMPARE(QString::fromWCharArray(type.bstrVal), QString("NetFolder"));
        QCOMPARE(network->LoadItems(), S_OK);
        const int index = findShare(*network, name);
        QVERIFY(index >= 0);
        CMyComPtr<IFolderFolder> folder;
        QCOMPARE(network->BindToFolder(UInt32(index), &folder), S_OK);
        QVERIFY(folder);
        QCOMPARE(folder->LoadItems(), S_OK);
        UInt32 count = 0;
        QCOMPARE(folder->GetNumberOfItems(&count), S_OK);
        QCOMPARE(count, UInt32(1));
        NWindows::NCOM::CPropVariant prop;
        QCOMPARE(folder->GetProperty(0, kpidName, &prop), S_OK);
        QCOMPARE(prop.vt, VARTYPE(VT_BSTR));
        QCOMPARE(QString::fromWCharArray(prop.bstrVal), fileName);
        QFile received(QDir::fromNativeSeparators(server + name + "\\" + fileName));
        QVERIFY(received.open(QIODevice::ReadOnly));
        QCOMPARE(received.readAll(), payload);
        received.close();
        folder.Release();
        QCOMPARE(fixture.remove(), static_cast<unsigned long>(NERR_Success));
        QVERIFY(network->BindToFolder(UInt32(index), &folder) != S_OK);
        QVERIFY(!folder);
        QCOMPARE(network->LoadItems(), S_OK);
        QCOMPARE(findShare(*network, name), -1);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), payload);
    }
};

QTEST_GUILESS_MAIN(NativeNetworkFolderTests)
#include "native_network_folder_tests.moc"
