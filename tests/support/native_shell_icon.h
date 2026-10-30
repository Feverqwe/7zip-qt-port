#pragma once

#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QString>

#ifdef Q_OS_WIN
#include <windows.h>

#include <shellapi.h>

namespace NativeShellIcon {
// Independent HICON query for the SHGFI_USEFILEATTRIBUTES policy used by 7zFM.
inline QImage image(const QString &path, DWORD attributes) {
    SHFILEINFOW info{};
    if (!SHGetFileInfoW(reinterpret_cast<const wchar_t *>(path.utf16()), attributes, &info,
                        sizeof(info), SHGFI_USEFILEATTRIBUTES | SHGFI_ICON | SHGFI_SMALLICON))
        return {};
    const QIcon icon(QPixmap::fromImage(QImage::fromHICON(info.hIcon)));
    DestroyIcon(info.hIcon);
    return icon.pixmap(16, 16).toImage();
}
} // namespace NativeShellIcon
#endif
