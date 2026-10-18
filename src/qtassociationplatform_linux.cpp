#include "Common/StringConvert.h"
#include "desktopidentity.h"
#include "qtassociationplatform.h"
#include "qtlangcontrols.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QVector>
#include <pwd.h>
#include <unistd.h>
#undef signals
#include <gio/gdesktopappinfo.h>
#include <gio/gio.h>

namespace {
using App = std::unique_ptr<GAppInfo, decltype(&g_object_unref)>;
QString desktopId() { return QString::fromLatin1(NDesktopIdentity::kFileName) + ".desktop"; }
QByteArray mimeType(const UString &ext) {
    const auto type = QMimeDatabase().mimeTypeForFile("file." + QString::fromWCharArray(ext.Ptr()),
                                                      QMimeDatabase::MatchExtension);
    return type.isDefault() ? QByteArray() : type.name().toUtf8();
}
App ownApp() {
    App app(G_APP_INFO(g_desktop_app_info_new(desktopId().toUtf8().constData())), &g_object_unref);
    if (!app)
        return app;
    const char *command = g_app_info_get_commandline(app.get());
    gint argc = 0;
    gchar **argv = nullptr;
    // GIO's executable accessor takes the first space-delimited word and
    // retains quotes. Parse the desktop Exec syntax without invoking a shell.
    if (!command || !g_shell_parse_argv(command, &argc, &argv, nullptr) || argc == 0) {
        app.reset();
        return app;
    }
    const std::unique_ptr<gchar *, decltype(&g_strfreev)> arguments(argv, &g_strfreev);
    QString executable = QString::fromUtf8(argv[0]);
    executable.replace("%%", "%");
    if (!QFileInfo(executable).isAbsolute())
        executable = QStandardPaths::findExecutable(executable);
    if (QFileInfo(executable).canonicalFilePath() !=
        QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath())
        app.reset();
    return app;
}

QIcon nativeIcon(GIcon *icon) {
    // GIO's handler icon is independent of its Exec command. In particular,
    // desktop entries sharing an executable may have distinct file/theme icons.
    if (!icon || !qobject_cast<QGuiApplication *>(QCoreApplication::instance()))
        return {};
    if (G_IS_FILE_ICON(icon)) {
        const std::unique_ptr<gchar, decltype(&g_free)> path(
            g_file_get_path(g_file_icon_get_file(G_FILE_ICON(icon))), &g_free);
        return path ? QIcon(QString::fromUtf8(path.get())) : QIcon();
    }
    if (G_IS_THEMED_ICON(icon)) {
        const gchar *const *names = g_themed_icon_get_names(G_THEMED_ICON(icon));
        for (; names && *names; ++names) {
            const QIcon image = QIcon::fromTheme(QString::fromUtf8(*names));
            if (!image.isNull())
                return image;
        }
    }
    if (G_IS_EMBLEMED_ICON(icon))
        return nativeIcon(g_emblemed_icon_get_icon(G_EMBLEMED_ICON(icon)));
    return {};
}
LONG result(gboolean success, GError *error) {
    LONG code = success                                                                     ? 0
                : error && g_error_matches(error, G_IO_ERROR, G_IO_ERROR_WRONG_ETAG)        ? 1237
                : error && g_error_matches(error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED) ? 5
                                                                                            : 31;
    if (error)
        g_error_free(error);
    return code;
}
LONG removeOwnDefault(const QByteArray &type) {
    const QString path =
        QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
            .filePath("mimeapps.list");
    if (!QFileInfo::exists(path))
        return 0;
    std::unique_ptr<GFile, decltype(&g_object_unref)> file(
        g_file_new_for_path(QFile::encodeName(path).constData()), &g_object_unref);
    gchar *data = nullptr;
    gchar *etag = nullptr;
    gsize length = 0;
    GError *error = nullptr;
    if (!g_file_load_contents(file.get(), nullptr, &data, &length, &etag, &error))
        return result(false, error);
    const std::unique_ptr<gchar, decltype(&g_free)> contents(data, &g_free);
    const std::unique_ptr<gchar, decltype(&g_free)> revision(etag, &g_free);
    const std::unique_ptr<GKeyFile, decltype(&g_key_file_free)> keys(g_key_file_new(),
                                                                     &g_key_file_free);
    if (!g_key_file_load_from_data(
            keys.get(), data, length,
            GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS), &error))
        return result(false, error);
    gsize count = 0;
    gchar **defaults = g_key_file_get_string_list(keys.get(), "Default Applications",
                                                  type.constData(), &count, nullptr);
    const std::unique_ptr<gchar *, decltype(&g_strfreev)> ownedDefaults(defaults, &g_strfreev);
    QVector<const gchar *> retained;
    const QByteArray own = desktopId().toUtf8();
    for (gsize i = 0; i < count; ++i)
        if (own != defaults[i])
            retained.append(defaults[i]);
    if (gsize(retained.size()) == count)
        return 0;
    if (retained.isEmpty())
        g_key_file_remove_key(keys.get(), "Default Applications", type.constData(), nullptr);
    else
        g_key_file_set_string_list(keys.get(), "Default Applications", type.constData(),
                                   retained.constData(), retained.size());
    const std::unique_ptr<gchar, decltype(&g_free)> updated(
        g_key_file_to_data(keys.get(), &length, nullptr), &g_free);
    // GIO has no API to remove only one default. Retain other defaults and use
    // the file revision so concurrent preference changes are not overwritten.
    const gboolean ok = g_file_replace_contents(file.get(), updated.get(), length, etag, false,
                                                G_FILE_CREATE_NONE, nullptr, nullptr, &error);
    return result(ok, error);
}
class LinuxAssociationPlatform final : public IQtAssociationPlatform {
  public:
    bool CanApply(unsigned group) const override { return group == 0 && bool(ownApp()); }
    QString CurrentUserName() const override {
        const auto *user = getpwuid(getuid());
        return user ? QString::fromLocal8Bit(user->pw_name) : QString();
    }
    QString UnavailableReason(unsigned group) const override {
        return group != 0 ? QCoreApplication::translate(
                                "qtassociationplatform_linux",
                                "All-users associations are managed by the system administrator.")
                          : QCoreApplication::translate(
                                "qtassociationplatform_linux",
                                "Install the application desktop entry to change associations.");
    }
    UString OwnIconPath() const override {
        return UString(QCoreApplication::applicationFilePath().toStdWString().c_str());
    }
    QIcon OwnIcon() const override {
        const App app = ownApp();
        return app ? nativeIcon(g_app_info_get_icon(app.get())) : QIcon();
    }
    LONG Read(unsigned group, const UString &ext, QtAssociationRecord &record) override {
        if (group != 0)
            return 50;
        const QByteArray type = mimeType(ext);
        if (type.isEmpty())
            return 50; // Never claim application/octet-stream for an unknown extension.
        App app(g_app_info_get_default_for_type(type.constData(), false), &g_object_unref);
        const QString id = app ? QString::fromUtf8(g_app_info_get_id(app.get())) : QString();
        record.icon = app ? nativeIcon(g_app_info_get_icon(app.get())) : QIcon();
        record.info.ProgramKey = GetSystemString(
            UString((id == desktopId() ? QString("7-Zip.desktop") : id).toStdWString().c_str()));
        record.info.IconPath = app ? UString(QString::fromUtf8(g_app_info_get_executable(app.get()))
                                                 .toStdWString()
                                                 .c_str())
                                   : UString();
        if (id == desktopId() && ownApp())
            record.info.IconPath = OwnIconPath();
        QByteArray revision = type + '\0' + id.toUtf8();
        if (app)
            revision += QByteArray(g_app_info_get_commandline(app.get()));
        // Include the mutation file, including other applications' ordering.
        QFile preferences(
            QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
                .filePath("mimeapps.list"));
        if (preferences.exists() && !preferences.open(QIODevice::ReadOnly))
            return 5;
        if (preferences.isOpen()) {
            const QByteArray contents = preferences.readAll();
            const std::unique_ptr<GKeyFile, decltype(&g_key_file_free)> keys(g_key_file_new(),
                                                                             &g_key_file_free);
            if (!g_key_file_load_from_data(keys.get(), contents.constData(), contents.size(),
                                           G_KEY_FILE_NONE, nullptr)) {
                return 31;
            }
            for (const char *groupName :
                 {"Default Applications", "Added Associations", "Removed Associations"}) {
                const std::unique_ptr<gchar, decltype(&g_free)> value(
                    g_key_file_get_value(keys.get(), groupName, type.constData(), nullptr),
                    &g_free);
                if (value) {
                    revision += groupName;
                    revision += value.get();
                }
            }
        }
        record.revision = QCryptographicHash::hash(revision, QCryptographicHash::Sha256);
        record.scope = "linux:0:" + type;
        return 0;
    }
    LONG Write(unsigned group, const UString &ext, bool associate, const QtAssociationRecord &,
               const UString &, const UString &, const UString &, int) override {
        if (group != 0)
            return 5;
        const QByteArray type = mimeType(ext);
        auto app = ownApp();
        if (!app || type.isEmpty())
            return 50;
        if (!associate) {
            const LONG removed = removeOwnDefault(type);
            if (removed != 0)
                return removed;
        }
        GError *error = nullptr;
        const gboolean ok =
            associate ? g_app_info_set_as_default_for_type(app.get(), type.constData(), &error)
                      : g_app_info_remove_supports_type(app.get(), type.constData(), &error);
        return result(ok, error);
    }
    void NotifyChanged() override {} // GIO updates and notifies its MIME database.
};
} // namespace
std::shared_ptr<IQtAssociationPlatform> CreateNativeAssociationPlatform() {
    return std::make_shared<LinuxAssociationPlatform>();
}
