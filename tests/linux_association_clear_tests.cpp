#include "Common/StringConvert.h"
#include "qtassociationplatform.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

namespace {
bool write(const QString &path, const QByteArray &data) {
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}
QByteArray read(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
} // namespace

class LinuxAssociationClearTests final : public QObject {
    Q_OBJECT
  private slots:
    void foreignExecutableIsRejected() {
        const QString preferences = qEnvironmentVariable("XDG_CONFIG_HOME") + "/mimeapps.list";
        const QByteArray before = read(preferences);
        const QString desktop =
            qEnvironmentVariable("XDG_DATA_HOME") + "/applications/sevenzip_desktop.desktop";
        const QByteArray own = read(desktop);
        QVERIFY(write(desktop, "[Desktop Entry]\nType=Application\nName=Foreign\n"
                               "Exec=\"/usr/bin/true\" %F\n"));
        const auto platform = CreateNativeAssociationPlatform();
        QVERIFY(!platform->CanApply(0));
        const auto services = CreateQtAssociationServices(platform);
        NRegistryAssoc::CShellExtInfo info;
        QCOMPARE(services->Add(0, UString(L"7z"), info, {}, {}, {}, 0), LONG(5));
        QCOMPARE(read(preferences), before);
        QVERIFY(write(desktop, own));
    }
    void clearRemovesOwnDefaultAndRetainsOtherPreferences() {
        const bool other = qEnvironmentVariableIntValue("SEVENZIP_ASSOC_OTHER") != 0;
        const QString preferences = qEnvironmentVariable("XDG_CONFIG_HOME") + "/mimeapps.list";
        const auto platform = CreateNativeAssociationPlatform();
        QVERIFY(platform->CanApply(0));
        QVERIFY(!platform->CanApply(1));
        const auto services = CreateQtAssociationServices(platform);
        NRegistryAssoc::CShellExtInfo info;
        QVERIFY(services->Read(0, UString(L"7z"), info));
        if (!other) {
            QVERIFY(info.ProgramKey.IsEmpty());
            QCOMPARE(services->Add(0, UString(L"7z"), info, {}, {}, {}, 0), LONG(0));
            QVERIFY(services->Read(0, UString(L"7z"), info));
        }
        QCOMPARE(QString::fromWCharArray(GetUnicodeString(info.ProgramKey)),
                 QString("7-Zip.desktop"));
        QCOMPARE(QString::fromWCharArray(info.IconPath),
                 QString::fromWCharArray(platform->OwnIconPath()));
        QCOMPARE(services->Delete(0, UString(L"7z"), info), LONG(0));
        QVERIFY(services->Read(0, UString(L"7z"), info));
        QCOMPARE(QString::fromWCharArray(GetUnicodeString(info.ProgramKey)),
                 other ? QString("Other.desktop") : QString());
        const QByteArray updated = read(preferences);
        QVERIFY(updated.contains("text/plain=Other.desktop;"));
        QVERIFY(!updated.contains("application/x-7z-compressed=sevenzip_desktop.desktop;Other"));
        if (other)
            QVERIFY(updated.contains("application/x-7z-compressed=Other.desktop;"));
        QProcess query;
        query.start(QStandardPaths::findExecutable("gio"), {"mime", "application/x-7z-compressed"});
        QVERIFY(query.waitForFinished(5000));
        QCOMPARE(query.exitCode(), 0);
        const QByteArray output = query.readAllStandardOutput();
        QVERIFY2(other ? output.contains("Other.desktop")
                       : output.contains("No default applications"),
                 output.constData());
        QVERIFY(!output.contains("Default application for") ||
                !output.contains("sevenzip_desktop.desktop"));
    }
};

int main(int argc, char **argv) {
    // GIO caches XDG locations: isolate before constructing either app or service.
    QTemporaryDir fixture;
    if (!fixture.isValid())
        return 2;
    for (const QString &directory : {"config", "data/applications", "system", "bin"})
        if (!QDir().mkpath(fixture.filePath(directory)))
            return 2;
    qputenv("HOME", fixture.path().toUtf8());
    qputenv("XDG_CONFIG_HOME", fixture.filePath("config").toUtf8());
    qputenv("XDG_DATA_HOME", fixture.filePath("data").toUtf8());
    qputenv("XDG_CONFIG_DIRS", fixture.filePath("system").toUtf8());
    qputenv("XDG_DATA_DIRS", fixture.filePath("system").toUtf8());
    qputenv("XDG_CURRENT_DESKTOP", "");
    qputenv("LC_ALL", "C.UTF-8");
    QCoreApplication app(argc, argv);
    QString executable = fixture.filePath(QStringLiteral("bin/Sevenzip 空 test"));
    if (!QFile::link(QCoreApplication::applicationFilePath(), executable))
        return 2;
    executable.replace('\\', "\\\\").replace('"', "\\\"").replace('`', "\\`").replace('$', "\\$");
    executable.replace('%', "%%");
    if (!write(fixture.filePath("data/applications/sevenzip_desktop.desktop"),
               "[Desktop Entry]\nType=Application\nName=Sevenzip test\nExec=\"" +
                   executable.toUtf8() + "\" %F\n") ||
        !write(fixture.filePath("data/applications/Other.desktop"),
               "[Desktop Entry]\nType=Application\nName=Other\nExec=/usr/bin/true %F\n"))
        return 2;
    QByteArray preferences =
        "# unrelated preference\n[Default Applications]\ntext/plain=Other.desktop;\n";
    if (qEnvironmentVariableIntValue("SEVENZIP_ASSOC_OTHER"))
        preferences += "application/x-7z-compressed=sevenzip_desktop.desktop;Other.desktop;\n"
                       "[Added Associations]\n"
                       "application/x-7z-compressed=sevenzip_desktop.desktop;Other.desktop;\n";
    if (!write(fixture.filePath("config/mimeapps.list"), preferences))
        return 2;
    LinuxAssociationClearTests test;
    return QTest::qExec(&test, argc, argv);
}
#include "linux_association_clear_tests.moc"
