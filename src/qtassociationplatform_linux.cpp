#include "Common/StringConvert.h"
#include "desktopidentity.h"
#include "qtassociationplatform.h"
#include "qtlangcontrols.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QStandardPaths>
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
    QString executable = QString::fromUtf8(g_app_info_get_executable(app.get()));
    if (!QFileInfo(executable).isAbsolute())
        executable = QStandardPaths::findExecutable(executable);
    if (QFileInfo(executable).canonicalFilePath() !=
        QFileInfo(QCoreApplication::applicationFilePath()).canonicalFilePath())
        app.reset();
    return app;
}
LONG result(gboolean success, GError *error) {
    LONG code = success                                                                     ? 0
                : error && g_error_matches(error, G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED) ? 5
                                                                                            : 31;
    if (error)
        g_error_free(error);
    return code;
}
class LinuxAssociationPlatform final : public IQtAssociationPlatform {
  public:
    bool CanApply(unsigned group) const override { return group == 0 && bool(ownApp()); }
    QString CurrentUserName() const override {
        const auto *user = getpwuid(getuid());
        return user ? QString::fromLocal8Bit(user->pw_name) : QString();
    }
    QString UnavailableReason(unsigned group) const override {
        return group != 0
                   ? QtLocalText(
                         "All-users associations are managed by the system administrator.",
                         "Ассоциациями всех пользователей управляет системный администратор.")
                   : QtLocalText("Install the application desktop entry to change associations.",
                                 "Для изменения ассоциаций установите desktop-файл приложения.");
    }
    UString OwnIconPath() const override {
        return UString(QCoreApplication::applicationFilePath().toStdWString().c_str());
    }
    LONG Read(unsigned group, const UString &ext, QtAssociationRecord &record) override {
        if (group != 0)
            return 50;
        const QByteArray type = mimeType(ext);
        if (type.isEmpty())
            return 50; // Never claim application/octet-stream for an unknown extension.
        App app(g_app_info_get_default_for_type(type.constData(), false), &g_object_unref);
        const QString id = app ? QString::fromUtf8(g_app_info_get_id(app.get())) : QString();
        record.info.ProgramKey = GetSystemString(
            UString((id == desktopId() ? QString("7-Zip.desktop") : id).toStdWString().c_str()));
        record.info.IconPath = app ? UString(QString::fromUtf8(g_app_info_get_executable(app.get()))
                                                 .toStdWString()
                                                 .c_str())
                                   : UString();
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
            GKeyFile *keys = g_key_file_new();
            if (!g_key_file_load_from_data(keys, contents.constData(), contents.size(),
                                           G_KEY_FILE_NONE, nullptr)) {
                g_key_file_free(keys);
                return 31;
            }
            for (const char *groupName :
                 {"Default Applications", "Added Associations", "Removed Associations"}) {
                gchar *value = g_key_file_get_value(keys, groupName, type.constData(), nullptr);
                if (value) {
                    revision += groupName;
                    revision += value;
                    g_free(value);
                }
            }
            g_key_file_free(keys);
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
