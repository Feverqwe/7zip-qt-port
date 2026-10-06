#include "native_network_share.h"
#include <QDir>
#include <QFile>
#include <QScopeGuard>
#include <QSysInfo>
#include <QUuid>
#include <windows.h>

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
} // namespace

NativeNetworkShare::NativeNetworkShare()
    : directory_(QDir(QDir::tempPath()).filePath(QStringLiteral("sevenzip native сеть 空 XXXXXX"))),
      name_(QStringLiteral("Sevenzip test ") + QUuid::createUuid().toString(QUuid::WithoutBraces)) {
}

NativeNetworkShare::~NativeNetworkShare() {
    if (shared_)
        remove();
}

QString NativeNetworkShare::create() {
    if (shared_)
        return QStringLiteral("Fixture is already shared");
    if (!directory_.isValid())
        return QStringLiteral("Temporary directory creation failed");
    QFile file(filePath());
    if (!file.open(QIODevice::WriteOnly) || file.write(payload()) != payload().size())
        return file.errorString();
    file.close();
    const QString sid = currentUserSid();
    if (sid.isEmpty())
        return QStringLiteral("Current runner SID is unavailable");
    const std::wstring sddl = QString("D:P(A;;FA;;;%1)").arg(sid).toStdWString();
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1,
                                                              &descriptor, nullptr))
        return QString("Share security descriptor error %1").arg(GetLastError());
    const auto free = qScopeGuard([&] { LocalFree(descriptor); });
    std::wstring nativeName = name_.toStdWString();
    std::wstring nativePath = QDir::toNativeSeparators(directory_.path()).toStdWString();
    SHARE_INFO_502 info{};
    info.shi502_netname = nativeName.data();
    info.shi502_type = STYPE_DISKTREE;
    info.shi502_max_uses = SHI_USES_UNLIMITED;
    info.shi502_path = nativePath.data();
    info.shi502_security_descriptor = descriptor;
    DWORD parameter = 0;
    const NET_API_STATUS status =
        NetShareAdd(nullptr, 502, reinterpret_cast<LPBYTE>(&info), &parameter);
    if (status != NERR_Success)
        return QString("NetShareAdd error %1, parameter %2").arg(status).arg(parameter);
    shared_ = true;
    return {};
}

unsigned long NativeNetworkShare::remove() {
    if (!shared_)
        return NERR_Success;
    std::wstring nativeName = name_.toStdWString();
    const NET_API_STATUS status = NetShareDel(nullptr, nativeName.data(), 0);
    if (status == NERR_Success)
        shared_ = false;
    return status;
}

QString NativeNetworkShare::server() const {
    return QStringLiteral("\\\\") + QSysInfo::machineHostName() + "\\";
}
QString NativeNetworkShare::sharePath() const { return server() + name_ + "\\"; }
QString NativeNetworkShare::filePath() const { return directory_.filePath(fileName()); }
QString NativeNetworkShare::fileName() { return QStringLiteral("файл 空 с пробелом.txt"); }
QByteArray NativeNetworkShare::payload() { return QByteArray("native network fixture\0bytes", 28); }
