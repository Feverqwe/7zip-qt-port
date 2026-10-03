#pragma once
#include <QByteArray>
#include <QDir>
#include <QString>
#ifdef Q_OS_WIN
#include "Common/MyWindows.h"
#include <sddl.h>

inline bool SetFixturePublicDacl(const QString &path) {
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!::ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:P(A;;FA;;;WD)", SDDL_REVISION_1,
                                                                &descriptor, nullptr))
        return false;
    const std::wstring native = QDir::toNativeSeparators(path).toStdWString();
    const bool result = ::SetFileSecurityW(
        native.c_str(), DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
        descriptor);
    ::LocalFree(descriptor);
    return result;
}

inline QString FixtureDacl(const QString &path) {
    const std::wstring native = QDir::toNativeSeparators(path).toStdWString();
    DWORD needed = 0;
    ::GetFileSecurityW(native.c_str(), DACL_SECURITY_INFORMATION, nullptr, 0, &needed);
    if (!needed)
        return {};
    QByteArray descriptor(static_cast<int>(needed), '\0');
    if (!::GetFileSecurityW(native.c_str(), DACL_SECURITY_INFORMATION, descriptor.data(), needed,
                            &needed))
        return {};
    LPWSTR text = nullptr;
    if (!::ConvertSecurityDescriptorToStringSecurityDescriptorW(
            descriptor.data(), SDDL_REVISION_1, DACL_SECURITY_INFORMATION, &text, nullptr))
        return {};
    const QString result = QString::fromWCharArray(text);
    ::LocalFree(text);
    return result;
}
#endif
