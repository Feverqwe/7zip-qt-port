#include "archivesfxmodule.h"
#include "../third_party/7zip/C/Sha256.h"
#include "CPP/7zip/UI/Common/Update.h"
#include "Common/StringConvert.h"
#include "qtlanguageservices.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>

namespace {
QString hostSystem() {
#ifdef Q_OS_WIN
    return "Windows";
#elif defined(Q_OS_MACOS)
    return "Darwin";
#else
    return "Linux";
#endif
}
QString architecture(QString value) {
    value = value.toLower();
    if (value == "aarch64")
        return "arm64";
    if (value == "amd64")
        return "x86_64";
    return value;
}
QByteArray digest(QFile &file) {
    CSha256 hash;
    Sha256_Init(&hash);
    while (!file.atEnd()) {
        const QByteArray block = file.read(65536);
        if (block.isEmpty())
            return {};
        Sha256_Update(&hash, reinterpret_cast<const Byte *>(block.constData()),
                      size_t(block.size()));
    }
    QByteArray result(SHA256_DIGEST_SIZE, 0);
    Sha256_Final(&hash, reinterpret_cast<Byte *>(result.data()));
    return result.toHex();
}
} // namespace
bool ArchiveSfxModule::MakeExecutable(const QString &path) {
#ifdef Q_OS_WIN
    Q_UNUSED(path)
    return true;
#else
    return QFile::setPermissions(path, QFile::permissions(path) | QFileDevice::ExeOwner |
                                           QFileDevice::ExeUser);
#endif
}
ArchiveSfxModule ArchiveSfxModule::Read(const QString &path, quint32 expectedVersion) {
    ArchiveSfxModule result;
    result.error_ = QtLocalText("The official SFX module is missing or incompatible.",
                                "Официальный модуль SFX отсутствует или несовместим.");
    FString module = us2fs(path.toStdWString().c_str());
    CUpdateErrorInfo error;
    if (PrepareSfxModule(module, FString(), error) != S_OK)
        return result;
    const QFileInfo info(path);
    QFile file(info.absoluteFilePath()), metadata(info.absoluteFilePath() + ".json");
    if (!info.isFile() || info.size() <= 0 || info.size() > (64 << 20) ||
        !file.open(QIODevice::ReadOnly) || !metadata.open(QIODevice::ReadOnly) ||
        metadata.size() > 16384)
        return result;
    const QJsonObject record = QJsonDocument::fromJson(metadata.readAll()).object();
    const quint32 version = quint32(record.value("version").toDouble());
    if (record.value("schema").toInt() != 1 ||
        record.value("repository").toString() != "https://github.com/ip7z/7zip.git" ||
        record.value("revision").toString() != QString::fromLatin1(SEVENZIP_UPSTREAM_REVISION) ||
        record.value("system").toString() != hostSystem() ||
        architecture(record.value("architecture").toString()) !=
            architecture(QSysInfo::buildCpuArchitecture()) ||
        !version || (expectedVersion && expectedVersion != version) ||
        record.value("sha256").toString().toLatin1() != digest(file))
        return result;
    const QString target = record.value("target").toString();
#ifdef Q_OS_WIN
    if (target != "windows-gui" ||
        record.value("producer").toString() != "sevenzip-desktop/build-sfx-windows" ||
        record.value("source").toString() != "CPP/7zip/Bundles/SFXWin/makefile")
        return result;
#else
    if (target != "native-console" ||
        record.value("producer").toString() != "sevenzip-desktop/build-sfx" ||
        record.value("source").toString() != "CPP/7zip/Bundles/SFXCon/makefile.gcc")
        return result;
#endif
    result.path_ = info.absoluteFilePath();
    result.version_ = version;
    result.error_.clear();
    return result;
}
ArchiveSfxModule ArchiveSfxModule::Discover(quint32 expectedVersion) {
    const QString override = qEnvironmentVariable("SEVENZIP_SFX_MODULE");
    if (!override.isEmpty())
        return Read(override, expectedVersion);
#ifdef Q_OS_WIN
    const QString name = "7z.sfx";
#else
    const QString name = "7zCon.sfx";
#endif
    const QDir app(QCoreApplication::applicationDirPath());
    QStringList directories{app.absolutePath(), app.filePath("sfx"),
                            app.filePath("../Resources/7zip")};
#ifdef SEVENZIP_BUILD_SFX_DIR
    directories.append(QString::fromLatin1(SEVENZIP_BUILD_SFX_DIR));
#endif
    for (const QString &directory : directories) {
        auto module = Read(QDir(directory).filePath(name), expectedVersion);
        if (module.available())
            return module;
    }
    return Read(QString(), expectedVersion);
}
