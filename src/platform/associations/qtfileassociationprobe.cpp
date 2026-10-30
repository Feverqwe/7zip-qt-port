#include "platform/associations/qtfileassociationprobe.h"
#include <QFileInfo>
#ifdef Q_OS_WIN
#include <windows.h>

#include <shlwapi.h>
#else
#include <QMimeDatabase>
#undef signals
#include <gio/gio.h>
#endif

bool QtFileHasOpenAssociation(const QString &path) {
#ifdef Q_OS_WIN
    const QString suffix = QFileInfo(path).suffix();
    if (suffix.isEmpty())
        return false;
    const std::wstring extension = ("." + suffix).toStdWString();
    DWORD size = 0;
    const HRESULT result = AssocQueryStringW(ASSOCF_INIT_IGNOREUNKNOWN, ASSOCSTR_FRIENDLYAPPNAME,
                                             extension.c_str(), L"open", nullptr, &size);
    return (result == S_FALSE || result == S_OK) && size > 1;
#else
    const auto type = QMimeDatabase().mimeTypeForFile(path, QMimeDatabase::MatchDefault);
    if (!type.isValid() || type.isDefault())
        return false;
    GAppInfo *app = g_app_info_get_default_for_type(type.name().toUtf8().constData(), false);
    if (!app)
        return false;
    g_object_unref(app);
    return true;
#endif
}
