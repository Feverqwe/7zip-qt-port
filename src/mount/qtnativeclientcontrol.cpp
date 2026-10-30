#include "mount/qtnativeclientcontrol.h"
#ifdef Q_OS_WIN
#include <QScopeGuard>
// clang-format off
#include <windows.h>
#include <sddl.h>
// clang-format on
#else
#include <unistd.h>
#endif

QString QtNativeClientOwnerIdentity() {
#ifdef Q_OS_WIN
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return {};
    const auto closeToken = qScopeGuard([token] { CloseHandle(token); });
    DWORD size = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    QByteArray buffer(qsizetype(size), Qt::Uninitialized);
    const bool queried = GetTokenInformation(token, TokenUser, buffer.data(), size, &size);
    if (!queried)
        return {};
    LPWSTR sid = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER *>(buffer.data())->User.Sid, &sid))
        return {};
    const auto releaseSid = qScopeGuard([sid] { LocalFree(sid); });
    return QString::fromWCharArray(sid);
#else
    return QString::number(getuid());
#endif
}
QtNativeClientControl::QtNativeClientControl(QtNativeMountParameters parameters)
    : parameters_(std::move(parameters)) {}
quint16 QtNativeClientControl::port() const { return parameters_.Port; }
QString QtNativeClientControl::share() const { return parameters_.Share; }
QString QtNativeClientControl::user() const { return parameters_.User; }
QString QtNativeClientControl::password() const { return parameters_.Password; }
QString QtNativeClientControl::owner() const { return parameters_.Owner; }
QString QtNativeClientControl::directory() const { return parameters_.Directory; }
bool QtNativeClientControl::recovery() const { return recovery_; }
bool QtNativeClientControl::force() const { return force_; }
bool QtNativeClientControl::ready() {
    connected_ = true;
    return true;
}
bool QtNativeClientControl::connected() const { return connected_; }
void QtNativeClientControl::prepareRecovery(bool force) {
    recovery_ = true;
    force_ = force;
    emit recoveryChanged(recovery_);
    emit forceChanged(force_);
}
bool QtNativeClientControl::mounted(QString path, QString error) {
    emit mountReported(std::move(path), std::move(error));
    return true;
}
bool QtNativeClientControl::unmounted(QString error) {
    emit unmountReported(std::move(error));
    return true;
}
