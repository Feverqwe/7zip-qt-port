#include "7zip/PropID.h"
#include "CPP/7zip/UI/FileManager/NetFolder.h"
#include "Windows/PropVariant.h"
#include <QDir>
#include <QFile>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>
#include <lm.h>
#include <sddl.h>

namespace {
QString currentUserSid() {
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return {};
    const auto close = qScopeGuard([&] { CloseHandle(token); });
    DWORD needed = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &needed);
    if (!needed)
        return {};
    QByteArray info(int(needed), '\0');
    if (!GetTokenInformation(token, TokenUser, info.data(), needed, &needed))
        return {};
    LPWSTR text = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER *>(info.data())->User.Sid, &text))
        return {};
    const auto free = qScopeGuard([&] { LocalFree(text); });
    return QString::fromWCharArray(text);
}

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
    void nativeShareCanOpenThenDisappear() {
        QTemporaryDir fixture(
            QDir(QDir::tempPath()).filePath(QStringLiteral("sevenzip native сеть 空 XXXXXX")));
        QVERIFY(fixture.isValid());
        const QString name =
            QStringLiteral("Sevenzip test ") + QUuid::createUuid().toString(QUuid::WithoutBraces);
        const QString fileName = QStringLiteral("файл 空 с пробелом.txt");
        const QByteArray payload("native network fixture\0bytes", 28);
        QFile file(fixture.filePath(fileName));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(payload), qint64(payload.size()));
        file.close();
        const QString sid = currentUserSid();
        QVERIFY(!sid.isEmpty());
        const std::wstring sddl = QString("D:P(A;;FA;;;%1)").arg(sid).toStdWString();
        PSECURITY_DESCRIPTOR descriptor = nullptr;
        QVERIFY(ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1,
                                                                     &descriptor, nullptr));
        const auto free = qScopeGuard([&] { LocalFree(descriptor); });
        std::wstring nativeName = name.toStdWString();
        std::wstring nativePath = QDir::toNativeSeparators(fixture.path()).toStdWString();
        SHARE_INFO_502 info{};
        info.shi502_netname = nativeName.data();
        info.shi502_type = STYPE_DISKTREE;
        info.shi502_max_uses = SHI_USES_UNLIMITED;
        info.shi502_path = nativePath.data();
        info.shi502_security_descriptor = descriptor;
        DWORD parameter = 0;
        QCOMPARE(NetShareAdd(nullptr, 502, reinterpret_cast<LPBYTE>(&info), &parameter),
                 NET_API_STATUS(NERR_Success));
        bool shared = true;
        const auto remove = qScopeGuard([&] {
            if (shared)
                NetShareDel(nullptr, nativeName.data(), 0);
        });
        const QString server = QStringLiteral("\\\\") + QSysInfo::machineHostName() + "\\";
        CMyComPtr<IFolderFolder> network = new CNetFolder;
        static_cast<CNetFolder *>((IFolderFolder *)network)
            ->Init(UString(server.toStdWString().c_str()));
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
        QCOMPARE(NetShareDel(nullptr, nativeName.data(), 0), NET_API_STATUS(NERR_Success));
        shared = false;
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
